/* truly no idea is __WORDSIZE is 64 on VS compiler at compiling x64 solution, I tested just with GCC */
#ifndef _MSC_VER
#if (__WORDSIZE==64) || defined(_WIN64) || defined(WIN64)	
#define IS64
#endif
#endif

#ifndef SHARED_H
#define SHARED_H

/* Trying to compile the headers from SDK => 4MB error log @ G++ */
#ifdef _MSC_VER
	#define USE_SDK
#endif

#ifdef USE_SDK
//#define WANT_CIRCLEJUMP
#define mkcall(r,s,o)	/* do nothing */
#define mkcall2(r,s,o,p1,p2)	/* do nothing */
#else
#define mkcall(r,s,o)\
asm(\
	"movl (%1),%%eax\n"\
	"movl "#o"(%%eax),%%edx\n"\
	"movl %1,%%ecx\n"\
	"call %%edx\n"\
	"movl %%eax,%0\n"\
	:"=r"(r)\
	:"r"(s)\
	:"%eax","%edx","%ecx")
#define mkcall2(r,s,o,p1,p2)\
asm(\
	"movl (%1),%%eax\n"\
	"movl "#o"(%%eax),%%edx\n"\
	"movl %1,%%ecx\n"\
	"pushl %3\n"\
	"pushl %2\n"\
	"call %%edx\n"\
	"movl %%eax,%0\n"\
	:"=r"(r)\
	:"r"(s),"r"(p1),"r"(p2)\
	:"%eax","%edx","%ecx")
#endif

void* hookp(void *c,const void *d,const int sz);
int getGameVer(const char*);
void hook(void *src,void *dest);
void unhook(void *src);
#include <string>
std::string fastDownload(const char *url);
bool autoUpdateClient();
#pragma comment(lib,"wininet")

#endif SHARED_H