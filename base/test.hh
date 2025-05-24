#pragma once
#include "inc.hh"
namespace a {
// -----------------------------------------------------------------------------

#define test_run(name) x_test_run(#name, name)
void x_test_run(cchar* name, void (*fn)(void));

// -----------------------------------------------------------------------------

#if TEST
void test_base();
#endif

// -----------------------------------------------------------------------------
}  // namespace