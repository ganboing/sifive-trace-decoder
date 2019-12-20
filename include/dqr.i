%module TraceDecoder
//%include "typemaps.i"
//%include "various.i"
%include "cpointer.i"
%include "std_string.i"

%pointer_functions(int, intp);

typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned long uint32_t;
typedef unsigned long long uint64_t;

typedef signed char int8_t;
typedef signed short int16_t;
typedef signed long int32_t;
typedef signed long long int64_t;

/* Force the generated Java code to use the C++ enum values rather than making a JNI call */

%javaconst(1);

%{
#include "dqr.hpp"
%}

/* Let's just grab the original header file here */

%include "dqr.hpp"
