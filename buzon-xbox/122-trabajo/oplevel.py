import sys, os
root = sys.argv[1]  # .../src/net/minecraft/src

def rw(name, fn):
    p = os.path.join(root, name)
    s = open(p, encoding='utf-8', newline='').read()
    nl = '\r\n' if '\r\n' in s else '\n'
    s2 = fn(s.replace('\r\n', '\n')).replace('\n', nl)
    open(p, 'w', encoding='utf-8', newline='').write(s2)
    print('ok', name)

def h(s):
    old = 'bool statusToNative(const std::string &statusJson'
    assert s.count(old) == 1
    return s.replace(old, '// Op permission level the server gave the local player (Entity Status 24..28),\n'
                     '// -1 until it says; reset on every login. The admin mode mod reads it.\n'
                     'int localOpLevel();\nvoid resetLocalOpLevel();\n\n' + old, 1)

def pk(s):
    old = 'namespace translator340\n{\nnamespace\n{\n'
    assert s.count(old) == 1
    s = s.replace(old, old + 'std::atomic<int> s_localOpLevel{-1};\n', 1)
    old2 = '\t\tconst int32_t eid = c.i32();\n\t\tconst int8_t status = c.i8();\n\t\tif (!c.ok || status < 0 || status > 11)'
    assert s.count(old2) == 1
    s = s.replace(old2, '\t\tconst int32_t eid = c.i32();\n\t\tconst int8_t status = c.i8();\n'
                  '\t\t// 24..28: op permission level 0..4, only ever sent about the local player.\n'
                  '\t\tif (c.ok && status >= 24 && status <= 28)\n'
                  '\t\t\ts_localOpLevel.store(status - 24, std::memory_order_relaxed);\n'
                  '\t\tif (!c.ok || status < 0 || status > 11)', 1)
    i = s.rfind('} // namespace translator340')
    assert i > 0
    fn = ('int localOpLevel()\n{\n\treturn s_localOpLevel.load(std::memory_order_relaxed);\n}\n\n'
          'void resetLocalOpLevel()\n{\n\ts_localOpLevel.store(-1, std::memory_order_relaxed);\n}\n\n')
    return s[:i] + fn + s[i:]

def io(s):
    old = '\t\tsession_.stage.store(static_cast<int>(translator340::Session::Stage::Login), std::memory_order_release);'
    assert s.count(old) == 1
    return s.replace(old, '\t\ttranslator340::resetLocalOpLevel();\n' + old, 1)

def nch(s):
    old = 'NetClientHandler::NetClientHandler(Minecraft* minecraft, const std::string& host, int port, int protocolVersion)\n{\n'
    assert s.count(old) == 1
    s = s.replace(old, old + '    translator340::resetLocalOpLevel(); // new connection: not op until the server says so\n', 1)
    inc = '#include "Packet38EntityStatus.h"'
    assert s.count(inc) == 1
    return s.replace(inc, inc + '\n#include "ProtocolTranslator340.h"', 1)

rw('ProtocolTranslator340.h', h)
rw('ProtocolTranslator340Packets.cpp', pk)
rw('ProtocolTranslator340IO.cpp', io)
rw('NetClientHandler.cpp', nch)
