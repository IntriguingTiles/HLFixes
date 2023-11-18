#include <stdio.h>
#include <dlfcn.h>
#include <string.h>
#include <limits.h>

#include <unordered_map>
#include <string_view>

#include "types.h"
#include "funchook.h"
#include "utils.h"

typedef int (*ConnectToServer)(void *_this, const char *game, int b, int c);
typedef bool (*SaveGameSlot)(char *save, char *comment);
typedef void *(*_CreateInterface)(const char *name, u32 *b);
typedef int (*R_BuildLightMap)(int a1, int a2, int a3);
typedef u32 (*_GetInteralCDAudio)();
typedef void (*Host_Version_f)();
typedef void (*_Con_Printf)(const char *format, ...);
typedef int (*Q_strncmp)(const char *s1, const char *s2, int count);
typedef void *(*_dlopen)(const char *__file, int __mode);
typedef void (*_SetEngineDLL)(const char **ppEngineDLL);

ConnectToServer orig_ConnectToServer = nullptr;
SaveGameSlot orig_SaveGameSlot = nullptr;
R_BuildLightMap orig_R_BuildLightMap = nullptr;
_dlopen orig_dlopen = nullptr;
_GetInteralCDAudio GetInteralCDAudio = nullptr; // "Interal" is a typo on valve's part
Host_Version_f orig_Host_Version_f = nullptr;
_Con_Printf Con_Printf = nullptr;
Q_strncmp orig_Q_strncmp = nullptr;
_SetEngineDLL SetEngineDLL = nullptr;

bool *gl_texsort = nullptr;
void *engine = nullptr;
u32 engineAddr = 0;
u32 gameuiAddr = 0;
u32 launcherAddr = 0;

bool noFixes = false;
bool fixSaves = true;
bool fixOverbright = true;
bool fixMusic = true;
bool fixStartupMusic = true;
bool fixSky = true;

std::unordered_map<std::string_view, u32> engineSymbols;
std::unordered_map<std::string_view, u32> gameuiSymbols;
std::unordered_map<std::string_view, u32> launcherSymbols;

funchook_t *engineFunchook;
funchook_t *dlopenFunchook;

u32 getEngineSymbol(const char *symbol)
{
    return engineSymbols[symbol] + engineAddr;
}

u32 getGameUISymbol(const char *symbol)
{
    return gameuiSymbols[symbol] + gameuiAddr;
}

u32 getLauncherSymbol(const char *symbol)
{
    return launcherSymbols[symbol] /*+ launcherAddr*/;
}

int hooked_ConnectToServer(void *_this, const char *game, int b, int c)
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

bool hooked_SaveGameSlot(char *save, char *comment)
{
    if (!strcasecmp(save, "quick.sav"))
        return orig_SaveGameSlot((char *)"quick", comment);
    else
        return orig_SaveGameSlot(save, comment);
}

int hooked_R_BuildLightMap(int a1, int a2, int a3)
{
    *gl_texsort = true;
    return orig_R_BuildLightMap(a1, a2, a3);
}

int hooked_Q_strncmp(const char *s1, const char *s2, int count)
{
    if (!strncmp(s1, "skycull", 7) && !strncmp(s2, "sky", 3))
        return 1;
    else
        return orig_Q_strncmp(s1, s2, count);
}

void hooked_Host_Version_f()
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
        readSymbols(buf, gameuiSymbols);
        getModuleAddress(buf, &gameuiAddr);
        orig_ConnectToServer = (ConnectToServer)getGameUISymbol("_ZN7CGameUI15ConnectToServerEPKcii");
        funchook_prepare(funchook, (void **)&orig_ConnectToServer, (void *)hooked_ConnectToServer);
        funchook_install(funchook, 0);

        // get rid of the dlopen hook to avoid future issues
        funchook_uninstall(dlopenFunchook, 0);
        funchook_destroy(dlopenFunchook);
    }

    return ret;
}

