#include "xbox/XboxXtl.h"
#include <cstddef>
struct Probe { char c; double d; int i; void* p; };
static_assert(sizeof(Probe) == 24, "packing changed after XboxXtl.h");
static_assert(offsetof(Probe, d) == 8, "double alignment changed after XboxXtl.h");
int main() { return 0; }
