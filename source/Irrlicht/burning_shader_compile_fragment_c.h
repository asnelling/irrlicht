// fragment start
#include "burning_shader_compile_fragment_start.h"

// pixelshader
#ifdef IPOL_C0
vec4_to_fix(r0, g0, b0, line.c[0][0], inversew);
dst[i] = fix_to_sample(r0, g0, b0);
#else
dst[i] = COLOR_BRIGHT_WHITE;
#endif

#include "burning_shader_compile_fragment_end.h"
