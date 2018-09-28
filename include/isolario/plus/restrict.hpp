// this file is only preprocessor logic, so no guards are necessary

#ifndef restrict

#ifdef __GNUC__
#define restrict __restrict__
#else
#define restrict
#endif

#endif
