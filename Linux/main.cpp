#include <stdio.h>
#include <dlfcn.h>
#include <string.h>
#include <limits.h>

#include <unordered_map>
#include <string_view>

#include "types.h"
#include "funchook.h"
#include "utils.h"

typedef struct
{
    char *name;
    char *string;
    int flags;
    float value;
    void *next;
} cvar_t;

static_assert(sizeof(cvar_t) == 0x14, "wrong size for cvar_t");

typedef struct
{
    void *hook;
    cvar_t *cvar;
    void *next;
} cvarhook_t;

static_assert(sizeof(cvarhook_t) == 0xC, "wrong size for cvarhook_t");

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
typedef bool (*_Cvar_HookVariable)(char *var_name, cvarhook_t *pHook);
typedef void (*R_Init)();
typedef void (*Cmd_ExecuteStringWithPrivilegeCheck)(const char *text, int bIsPrivileged, int src);
typedef void (*_Cmd_ExecuteString)(char* text, int src);
typedef void (*PlayStartupSequence)(void *_this);

ConnectToServer orig_ConnectToServer = nullptr;
SaveGameSlot orig_SaveGameSlot = nullptr;
R_BuildLightMap orig_R_BuildLightMap = nullptr;
_dlopen orig_dlopen = nullptr;
_GetInteralCDAudio GetInteralCDAudio = nullptr; // "Interal" is a typo on valve's part
Host_Version_f orig_Host_Version_f = nullptr;
_Con_Printf Con_Printf = nullptr;
Q_strncmp orig_Q_strncmp = nullptr;
_SetEngineDLL SetEngineDLL = nullptr;
_Cvar_HookVariable Cvar_HookVariable = nullptr;
R_Init orig_R_Init = nullptr;
Cmd_ExecuteStringWithPrivilegeCheck orig_Cmd_ExecuteStringWithPrivilegeCheck = nullptr;
_Cmd_ExecuteString Cmd_ExecuteString = nullptr;
PlayStartupSequence orig_PlayStartupSequence = nullptr;

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
bool fixStartupVideoMusic = true;
bool isPreAnniversary = false;
bool finishedStartupVideos = false;

std::unordered_map<std::string_view, u32> engineSymbols;
std::unordered_map<std::string_view, u32> gameuiSymbols;
std::unordered_map<std::string_view, u32> launcherSymbols;

funchook_t *engineFunchook;
funchook_t *dlopenFunchook;
funchook_t *engineExecuteFunchook;

u32 getEngineSymbol(const char *symbol)
{
    return engineSymbols[symbol] + engineAddr;
}

