#ifndef COMPILER_INTERFACE
#define COMPILER_INTERFACE

#include "Context.h"
#include "Function.h"

// low level compiler interface
extern "C" {
void easy_register(void*, const char*, easy::GlobalMapping*, const char*, size_t); // registers a function and its associated bitcode 

easy::Context* easy_new_context(unsigned);
void easy_set_parameter_index(easy::Context*, unsigned, unsigned);
void easy_set_parameter_int(easy::Context*, unsigned, int64_t);
void easy_set_parameter_float(easy::Context*, unsigned, double);
void easy_set_parameter_ptr(easy::Context*, unsigned, void*);

easy::Function* easy_generate_code(void*, easy::Context*); // returns a pointer to the function pointer 
void easy_destroy(easy::Function*); // destroy the function 
}

#endif
