#ifndef COMMON_H_
#define COMMON_H_

#if DEBUG
#define debug_int(thing) printf(#thing " = %d\n", thing)
#define debug_double(thing) printf(#thing " = %f\n", thing)
#define debug_nl() puts("")
#else
#define debug_int(...)
#define debug_double(...)
#define debug_nl(...)
#endif

#define countof(X) (sizeof (X) / sizeof (X)[0])

#endif

