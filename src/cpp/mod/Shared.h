/* truly no idea is __WORDSIZE is 64 on VS compiler at compiling x64 solution, I tested just with GCC */
#ifndef _MSC_VER
#if (__WORDSIZE==64) || defined(_WIN64) || defined(WIN64)	
#define IS64
#endif
#endif

#ifndef SHARED_H
#define SHARED_H

#define MAX_ASYNC_QUEUE 255

struct MENU_SCREEN {
	void *PTR0;
	void *PTR1;
};
typedef void* VOIDPTR;
enum EMENUSCREEN
{
	MENUSCREEN_FRONTENDSTART,
	MENUSCREEN_FRONTENDINGAME,
	MENUSCREEN_FRONTENDLOADING,
	MENUSCREEN_FRONTENDRESET,
	MENUSCREEN_FRONTENDTEST,
	MENUSCREEN_FRONTENDSPLASH,
	MENUSCREEN_COUNT
};
typedef MENU_SCREEN* MENU_SCREEN_PTR;
template<int T,int NPtr,class Q>
struct OFFSET_STRUCT {
	unsigned char dummy[T];
	Q arr[NPtr];
};
typedef OFFSET_STRUCT<0x68, 6, MENU_SCREEN_PTR> FLASH_OBJ_32_6156;
typedef OFFSET_STRUCT<0x80, 6, MENU_SCREEN_PTR> FLASH_OBJ_64_6156;
struct GAME_32_6156 {
	unsigned char dummy[0x30];
	FLASH_OBJ_32_6156 *pFlashObj;
};

/* Trying to compile the headers from SDK => 4MB error log @ G++ */
#ifdef _MSC_VER
	#define USE_SDK
	#if _MSC_VER <= 1600  // VS2010 and older
		#define OLD_MSVC_DETECTED  // almost no C++11 support
	#endif
#endif

void ToggleLoading(const char *text, bool loading = true, bool reset = true);

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