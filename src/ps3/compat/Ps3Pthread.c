// Ps3Pthread.c -- the pthread subset libgcc/libstdc++ need, on lv2 syscalls.
//
// The PS3 toolchain is gcc 13 built with --enable-threads=posix against the
// pthread-embedded (PTE) headers (ps3\tools\construir_gcc_hilos.sh), so
// std::thread/std::mutex/std::condition_variable call pthread_*. Neither PTE's
// library nor PSL1GHT's sysLwMutex*/sysThreadGetId can serve them: libgcc locks
// a pthread mutex during the C runtime startup (crtbegin ->
// __register_frame_info), before main, before .preinit_array and before
// PSL1GHT has bound the firmware's sprx imports -- and those helpers are sprx
// imports, so calling them that early jumps into an unbound stub and spins
// forever. PTE additionally needs a global pthread_init() first.
//
// So everything here is a direct lv2 syscall, valid from the first instruction,
// and nothing needs initialising. Same types as PTE's pthread.h (pointer-sized
// handles with -1 static initialisers, pthread_t = { void* p; unsigned x; }):
//
//  * mutex       -> sys_mutex (syscalls 100-104), created on first use; the ones
//                   created from a static initialiser come from a static pool,
//                   so the startup path does not touch malloc either;
//  * cond        -> sys_cond (syscalls 105-109), bound to the mutex of its first
//                   wait (std::condition_variable always waits with the same one);
//  * once        -> atomic state word;
//  * keys (TLS)  -> a small table per thread id;
//  * threads     -> sys_ppu_thread, joinable unless detached;
//  * rwlock      -> a plain mutex (exclusive: correct, just not parallel).
#ifdef PS3_PLATFORM

#include <errno.h>
#include <pthread.h>
#include <sched.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <ppu-lv2.h>
#include <sys/cond.h>
#include <sys/mutex.h>
#include <sys/systime.h>
#include <sys/thread.h>

#define PS3_LV2_ETIMEDOUT 0x8001000Bu
#define PS3_THREAD_PRIORITY 1001
#define PS3_THREAD_STACK (512 * 1024)
#define PS3_MAX_KEYS 64
#define PS3_MAX_THREADS 128
#define PS3_STATIC_MUTEX_POOL 512

// ---------------------------------------------------------------------------
// Raw lv2 syscalls (no sprx imports)
// ---------------------------------------------------------------------------
static s32 lv2ThreadGetId(sys_ppu_thread_t* id)
{
    lv2syscall1(48, (u64)id);
    return_to_user_prog(s32);
}

static void lv2ThreadYield(void)
{
    lv2syscall0(43);
}

// ---------------------------------------------------------------------------
// Mutexes
// ---------------------------------------------------------------------------
struct pthread_mutex_t_
{
    sys_mutex_t id;
    int recursive;
};

struct pthread_mutexattr_t_
{
    int type;
};

static struct pthread_mutex_t_ s_mutexPool[PS3_STATIC_MUTEX_POOL];
static int s_mutexPoolUsed = 0;

static int mutexCreateInto(struct pthread_mutex_t_* m, int recursive)
{
    sys_mutex_attr_t attr;
    memset(&attr, 0, sizeof(attr));
    attr.attr_protocol = SYS_MUTEX_PROTOCOL_FIFO;
    attr.attr_recursive = recursive ? SYS_MUTEX_ATTR_RECURSIVE : SYS_MUTEX_ATTR_NOT_RECURSIVE;
    attr.attr_pshared = SYS_MUTEX_ATTR_NOT_PSHARED;
    attr.attr_adaptive = SYS_MUTEX_ATTR_NOT_ADAPTIVE;
    memcpy(attr.name, "ocmutex", 8);
    if (sysMutexCreate(&m->id, &attr) != 0)
        return 0;
    m->recursive = recursive;
    return 1;
}

static int isPooled(const struct pthread_mutex_t_* m)
{
    return m >= s_mutexPool && m < s_mutexPool + PS3_STATIC_MUTEX_POOL;
}

static struct pthread_mutex_t_* mutexCreate(int recursive, int fromStatic)
{
    struct pthread_mutex_t_* m = NULL;
    if (fromStatic)
    {
        const int slot = __atomic_fetch_add(&s_mutexPoolUsed, 1, __ATOMIC_ACQ_REL);
        if (slot < PS3_STATIC_MUTEX_POOL)
            m = &s_mutexPool[slot];
    }
    if (m == NULL)
        m = (struct pthread_mutex_t_*)calloc(1, sizeof(*m));
    if (m == NULL)
        return NULL;
    if (!mutexCreateInto(m, recursive))
    {
        if (!isPooled(m))
            free(m);
        return NULL;
    }
    return m;
}

