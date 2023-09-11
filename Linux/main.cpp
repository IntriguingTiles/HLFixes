#include <stdio.h>
#include <dlfcn.h>
#include <string.h>
#include <limits.h>
#include <sys/mman.h>
#include <errno.h>
#include <unistd.h>
#include "types.h"
#include "funchook.h"

#define CDECL __attribute__((__cdecl__))

typedef int(CDECL *ConnectToServer)(void *_this, const char *game, int b, int c);
typedef bool(CDECL *SaveGameSlot)(char *save, char *comment);
typedef void *(CDECL *_CreateInterface)(const char *name, u32 *b);
typedef int(CDECL *R_BuildLightMap)(int a1, int a2, int a3);
typedef u32 (*_GetInteralCDAudio)();
typedef void(CDECL *Host_Version_f)();
typedef void (*_Con_Printf)(const char *format, ...);
typedef int(CDECL *Q_strncmp)(const char *s1, const char *s2, int count);
typedef void *(*_dlopen)(const char *__file, int __mode);

ConnectToServer orig_ConnectToServer = nullptr;
SaveGameSlot orig_SaveGameSlot = nullptr;
R_BuildLightMap orig_R_BuildLightMap = nullptr;
_dlopen orig_dlopen = nullptr;
_GetInteralCDAudio GetInteralCDAudio = nullptr; // "Interal" is a typo on valve's part
Host_Version_f orig_Host_Version_f = nullptr;
_Con_Printf Con_Printf = nullptr;
Q_strncmp orig_Q_strncmp = nullptr;

bool *gl_texsort = nullptr;
void *hw = nullptr;

bool fixSaves = true;
bool fixOverbright = true;
bool fixMusic = true;
bool fixStartupMusic = true;
bool fixSky = true;

int CDECL hooked_ConnectToServer(void *_this, const char *game, int b, int c)
{
    u32 g_CDAudio = GetInteralCDAudio();
    if (g_CDAudio && fixStartupMusic)
    {
        // offset seems to be the same as on windows
        char *curTrack = (char *)(g_CDAudio + 0x3D3);
        if (strcasestr(curTrack, "media/gamestartup.mp3"))
        {
            // ConnectToServer runs "mp3 stop" if the game name isn't "valve"
            // take advantage of this to stop the main menu music from playing in-game
            return orig_ConnectToServer(_this, "hlfixes", b, c);
        }
    }
    return orig_ConnectToServer(_this, "valve", b, c);
}

bool CDECL hooked_SaveGameSlot(char *save, char *comment)
{
    if (!strcasecmp(save, "quick.sav"))
        return orig_SaveGameSlot((char *)"quick", comment);
    else
        return orig_SaveGameSlot(save, comment);
}

int CDECL hooked_R_BuildLightMap(int a1, int a2, int a3)
{
    *gl_texsort = true;
    return orig_R_BuildLightMap(a1, a2, a3);
}

int CDECL hooked_Q_strncmp(const char *s1, const char *s2, int count)
{
    if (!strncmp(s1, "skycull", 7) && !strncmp(s2, "sky", 3))
        return 1;
    else
        return orig_Q_strncmp(s1, s2, count);
}

void CDECL hooked_Host_Version_f()
{
    orig_Host_Version_f();
    Con_Printf("Patched with HLFixes (built " __TIME__ " " __DATE__ ")\n");
}

extern "C" void *hooked_dlopen(const char *__file, int __mode)
{
    auto ret = orig_dlopen(__file, __mode);

    // need to do this to avoid a crash
    char buf[PATH_MAX];
    sprintf(buf, "%s", __file);

    if (strstr(buf, "gameui.so"))
    {
        funchook_t *funchook = funchook_create();
        orig_ConnectToServer = (ConnectToServer)dlsym(ret, "_ZN7CGameUI15ConnectToServerEPKcii");
        funchook_prepare(funchook, (void **)&orig_ConnectToServer, (void *)hooked_ConnectToServer);
        funchook_install(funchook, 0);
    }

    return ret;
}

