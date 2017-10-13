#define UNICODE
#include <Windows.h>
#include <stdio.h>
#include <string.h>
#include <string>
#include <time.h>
#include <fstream>
#include <string>
#include <direct.h>
#pragma comment(lib,"Ws2_32")
#pragma comment(lib,"urlmon")
using namespace std;

#define LOADINTO(o,delim){\
    int c=0;\
    for(;i<len,argv[1][i]!=delim;i++,c++)\
        o[c]=argv[1][i];\
    i++;\
    o[c]=0;\
    }

bool LoadFileDialog(char *nPath){
    OPENFILENAME name;
    wchar_t buffer[0xFF];
    for(short j=0;j<255;j++)buffer[j]=0;
    ZeroMemory(&name,sizeof(name));
    name.lStructSize=sizeof(name);
    name.lpstrFilter=L"Crysis executable (Cry*.exe)\0Cry*.exe\0|";
    name.lpstrCustomFilter=L"Crysis executable (Cry*.exe)\0Cry*.exe\0|";
    name.hwndOwner=GetConsoleWindow();
    name.lpstrFile=buffer;
    name.nMaxFile=sizeof buffer;
    name.Flags=OFN_EXPLORER;
    name.lpstrDefExt=L"exe";
    name.lpstrTitle=L"Open";
    name.lpstrFileTitle=L"Crysis.exe";
    name.lpstrInitialDir=NULL;
    if(GetOpenFileName(&name)){
        for(short i=0;i<0xFF;i++)nPath[i]=static_cast<char>(buffer[i]);
        printf("Path: %s",nPath);
		return true;
    }  else {
		printf("No path selected!");
		return false;
	}
}
bool DirExists(const char *d){
  DWORD a=GetFileAttributesA(d);
  return (a!=INVALID_FILE_ATTRIBUTES&&(a&FILE_ATTRIBUTE_DIRECTORY));
}
int main(int argc,char **argv){
    if(argc<2){
        printf("Usage: <ip:port:id:uid:pwd:mapname|mapdl@name>");
		return 1;
    } else {
		char cwd[512];
		GetModuleFileNameA(0,cwd,5152);
		int last=-1;
		for(int i=0,j=strlen(cwd);i<j;i++){
			if(cwd[i]=='\\')
				last=i;
		}
		if(last>=0)
			cwd[last]=0;
        char ip[255];
        char port[255];
        char profid[255];
        char uid[255];
        char name[255];
        char pwd[255];
        char mapn[255];
        char mapdl[255];
        ZeroMemory(pwd,sizeof(pwd));
        ZeroMemory(mapdl,sizeof(mapdl));
        bool usePwd=false;
        int i=0;
        int len=(int)strlen(argv[1]);
        //printf("%s",argv[1]);
        while((argv[1][i]) && argv[1][i]!='%')
            i++;
        i++;
        /*
            fmt:
            %ip:port:profid:uid@name%mapName:pwd:mapDl\0
        */
        LOADINTO(ip,':');
        LOADINTO(port,':');
        LOADINTO(profid,':');
        LOADINTO(uid,'@');
        LOADINTO(name,'%');
        LOADINTO(mapn,':');
		LOADINTO(pwd,':');
		LOADINTO(mapdl,0);
        usePwd=(strlen(pwd)>0);
        if(!strcmp(ip,"<local>")){
            WSAData data;
            WSAStartup(0x202,&data);
            char hostn[255];
            if (gethostname(hostn, sizeof(hostn))!= SOCKET_ERROR) {
                struct hostent *host = gethostbyname(hostn);
                if(host){
                    for (int i = 0; host->h_addr_list[i] != 0; ++i) {
                        struct in_addr addr;
                        memcpy(&addr, host->h_addr_list[i], sizeof(struct in_addr));
                        strcpy_s(ip,inet_ntoa(addr));
                    }
                }
            }
            WSACleanup();
        }
		if(isalpha(*ip)){
			sprintf_s(ip,"%X%s",*ip,ip+1);
		}
		printf("Cwd: %s\n",cwd);
        printf("IP: %s\n",ip);
        printf("Port: %s\n",port);
        printf("Profile ID: %s\n",profid);
        printf("uID: %s\n",uid);
        printf("Name: %s\n",name);
        printf("Use password: %s\n",usePwd?"true":"false");
        printf("Password: %s\n",pwd);
        printf("Map: %s\n",mapn);
        if(strlen(mapdl)){
            printf("Map URL: %s\n",mapdl);
        }
        char path[1024];
        bool ok=LoadFileDialog(path);
		if(!ok){
			return 1;
		}
        char cmd[2048];
        char *gm=path;
        int j=strlen(path);
        for(int i=0;i<j;i++){
            if(path[i]=='/' || path[i]=='\\'){
                gm=path+i+1;
            }
        }
        if(gm!=path)
            *(gm-1)=0;
        sprintf_s(cmd,"cd %s",path);
        printf("\nDirectory: %s\n",path);
        printf("Game: %s\n",gm);
        system(cmd);
        if(strlen(mapdl)>5){
			char mpath[512];
			char omapn[512];
			strcpy_s(omapn,mapn);
			char *ver=0;
			for(int i=0,j=strlen(mapn);i<j;i++){
				if(mapn[i]=='|'){
					mapn[i]=0;
					ver=mapn+i+1;
				}
			}
			for(int i=0,j=strlen(mapn);i<j;i++)
				mapn[i]=mapn[i]=='/'?'\\':mapn[i];
			sprintf_s(mpath,"%s\\..\\Game\\Levels\\%s",path,mapn);
            bool exists=DirExists(mpath);
			if(ver && exists){
				printf("Map version: %s\n",ver);
				char lpath[512];
				sprintf_s(lpath,"%s\\..\\Game\\_levels.dat",path);
				exists=false;
				FILE *f=fopen(lpath,"r");
				if(f){
					char name[255];
					char veri[255];
					while(!feof(f)){
						fscanf(f,"%s %s",name,veri);
						printf("Found map: %s v%s\n",name,veri);
						if(!strcmp(name,mapn)){
							exists=!strcmp(veri,ver);
							break;
						}
					}
					fclose(f);
				}
			}
            if(!exists){
                printf("Downloading map %s...\n",mapn);
				char params[5120];
				sprintf_s(params,"\"%s\" \"%s\" \"%s\"",omapn,mapdl,path);
				SHELLEXECUTEINFOA info;
				ZeroMemory(&info,sizeof(SHELLEXECUTEINFOA));
				info.lpDirectory=cwd;
				info.lpParameters=params;
				info.lpFile="MapDownloader.exe";
				info.nShow=SW_SHOW;
				info.cbSize=sizeof(SHELLEXECUTEINFOA);
				info.fMask=SEE_MASK_NOCLOSEPROCESS;
				info.hwnd=0;
				if (!ShellExecuteExA(&info)) {
					printf("\nFailed to start map downloader, error code %d\n", GetLastError());
					while (getchar()!='\n') {}
					return 2;
				}
				WaitForSingleObject(info.hProcess,INFINITE);
				DWORD exitCode;
				if (GetExitCodeProcess(info.hProcess, &exitCode)) {
					if (exitCode != 0) {
						printf("\nFailed to download map, error code: %d\n", (int)exitCode);
						while (getchar()!='\n') {}
						return 2;
					}
				}
            }
            system(cmd);
        }
        if(usePwd)
            sprintf_s(cmd,"start %s -mod sfwcl +auth_id \"%s\" +auth_uid \"%s\" +auth_name \"%s\" +auth_pwd \"%s\" +auth_conn \"%s:%s\"",gm,profid,uid,name,pwd,ip,port);
        else sprintf_s(cmd,"start %s -mod sfwcl +auth_id \"%s\" +auth_uid \"%s\" +auth_name \"%s\" +auth_conn \"%s:%s\"",gm,profid,uid,name,ip,port);
		printf("Launching game...\n");
        system(cmd);
        return 0;
    }
}
