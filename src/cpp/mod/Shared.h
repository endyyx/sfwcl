#ifndef SHARED_H
#define SHARED_H

#define THREAD_SAFE		//for mutexes

#pragma comment(lib, "wininet")
#pragma comment(lib, "wldap32")
#pragma comment(lib, "shell32")
#pragma comment(lib, "ws2_32")

#ifdef _WIN64
#ifndef IS64
#define IS64  // 64-bit build
#endif
#endif

#ifdef IS64
typedef unsigned long long uintptr_t;
#define ARCH_BITS 64
#else
typedef unsigned int uintptr_t;
#define ARCH_BITS 32
#endif

#ifndef getField
#define getField(type,base,offset) (*(type*)(((unsigned char*)base)+offset))
#define GET_FIELD getField
#endif

#define MAX_ASYNC_QUEUE 6

struct MENU_SCREEN {
	void *PTR0;	//Virtual table pointer
	void *PTR1;	//Actual first item
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

void* trampoline(void *oldfn, void *newfn, int sz, int bits = ARCH_BITS);
#define hookp trampoline
int getGameVer(const char*);
void hook(void *src,void *dest);
void unhook(void *src);
#include <string>
std::string fastDownload(const char *url);
bool autoUpdateClient();

#endif SHARED_H