bool hasEngineSymbol(const char *symbol)
{
    return engineSymbols.find(symbol) != engineSymbols.end();
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

void gl_use_shaders_callback(cvar_t *cvar)
{
    if (cvar->value >= 0.99f)
        *gl_texsort = false;
    else
        *gl_texsort = true;
}

cvarhook_t gl_use_shaders_hook = {
    (void *)gl_use_shaders_callback,
    nullptr,
    nullptr,
};

void hooked_R_Init()
{
    orig_R_Init();
    Cvar_HookVariable("gl_use_shaders", &gl_use_shaders_hook);
}

void hooked_Cmd_ExecuteStringWithPrivilegeCheck(const char *text, int bIsPrivileged, int src)
{
    if (!finishedStartupVideos && strcmp(text, "mp3 loop media/gamestartup.mp3") == 0)
       return;
    orig_Cmd_ExecuteStringWithPrivilegeCheck(text, bIsPrivileged, src);
}

void hooked_PlayStartupSequence(void *_this)
{
    orig_PlayStartupSequence(_this);

    if (!finishedStartupVideos)
    {
        finishedStartupVideos = true;
        orig_Cmd_ExecuteStringWithPrivilegeCheck("mp3 loop media/gamestartup.mp3", true, 1);
        Cmd_ExecuteString("mp3 loop media/gamestartup.mp3", 1);
        funchook_uninstall(engineExecuteFunchook, 0);
        funchook_destroy(engineExecuteFunchook);
    }
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
    if (engine)
    {
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
        if (void *handle = dlopen(engineDLL, RTLD_NOW | RTLD_NOLOAD))
        {
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
                orig_R_Init = (R_Init)getEngineSymbol("R_Init");
            gl_texsort = (bool *)getEngineSymbol("gl_texsort");
            GetInteralCDAudio = (_GetInteralCDAudio)getEngineSymbol("_Z17GetInteralCDAudiov");
            orig_Host_Version_f = (Host_Version_f)getEngineSymbol("Host_Version_f");
            Con_Printf = (_Con_Printf)getEngineSymbol("Con_Printf");
            orig_Q_strncmp = (Q_strncmp)getEngineSymbol("Q_strncmp");
            orig_dlopen = dlopen;
            Cvar_HookVariable = (_Cvar_HookVariable)getEngineSymbol("Cvar_HookVariable");
            orig_Cmd_ExecuteStringWithPrivilegeCheck = (Cmd_ExecuteStringWithPrivilegeCheck)getEngineSymbol("Cmd_ExecuteStringWithPrivilegeCheck.part.3");
            Cmd_ExecuteString = (_Cmd_ExecuteString)getEngineSymbol("Cmd_ExecuteString");
            orig_PlayStartupSequence = (PlayStartupSequence)getEngineSymbol("_ZN17CVideoMode_Common19PlayStartupSequenceEv");

            if (isHW)
                isPreAnniversary = !hasEngineSymbol("R_UsingShaders");

            if (fixMusic)
            {
                dlopenFunchook = funchook_create();
                funchook_prepare(dlopenFunchook, (void **)&orig_dlopen, (void *)hooked_dlopen);
                funchook_install(dlopenFunchook, 0);
            }

            if (fixSaves)
                funchook_prepare(engineFunchook, (void **)&orig_SaveGameSlot, (void *)hooked_SaveGameSlot);

            if (isHW && fixOverbright)
            {
                if (isPreAnniversary)
                {
                    funchook_prepare(engineFunchook, (void **)&orig_R_BuildLightMap, (void *)hooked_R_BuildLightMap);
                }
                else
                {
                    funchook_prepare(engineFunchook, (void **)&orig_R_Init, (void *)hooked_R_Init);
                }
            }

            if (isHW && fixSky)
            {
                u32 addr_R_NewMap = getEngineSymbol("R_NewMap");
                u32 offset = isPreAnniversary ? 0x17B : 0x199;
                if (orig_Q_strncmp && *(u8 *)(addr_R_NewMap + offset) == 0xE8)
                {
                    if (RelativeToAbsolute(*(u32 *)(addr_R_NewMap + offset + 1), addr_R_NewMap + offset + 5) == (u32)orig_Q_strncmp)
                    {
                        u32 addr = AbsoluteToRelative((u32)hooked_Q_strncmp, addr_R_NewMap + offset + 5);
                        MakePatch(addr_R_NewMap + offset + 1, (u8 *)&addr, 4);
                    }
                }
            }

            if (!isPreAnniversary && fixStartupVideoMusic)
            {
                engineExecuteFunchook = funchook_create();
                funchook_prepare(engineExecuteFunchook, (void **)&orig_Cmd_ExecuteStringWithPrivilegeCheck, (void *)hooked_Cmd_ExecuteStringWithPrivilegeCheck);
                funchook_install(engineExecuteFunchook, 0);

                funchook_prepare(engineFunchook, (void **)&orig_PlayStartupSequence, (void *)hooked_PlayStartupSequence);
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
    fixStartupVideoMusic = !checkArg("--no-startup-video-music-fix", argc, argv);

    return 0;
}

__attribute__((destructor)) static void uninit()
{
    funchook_uninstall(engineFunchook, 0);
    funchook_destroy(engineFunchook);
    dlclose(engine);
}

__attribute__((section(".init_array"))) static void *ctr = (void *)&init;