extern "C" void *CDECL CreateInterface(const char *name, u32 *b)
{
    auto addr = (_CreateInterface)dlsym(hw, "CreateInterface");
    return addr(name, b);
}

bool checkArg(const char *arg, int argc, char **argv)
{
    for (int i = 0; i < argc; i++)
    {
        if (strcmp(argv[i], arg) == 0)
            return true;
    }

    return false;
}

u32 RelativeToAbsolute(u32 relAddr, u32 nextInstructionAddr)
{
    return relAddr + nextInstructionAddr;
}

u32 AbsoluteToRelative(u32 absAddr, u32 nextInstructionAddr)
{
    return absAddr - nextInstructionAddr;
}

void MakePatch(u32 addr, u8 patch[], u32 patchlen)
{
    int pageSize = sysconf(_SC_PAGESIZE);
    int pageDiff = addr % pageSize;
    mprotect((void *)(addr - pageDiff), patchlen + pageDiff, PROT_READ | PROT_WRITE);

    for (u32 i = 0; i < patchlen; i++)
    {
        u8 *byte = (u8 *)((u32)addr + i);
        *byte = patch[i];
    }

    mprotect((void *)(addr - pageDiff), patchlen + pageDiff, PROT_READ | PROT_EXEC);
}

static int init(int argc, char **argv, char **env)
{
    fixMusic = !checkArg("--no-music-fix", argc, argv);
    fixStartupMusic = !checkArg("--no-startup-music-fix", argc, argv);
    fixSaves = !checkArg("--no-quicksave-fix", argc, argv);
    fixOverbright = !checkArg("--no-overbright-fix", argc, argv);
    fixSky = !checkArg("--no-sky-fix", argc, argv);

    hw = dlopen("hw.so", RTLD_NOW);

    if (!checkArg("--no-fixes", argc, argv))
    {
        funchook_t *funchook = funchook_create();

        orig_SaveGameSlot = (SaveGameSlot)dlsym(hw, "SaveGameSlot");
        orig_R_BuildLightMap = (R_BuildLightMap)dlsym(hw, "R_BuildLightMap");
        gl_texsort = (bool *)dlsym(hw, "gl_texsort");
        GetInteralCDAudio = (_GetInteralCDAudio)dlsym(hw, "_Z17GetInteralCDAudiov");
        orig_Host_Version_f = (Host_Version_f)dlsym(hw, "Host_Version_f");
        Con_Printf = (_Con_Printf)dlsym(hw, "Con_Printf");
        orig_Q_strncmp = (Q_strncmp)dlsym(hw, "Q_strncmp");
        orig_dlopen = dlopen;

        if (fixMusic)
            funchook_prepare(funchook, (void **)&orig_dlopen, (void *)hooked_dlopen);
        if (fixSaves)
            funchook_prepare(funchook, (void **)&orig_SaveGameSlot, (void *)hooked_SaveGameSlot);
        if (fixOverbright)
            funchook_prepare(funchook, (void **)&orig_R_BuildLightMap, (void *)hooked_R_BuildLightMap);

        if (fixSky)
        {
            u32 addr_R_NewMap = (u32)dlsym(hw, "R_NewMap");
            if (orig_Q_strncmp && *(u8 *)(addr_R_NewMap + 0x17B) == 0xE8)
            {
                if (RelativeToAbsolute(*(u32 *)(addr_R_NewMap + 0x17C), addr_R_NewMap + 0x17B + 5) == (u32)orig_Q_strncmp)
                {
                    u32 addr = AbsoluteToRelative((u32)hooked_Q_strncmp, addr_R_NewMap + 0x17B + 5);
                    MakePatch(addr_R_NewMap + 0x17C, (u8 *)&addr, 4);
                }
            }
        }

        funchook_prepare(funchook, (void **)&orig_Host_Version_f, (void *)hooked_Host_Version_f);
        funchook_install(funchook, 0);
    }
    return 0;
}

__attribute__((section(".init_array"))) static void *ctr = (void *)&init;