static void mutexFree(struct pthread_mutex_t_* m)
{
    sysMutexDestroy(m->id);
    if (!isPooled(m))
        free(m);
}

static int isStaticInitializer(const void* value)
{
    const intptr_t v = (intptr_t)value;
    return v == -1 || v == -2 || v == -3;
}

// Resolves a static initialiser (PTHREAD_MUTEX_INITIALIZER = -1, recursive = -2,
// error-check = -3) into a real mutex the first time it is used, race-free.
static struct pthread_mutex_t_* ensureMutex(pthread_mutex_t* handle)
{
    struct pthread_mutex_t_* current = __atomic_load_n((struct pthread_mutex_t_**)handle, __ATOMIC_ACQUIRE);
    if (!isStaticInitializer(current))
        return current;
    struct pthread_mutex_t_* created = mutexCreate((intptr_t)current == -2, 1);
    if (created == NULL)
        return NULL;
    struct pthread_mutex_t_* expected = current;
    if (__atomic_compare_exchange_n((struct pthread_mutex_t_**)handle, &expected, created, 0, __ATOMIC_ACQ_REL,
                                    __ATOMIC_ACQUIRE))
        return created;
    mutexFree(created);
    return expected;
}

int pthread_mutexattr_init(pthread_mutexattr_t* attr)
{
    struct pthread_mutexattr_t_* a = (struct pthread_mutexattr_t_*)calloc(1, sizeof(*a));
    if (a == NULL)
        return ENOMEM;
    a->type = PTHREAD_MUTEX_NORMAL;
    *attr = a;
    return 0;
}

int pthread_mutexattr_destroy(pthread_mutexattr_t* attr)
{
    free(*attr);
    *attr = NULL;
    return 0;
}

int pthread_mutexattr_settype(pthread_mutexattr_t* attr, int type)
{
    (*attr)->type = type;
    return 0;
}

int pthread_mutexattr_gettype(pthread_mutexattr_t* attr, int* type)
{
    *type = (*attr)->type;
    return 0;
}

int pthread_mutex_init(pthread_mutex_t* mutex, const pthread_mutexattr_t* attr)
{
    const int recursive = attr != NULL && *attr != NULL && (*attr)->type == PTHREAD_MUTEX_RECURSIVE;
    struct pthread_mutex_t_* m = mutexCreate(recursive, 0);
    if (m == NULL)
        return ENOMEM;
    *mutex = m;
    return 0;
}

int pthread_mutex_destroy(pthread_mutex_t* mutex)
{
    struct pthread_mutex_t_* m = *mutex;
    if (m != NULL && !isStaticInitializer(m))
        mutexFree(m);
    *mutex = NULL;
    return 0;
}

int pthread_mutex_lock(pthread_mutex_t* mutex)
{
    struct pthread_mutex_t_* m = ensureMutex(mutex);
    if (m == NULL)
        return EINVAL;
    return sysMutexLock(m->id, 0) == 0 ? 0 : EINVAL;
}

int pthread_mutex_trylock(pthread_mutex_t* mutex)
{
    struct pthread_mutex_t_* m = ensureMutex(mutex);
    if (m == NULL)
        return EINVAL;
    return sysMutexTryLock(m->id) == 0 ? 0 : EBUSY;
}

int pthread_mutex_unlock(pthread_mutex_t* mutex)
{
    struct pthread_mutex_t_* m = ensureMutex(mutex);
    if (m == NULL)
        return EINVAL;
    return sysMutexUnlock(m->id) == 0 ? 0 : EPERM;
}

// ---------------------------------------------------------------------------
// Time helpers
// ---------------------------------------------------------------------------
// Microseconds from now until an absolute CLOCK_REALTIME deadline; 0 = passed.
static uint64_t microsUntil(const struct timespec* deadline)
{
    u64 sec = 0, nsec = 0;
    sysGetCurrentTime(&sec, &nsec);
    const int64_t now = (int64_t)sec * 1000000 + (int64_t)(nsec / 1000);
    const int64_t end = (int64_t)deadline->tv_sec * 1000000 + (int64_t)(deadline->tv_nsec / 1000);
    return end > now ? (uint64_t)(end - now) : 0;
}

int pthread_mutex_timedlock(pthread_mutex_t* mutex, const struct timespec* deadline)
{
    struct pthread_mutex_t_* m = ensureMutex(mutex);
    if (m == NULL)
        return EINVAL;
    const uint64_t wait = microsUntil(deadline);
    if (wait == 0)
        return sysMutexTryLock(m->id) == 0 ? 0 : ETIMEDOUT;
    return sysMutexLock(m->id, wait) == 0 ? 0 : ETIMEDOUT;
}