extern "C" void *CreateInterface(const char *name, u32 *b)
{
    if (engine) {
        // unload the engine
        dlclose(engine);
        engine = nullptr;
    }

    if (!engine)
    {
        // anniversary update of the game made the symbols private so we can't dlsym them anymore

        // figure out what renderer we're using
        readSymbols("hl_linux", launcherSymbols);
        getModuleAddress("hl_linux", &launcherAddr);
        const char *dll;
        bool isHW = true;
        const char *engineDLL = "hw.so";
        SetEngineDLL = (_SetEngineDLL)getLauncherSymbol("_Z12SetEngineDLLPPKc");
        SetEngineDLL(&dll);

        if (!strstr(dll, "hl.fx"))
        {
            // something other than hardware mode
            isHW = false;
            engineDLL = "sw.so";
        }

        // unload engine if already loaded (which should never happen here)
        if (void* handle = dlopen(engineDLL, RTLD_NOW | RTLD_NOLOAD)) {
            dlclose(handle);
        }

        engine = dlopen(engineDLL, RTLD_NOW);
        readSymbols(engineDLL, engineSymbols);
        getModuleAddress(engineDLL, &engineAddr);

        MakePatch(engineDLL, "hw.so", "hl.fx");
        MakePatch(engineDLL, "sw.so", "sw.fx");

        if (!noFixes)
        {
            engineFunchook = funchook_create();

            orig_SaveGameSlot = (SaveGameSlot)getEngineSymbol("SaveGameSlot");
            if (isHW)
                orig_R_BuildLightMap = (R_BuildLightMap)getEngineSymbol("R_BuildLightMap");
            gl_texsort = (bool *)getEngineSymbol("gl_texsort");
            GetInteralCDAudio = (_GetInteralCDAudio)getEngineSymbol("_Z17GetInteralCDAudiov");
            orig_Host_Version_f = (Host_Version_f)getEngineSymbol("Host_Version_f");
            Con_Printf = (_Con_Printf)getEngineSymbol("Con_Printf");
            orig_Q_strncmp = (Q_strncmp)getEngineSymbol("Q_strncmp");
            orig_dlopen = dlopen;

            if (fixMusic) {
                dlopenFunchook = funchook_create();
                funchook_prepare(dlopenFunchook, (void **)&orig_dlopen, (void *)hooked_dlopen);
                funchook_install(dlopenFunchook, 0);
            }
            if (fixSaves)
                funchook_prepare(engineFunchook, (void **)&orig_SaveGameSlot, (void *)hooked_SaveGameSlot);
            if (isHW && fixOverbright)
                funchook_prepare(engineFunchook, (void **)&orig_R_BuildLightMap, (void *)hooked_R_BuildLightMap);

            if (isHW && fixSky)
            {
                u32 addr_R_NewMap = getEngineSymbol("R_NewMap");
                if (orig_Q_strncmp && *(u8 *)(addr_R_NewMap + 0x17B) == 0xE8)
                {
                    if (RelativeToAbsolute(*(u32 *)(addr_R_NewMap + 0x17C), addr_R_NewMap + 0x17B + 5) == (u32)orig_Q_strncmp)
                    {
                        u32 addr = AbsoluteToRelative((u32)hooked_Q_strncmp, addr_R_NewMap + 0x17B + 5);
                        MakePatch(addr_R_NewMap + 0x17C, (u8 *)&addr, 4);
                    }
                }
            }

            funchook_prepare(engineFunchook, (void **)&orig_Host_Version_f, (void *)hooked_Host_Version_f);
            funchook_install(engineFunchook, 0);
        }
    }

    auto addr = (_CreateInterface)dlsym(engine, "CreateInterface");
    return addr(name, b);
}

static int init(int argc, char **argv, char **env)
{
    noFixes = checkArg("--no-fixes", argc, argv);
    fixMusic = !checkArg("--no-music-fix", argc, argv);
    fixStartupMusic = !checkArg("--no-startup-music-fix", argc, argv);
    fixSaves = !checkArg("--no-quicksave-fix", argc, argv);
    fixOverbright = !checkArg("--no-overbright-fix", argc, argv);
    fixSky = !checkArg("--no-sky-fix", argc, argv);

    return 0;
}

__attribute__((destructor)) static void uninit()
{
    funchook_uninstall(engineFunchook, 0);
    funchook_destroy(engineFunchook);
    dlclose(engine);
}

__attribute__((section(".init_array"))) static void *ctr = (void *)&init;