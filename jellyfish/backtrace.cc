#include <iostream>
#include <stdexcept>
#include <execinfo.h>
#include <stdlib.h>
#include <typeinfo>
#include <cxxabi.h>

static void handler() {
  // Display message of last thrown exception if any
  try { throw; }
  catch(const std::exception& e) { 
    int     status;
    size_t  n = 0;
    char   *name = abi::__cxa_demangle(typeid(e).name(), 0, &n, &status);
    std::cerr << "terminate called after throwing an instance of '"
              << (status < 0 ? "UNKNOWN" : name)
              << "'\n  what(): " << e.what() << "\n"; 
    if(n)
      free(name);
  }
  catch(...) {}

  void  *trace_elems[20];
  int    trace_elem_count(backtrace(trace_elems, 20));
  backtrace_symbols_fd(trace_elems, trace_elem_count, 2);
  abort();
}

void show_backtrace() {
  std::set_terminate(handler);
}