// ---------------------------------------------------------------------------
// Condition variables
// ---------------------------------------------------------------------------
struct pthread_cond_t_
{
    sys_cond_t id;
    struct pthread_mutex_t_* bound;  // the mutex the sys_cond was created with
};

struct pthread_condattr_t_
{
    int unused;
};

int pthread_cond_init(pthread_cond_t* cond, const pthread_condattr_t* attr)
{
    (void)attr;
    *cond = PTHREAD_COND_INITIALIZER;  // the sys_cond is created at the first wait
    return 0;
}

int pthread_cond_destroy(pthread_cond_t* cond)
{
    struct pthread_cond_t_* c = *cond;
    if (c != NULL && !isStaticInitializer(c))
    {
        sysCondDestroy(c->id);
        free(c);
    }
    *cond = NULL;
    return 0;
}

// Called with `m` locked, so creation is serialised with every other waiter of
// the same mutex; publication is still atomic for signalers that do not lock.
static struct pthread_cond_t_* ensureCond(pthread_cond_t* handle, struct pthread_mutex_t_* m)
{
    struct pthread_cond_t_* current = __atomic_load_n((struct pthread_cond_t_**)handle, __ATOMIC_ACQUIRE);
    if (current != NULL && !isStaticInitializer(current))
        return current;
    struct pthread_cond_t_* c = (struct pthread_cond_t_*)calloc(1, sizeof(*c));
    if (c == NULL)
        return NULL;
    sys_cond_attr_t attr;
    memset(&attr, 0, sizeof(attr));
    attr.attr_pshared = SYS_COND_ATTR_PSHARED;
    memcpy(attr.name, "occond", 7);
    if (sysCondCreate(&c->id, m->id, &attr) != 0)
    {
        free(c);
        return NULL;
    }
    c->bound = m;
    struct pthread_cond_t_* expected = current;
    if (__atomic_compare_exchange_n((struct pthread_cond_t_**)handle, &expected, c, 0, __ATOMIC_ACQ_REL,
                                    __ATOMIC_ACQUIRE))
        return c;
    sysCondDestroy(c->id);
    free(c);
    return expected;
}

static int condWait(pthread_cond_t* cond, pthread_mutex_t* mutex, uint64_t timeoutUs, int timed)
{
    struct pthread_mutex_t_* m = ensureMutex(mutex);
    if (m == NULL)
        return EINVAL;
    struct pthread_cond_t_* c = ensureCond(cond, m);
    if (c == NULL)
        return EINVAL;
    if (c->bound != m)
    {
        // Waited with a different mutex than the first wait: lv2 ties a cond to
        // one mutex. Behave as a spurious wakeup after a short sleep (allowed by
        // POSIX; callers re-check their predicate).
        sysMutexUnlock(m->id);
        sysUsleep(1000);
        sysMutexLock(m->id, 0);
        return timed && timeoutUs <= 1000 ? ETIMEDOUT : 0;
    }
    if (timed && timeoutUs == 0)
        return ETIMEDOUT;
    const s32 result = sysCondWait(c->id, timed ? timeoutUs : 0);
    if (result == 0)
        return 0;
    return (u32)result == PS3_LV2_ETIMEDOUT ? ETIMEDOUT : EINVAL;
}

int pthread_cond_wait(pthread_cond_t* cond, pthread_mutex_t* mutex)
{
    return condWait(cond, mutex, 0, 0);
}

int pthread_cond_timedwait(pthread_cond_t* cond, pthread_mutex_t* mutex, const struct timespec* deadline)
{
    return condWait(cond, mutex, microsUntil(deadline), 1);
}

int pthread_cond_signal(pthread_cond_t* cond)
{
    struct pthread_cond_t_* c = __atomic_load_n((struct pthread_cond_t_**)cond, __ATOMIC_ACQUIRE);
    if (c == NULL || isStaticInitializer(c))
        return 0;  // nobody has ever waited on it
    sysCondSignal(c->id);
    return 0;
}

int pthread_cond_broadcast(pthread_cond_t* cond)
{
    struct pthread_cond_t_* c = __atomic_load_n((struct pthread_cond_t_**)cond, __ATOMIC_ACQUIRE);
    if (c == NULL || isStaticInitializer(c))
        return 0;
    sysCondBroadcast(c->id);
    return 0;
}

