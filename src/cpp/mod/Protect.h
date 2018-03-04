#pragma once
#ifndef __PROTECT_H__
#define __PROTECT_H__

#include "Shared.h"

#ifdef _MSC_VER
# ifdef _WIN64
#  define EMIT(op) /* do nothing*/
# else
#  define EMIT(op) __asm { _asm _emit op }
# endif
#else
# define EMIT(op) asm __volatile__ (".byte " #op);
#endif

#define MK_PROTECT_SIG EMIT(0xE9); EMIT(0x01); EMIT(0x00); EMIT(0x00); EMIT(0x00); EMIT(0xE9);
#define PROTECT(name) {\
	MK_PROTECT_SIG; \
	DWORD flags;\
	VirtualProtect((void*)name, 4096, PAGE_EXECUTE_READWRITE, &flags);\
	unsigned char *_ptr = (unsigned char*)name;\
	bool _tmp_start = true;\
	for(;;){\
		if(_ptr[0]==0xE9 && _ptr[1]==0 && _ptr[2]==0 && _ptr[3]==0 && _ptr[4]==0 && _ptr[5]==0x90){\
			if(_tmp_start){\
				 _ptr+=6;\
				_tmp_start = false;\
				continue;\
			} else {\
				break;\
			}\
		} else if(!_tmp_start){\
			 *_ptr ^= 0x5A;\
		}\
		_ptr++;\
	}\
	VirtualProtect((void*)name, 4096, flags, 0);\
}

#ifdef _MSC_VER
# ifdef _WIN64
#  define SIGNATURE { extern long long PROTECT_FLAG = 0x9000000000E9LL; }
# else
#  define SIGNATURE _asm _emit 0xE9 _asm _emit 0x00 _asm _emit 0x00 _asm _emit 0x00 _asm _emit 0x00 _asm _emit 0x90
# endif
#else
# define SIGNATURE asm __volatile__ (".byte 0x90\n .byte 0x90\n .byte 0x90\n .byte 0x90"); 
#endif

#ifdef OFFICIAL_BUILD
#define PROTECT_BEGIN(name) PROTECT(name); SIGNATURE;
#define PROTECT_END(name) SIGNATURE; PROTECT(name);
#else
#define PROTECT_BEGIN(name) /* do nothing */
#define PROTECT_END(name) /* do nothing*/
#endif

static long PROTECT_FLAG = 0;

#endif