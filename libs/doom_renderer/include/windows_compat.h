#pragma once
//
// windows_compat.h — POSIX/Unix → MSVC shims for the vintage 1997 Doom code.
//
// Force-included via cmake's `/FI` for the doom_renderer target on MSVC.
// Not included anywhere on macOS/Linux. Centralises the small set of
// renames and headers required to compile the renderer with cl.exe so
// individual *.c files stay byte-identical to the upstream source.
//

#ifdef _MSC_VER

// MSVC's _alloca is declared in <malloc.h> (no <alloca.h> on Windows).
#include <malloc.h>

// Map POSIX names to their MSVC equivalents. Each callsite calls these
// names directly (e.g. r_data.c uses `alloca(...)`); these macros redirect
// the name without changing the source.
#define alloca       _alloca
#define strcasecmp   _stricmp
#define strncasecmp  _strnicmp

#endif // _MSC_VER