// ---------------------------------------------------------------------------
// Once
// ---------------------------------------------------------------------------
int pthread_once(pthread_once_t* once, void (*routine)(void))
{
    // state: 0 = not run, 1 = running, 2 = done.
    int expected = 0;
    if (__atomic_compare_exchange_n(&once->state, &expected, 1, 0, __ATOMIC_ACQ_REL, __ATOMIC_ACQUIRE))
    {
        routine();
        __atomic_store_n(&once->state, 2, __ATOMIC_RELEASE);
        return 0;
    }
    while (__atomic_load_n(&once->state, __ATOMIC_ACQUIRE) != 2)
        lv2ThreadYield();
    return 0;
}

// ---------------------------------------------------------------------------
// Thread-specific data
// ---------------------------------------------------------------------------
struct pthread_key_t_
{
    int index;
};

typedef struct
{
    sys_ppu_thread_t id;
    int used;
    void* values[PS3_MAX_KEYS];
} ThreadSlot;

static struct pthread_key_t_ s_keys[PS3_MAX_KEYS];
static void (*s_destructors[PS3_MAX_KEYS])(void*);
static int s_keyUsed[PS3_MAX_KEYS];
static ThreadSlot s_slots[PS3_MAX_THREADS];
static int s_tableLock = 0;

static void tableLock(void)
{
    int expected = 0;
    while (!__atomic_compare_exchange_n(&s_tableLock, &expected, 1, 0, __ATOMIC_ACQUIRE, __ATOMIC_RELAXED))
    {
        expected = 0;
        lv2ThreadYield();
    }
}

static void tableUnlock(void)
{
    __atomic_store_n(&s_tableLock, 0, __ATOMIC_RELEASE);
}

static sys_ppu_thread_t currentThreadId(void)
{
    sys_ppu_thread_t id = 0;
    lv2ThreadGetId(&id);
    return id;
}

// The calling thread's slot; created if `create`. Caller holds the table lock.
static ThreadSlot* slotFor(sys_ppu_thread_t id, int create)
{
    ThreadSlot* unused = NULL;
    for (int i = 0; i < PS3_MAX_THREADS; ++i)
    {
        if (s_slots[i].used && s_slots[i].id == id)
            return &s_slots[i];
        if (!s_slots[i].used && unused == NULL)
            unused = &s_slots[i];
    }
    if (!create || unused == NULL)
        return NULL;
    memset(unused, 0, sizeof(*unused));
    unused->used = 1;
    unused->id = id;
    return unused;
}

int pthread_key_create(pthread_key_t* key, void (*destructor)(void*))
{
    tableLock();
    for (int i = 0; i < PS3_MAX_KEYS; ++i)
    {
        if (!s_keyUsed[i])
        {
            s_keyUsed[i] = 1;
            s_destructors[i] = destructor;
            s_keys[i].index = i;
            *key = &s_keys[i];
            tableUnlock();
            return 0;
        }
    }
    tableUnlock();
    return EAGAIN;
}

int pthread_key_delete(pthread_key_t key)
{
    tableLock();
    const int i = key->index;
    s_keyUsed[i] = 0;
    s_destructors[i] = NULL;
    for (int t = 0; t < PS3_MAX_THREADS; ++t)
        s_slots[t].values[i] = NULL;
    tableUnlock();
    return 0;
}

void* pthread_getspecific(pthread_key_t key)
{
    tableLock();
    ThreadSlot* slot = slotFor(currentThreadId(), 0);
    void* value = slot != NULL ? slot->values[key->index] : NULL;
    tableUnlock();
    return value;
}

int pthread_setspecific(pthread_key_t key, const void* value)
{
    tableLock();
    ThreadSlot* slot = slotFor(currentThreadId(), 1);
    if (slot != NULL)
        slot->values[key->index] = (void*)value;
    tableUnlock();
    return slot != NULL ? 0 : ENOMEM;
}

// Runs the key destructors for the exiting thread and frees its slot.
static void runKeyDestructors(void)
{
    const sys_ppu_thread_t id = currentThreadId();
    for (int round = 0; round < 4; ++round)
    {
        void* pending[PS3_MAX_KEYS];
        void (*dtors[PS3_MAX_KEYS])(void*);
        int count = 0;
        tableLock();
        ThreadSlot* slot = slotFor(id, 0);
        if (slot != NULL)
        {
            for (int i = 0; i < PS3_MAX_KEYS; ++i)
            {
                if (slot->values[i] != NULL && s_destructors[i] != NULL)
                {
                    pending[count] = slot->values[i];
                    dtors[count] = s_destructors[i];
                    ++count;
                    slot->values[i] = NULL;
                }
            }
        }
        tableUnlock();
        if (count == 0)
            break;
        for (int i = 0; i < count; ++i)
            dtors[i](pending[i]);
    }
    tableLock();
    ThreadSlot* slot = slotFor(id, 0);
    if (slot != NULL)
        slot->used = 0;
    tableUnlock();
}

// ---------------------------------------------------------------------------
// Threads (created from game code, long after startup: sprx helpers are fine)
// ---------------------------------------------------------------------------
struct pthread_attr_t_
{
    u64 stackSize;
    int detached;
};

typedef struct
{
    void* (*routine)(void*);
    void* argument;
} ThreadStart;

static void threadEntry(void* raw)
{
    ThreadStart start = *(ThreadStart*)raw;
    free(raw);
    void* result = start.routine(start.argument);
    runKeyDestructors();
    sysThreadExit((u64)(uintptr_t)result);
}

int pthread_attr_init(pthread_attr_t* attr)
{
    struct pthread_attr_t_* a = (struct pthread_attr_t_*)calloc(1, sizeof(*a));
    if (a == NULL)
        return ENOMEM;
    a->stackSize = PS3_THREAD_STACK;
    *attr = a;
    return 0;
}

int pthread_attr_destroy(pthread_attr_t* attr)
{
    free(*attr);
    *attr = NULL;
    return 0;
}

int pthread_attr_setstacksize(pthread_attr_t* attr, size_t size)
{
    (*attr)->stackSize = size;
    return 0;
}

int pthread_attr_setdetachstate(pthread_attr_t* attr, int state)
{
    (*attr)->detached = state == PTHREAD_CREATE_DETACHED;
    return 0;
}

int pthread_create(pthread_t* thread, const pthread_attr_t* attr, void* (*routine)(void*), void* argument)
{
    ThreadStart* start = (ThreadStart*)malloc(sizeof(ThreadStart));
    if (start == NULL)
        return EAGAIN;
    start->routine = routine;
    start->argument = argument;
    u64 stack = PS3_THREAD_STACK;
    int detached = 0;
    if (attr != NULL && *attr != NULL)
    {
        if ((*attr)->stackSize >= 16 * 1024)
            stack = (*attr)->stackSize;
        detached = (*attr)->detached;
    }
    sys_ppu_thread_t id = 0;
    char name[] = "opticraft";
    if (sysThreadCreate(&id, threadEntry, start, PS3_THREAD_PRIORITY, stack, detached ? 0 : THREAD_JOINABLE,
                        name) != 0)
    {
        free(start);
        return EAGAIN;
    }
    thread->p = (void*)(uintptr_t)id;
    thread->x = 1;
    return 0;
}

int pthread_join(pthread_t thread, void** result)
{
    u64 value = 0;
    if (sysThreadJoin((sys_ppu_thread_t)(uintptr_t)thread.p, &value) != 0)
        return ESRCH;
    if (result != NULL)
        *result = (void*)(uintptr_t)value;
    return 0;
}

int pthread_detach(pthread_t thread)
{
    return sysThreadDetach((sys_ppu_thread_t)(uintptr_t)thread.p) == 0 ? 0 : ESRCH;
}

pthread_t pthread_self(void)
{
    pthread_t self;
    self.p = (void*)(uintptr_t)currentThreadId();
    self.x = 1;
    return self;
}

int pthread_equal(pthread_t a, pthread_t b)
{
    return a.p == b.p;
}

// libgcc's __gthread_active_p() keys off this symbol being linked; threads
// cannot be cancelled on lv2.
int pthread_cancel(pthread_t thread)
{
    (void)thread;
    return ESRCH;
}

int sched_yield(void)
{
    lv2ThreadYield();
    return 0;
}

// ---------------------------------------------------------------------------
// Read-write locks: exclusive (a plain mutex)
// ---------------------------------------------------------------------------
int pthread_rwlock_init(pthread_rwlock_t* lock, const pthread_rwlockattr_t* attr)
{
    (void)attr;
    *lock = (pthread_rwlock_t)mutexCreate(0, 0);
    return *lock != NULL ? 0 : ENOMEM;
}

int pthread_rwlock_destroy(pthread_rwlock_t* lock)
{
    return pthread_mutex_destroy((pthread_mutex_t*)lock);
}

int pthread_rwlock_rdlock(pthread_rwlock_t* lock)
{
    return pthread_mutex_lock((pthread_mutex_t*)lock);
}

int pthread_rwlock_wrlock(pthread_rwlock_t* lock)
{
    return pthread_mutex_lock((pthread_mutex_t*)lock);
}

int pthread_rwlock_unlock(pthread_rwlock_t* lock)
{
    return pthread_mutex_unlock((pthread_mutex_t*)lock);
}

#endif // PS3_PLATFORM
