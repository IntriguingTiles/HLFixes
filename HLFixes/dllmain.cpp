#include <string_view>
#include <Windows.h>
#include <Shlwapi.h>
#include <MinHook.h>
#include <ctime>
#include "types.h"
#include "hookutil.h"

using namespace std::literals::string_view_literals;

struct {
	std::string_view ConnectToServer = "\x55\x8B\xEC\x6A\xFF\x68\x2A\x2A\x2A\x2A\x64\xA1\x00\x00\x00\x00\x50\x81\xEC\x88\x00\x00\x00"sv;
	std::string_view SaveGameSlot = "\x55\x8B\xEC\x81\xEC\x90\x04\x00\x00"sv;
	std::string_view R_BuildLightMap = "\x55\x8B\xEC\x83\xEC\x18\x0F\x57\xC9"sv;
	std::string_view sub_1D08FF0 = "\x56\x8B\xF1\x68\xAC\x02\x00\x00"sv;
	std::string_view Host_Version_f = "\x68\x2A\x2A\x2A\x2A\x68\x2A\x2A\x2A\x2A\x6A\x30\x68\x2A\x2A\x2A\x2A"sv;
	std::string_view Con_Printf = "\x55\x8B\xEC\xB8\x04\x10\x00\x00\xE8\x2A\x2A\x2A\x2A\xA1\x2A\x2A\x2A\x2A\x33\xC5\x89\x45\xFC\x8D\x45\x0C"sv;
	std::string_view Q_strncmp = "\x55\x8B\xEC\x56\x8B\x75\x08\x85\xF6\x74\x2A\x8B\x45\x0C\x85\xC0\x74\x2A"sv;
	std::string_view R_NewMap = "\x55\x8B\xEC\x83\xEC\x08\xC7\x45\xFC\x00\x00\x00\x00"sv;
	std::string_view Cvar_HookVariable = "\x55\x8B\xEC\x56\x8B\x75\x0C\x85\xF6\x74\x2A\x83\x3E\x00"sv;
	std::string_view R_Init = "\x55\x8B\xEC\x81\xEC\x10\x03\x00\x00\xA1\x2A\x2A\x2A\x2A\x33\xC5\x89\x45\xFC\x6A\x10"sv;
	std::string_view Cmd_ExecuteStringWithPrivilegeCheck = "\x55\x8B\xEC\x83\xEC\x24\xA1\x2A\x2A\x2A\x2A\x33\xC5\x89\x45\xFC\x8B\x4D\x08"sv;
	std::string_view PlayStartupSequence = "\x56\x68\x2A\x2A\x2A\x2A\x8B\xF1\xE8\x2A\x2A\x2A\x2A\x83\xC4\x04\x85\xC0\x75\x2A"sv;
	std::string_view Host_GetMaxClients = "\x83\x3D\x2A\x2A\x2A\x2A\x00\xA1\x2A\x2A\x2A\x2A\x0F\x44\x05\x2A\x2A\x2A\x2A"sv;
} sigs;

struct {
	std::string_view ConnectToServer = "\x56\x8B\xF1\x8B\x0D\x2A\x2A\x2A\x2A\x57\x85\xC9"sv;
	std::string_view SaveGameSlot = "\x55\x8B\xEC\x81\xEC\x78\x02\x00\x00"sv;
	std::string_view R_BuildLightMap = "\x55\x8B\xEC\x83\xEC\x1C\xD9\x05\x2A\x2A\x2A\x2A\xD8\x1D\x2A\x2A\x2A\x2A\xDF\xE0"sv;
	std::string_view sub_1D08FF0 = "\xA1\x2A\x2A\x2A\x2A\x8B\x00\xC3"sv;
	std::string_view Con_Printf = "\x55\x8B\xEC\xB8\x00\x10\x00\x00\xE8\x2A\x2A\x2A\x2A\x8B\x4D\x08"sv;
	std::string_view Q_strncmp = "\x55\x8B\xEC\x8B\x55\x08\x53\x85\xD2\x56"sv;
	std::string_view Host_GetMaxClients = "\xA1\x2A\x2A\x2A\x2A\x85\xC0\xA1\x2A\x2A\x2A\x2A"sv;
} oldsigs;

typedef struct {
	char* name;
	char* string;
	int flags;
	float value;
	void* next;
} cvar_t;

static_assert(sizeof(cvar_t) == 0x14, "wrong size for cvar_t");

typedef struct {
	void* hook;
	cvar_t* cvar;
	void* next;
} cvarhook_t;

static_assert(sizeof(cvarhook_t) == 0xC, "wrong size for cvarhook_t");

typedef int(__fastcall* ConnectToServer)(void* _this, void* edx, const char* game, int b, int c);
typedef bool(__cdecl* SaveGameSlot)(char* save, char* comment);
typedef void* (__cdecl* _CreateInterface)(const char* name, u32* b);
typedef int(__cdecl* R_BuildLightMap)(int a1, int a2, int a3);
typedef u32(*_GetInteralCDAudio)();
typedef void(*Host_Version_f)();
typedef void(*_Con_Printf)(const char* format, ...);
typedef int(__cdecl* Q_strncmp)(const char* s1, const char* s2, int count);
typedef bool(__cdecl* _Cvar_HookVariable)(char* var_name, cvarhook_t* pHook); \
typedef void(*R_Init)();
typedef void(__cdecl* Cmd_ExecuteStringWithPrivilegeCheck)(const char* text, int bIsPrivileged, int src);
typedef void(__fastcall* PlayStartupSequence)(void* _this, void* edx);
typedef int(*_Host_GetMaxClients)();
typedef HMODULE(WINAPI* _LoadLibraryA)(LPCSTR lpLibFileName);

ConnectToServer orig_ConnectToServer = nullptr;
SaveGameSlot orig_SaveGameSlot = nullptr;
R_BuildLightMap orig_R_BuildLightMap = nullptr;
_LoadLibraryA orig_LoadLibraryA = nullptr;
_GetInteralCDAudio GetInteralCDAudio = nullptr; // "Interal" is a typo on valve's part
Host_Version_f orig_Host_Version_f = nullptr;
_Con_Printf Con_Printf = nullptr;
Q_strncmp orig_Q_strncmp = nullptr;
_Cvar_HookVariable Cvar_HookVariable = nullptr;
R_Init orig_R_Init = nullptr; 
Cmd_ExecuteStringWithPrivilegeCheck orig_Cmd_ExecuteStringWithPrivilegeCheck = nullptr;
PlayStartupSequence orig_PlayStartupSequence = nullptr;
_Host_GetMaxClients Host_GetMaxClients = nullptr;

u32 addr_R_BuildLightMap = 0;
u32 addr_Cmd_ExecuteStringWithPrivilegeCheck = 0;

const char* engineDLL = nullptr;
bool isHW = false;
bool fixStartupMusic = true;
bool isPreAnniversary = false;
bool finishedStartupVideos = false;
bool persistMusicInMP = false;

void ShowHookError(const char* func, const char* fix) {
	std::string error = "Failed to find signature for " + std::string(func) + ". The " + std::string(fix) + " fix will not be applied.\n\nEither your version of Half-Life is outdated, or HLFixes needs an update.";
	MessageBox(nullptr, error.c_str(), "HLFixes", MB_ICONERROR | MB_OK);
}

int __fastcall hooked_ConnectToServer(void* _this, void* edx, const char* game, int b, int c) {
	u32 g_CDAudio = GetInteralCDAudio();
	if (fixStartupMusic && g_CDAudio) {
		char* curTrack = (char*)(g_CDAudio + 0x3D3);
		if (StrStrIA(curTrack, "media\\gamestartup.mp3") != 0) {
			// ConnectToServer runs "mp3 stop" if the game name isn't "valve"
			// take advantage of this to stop the main menu music from playing in-game
			return orig_ConnectToServer(_this, edx, "hlfixes", b, c);
		}
	}

	if (!persistMusicInMP && Host_GetMaxClients && Host_GetMaxClients() > 1) {
		// use unfixed music behavior for multiplayer games
		return orig_ConnectToServer(_this, edx, game, b, c);
	}

	return orig_ConnectToServer(_this, edx, "valve", b, c);
}

bool __cdecl hooked_SaveGameSlot(char* save, char* comment) {
	if (!_stricmp(save, "quick.sav")) return orig_SaveGameSlot((char*)"quick", comment);
	else return orig_SaveGameSlot(save, comment);
}

int __cdecl hooked_Q_strncmp(const char* s1, const char* s2, int count) {
	// czero adds a texture called skycull which can break the check in R_NewMap
	// it should never be used as the actual skybox texture so let's pretend it didn't match
	if (!strncmp(s1, "skycull", 7) && !strncmp(s2, "sky", 3)) return 1;
	else return orig_Q_strncmp(s1, s2, count);
}

void hooked_Host_Version_f() {
	orig_Host_Version_f();
	Con_Printf("Patched with HLFixes (built " __TIME__ " " __DATE__ ")\n");
}

void gl_use_shaders_callback(cvar_t* cvar) {
	// silently fail if this isn't a CMP instruction
	if (*(u8*)(addr_R_BuildLightMap + 0x45) != 0x83) return;
	u8** gl_texsort = (u8**)(addr_R_BuildLightMap + 0x47);

	if (cvar->value >= 0.99f) {
		// turn off texsort since gl_use_shaders does its own overbright
		**gl_texsort = 0;
	} else {
		**gl_texsort = 1;
	}
}

cvarhook_t gl_use_shaders_hook = {
	gl_use_shaders_callback,
	nullptr,
	nullptr
};

void hooked_R_Init() {
	orig_R_Init();
	Cvar_HookVariable("gl_use_shaders", &gl_use_shaders_hook);
}

int __cdecl hooked_R_BuildLightMap(int a1, int a2, int a3) {
	u8** gl_texsort = (u8**)(addr_R_BuildLightMap + 0x1A);
	**gl_texsort = 1;
	return orig_R_BuildLightMap(a1, a2, a3);
}

void __cdecl hooked_Cmd_ExecuteStringWithPrivilegeCheck(const char* text, int bIsPrivileged, int src) {
	if (!finishedStartupVideos && strcmp(text, "mp3 loop media/gamestartup.mp3") == 0) return;
	orig_Cmd_ExecuteStringWithPrivilegeCheck(text, bIsPrivileged, src);
}

void __fastcall hooked_PlayStartupSequence(void* _this, void* edx) {
	orig_PlayStartupSequence(_this, edx);

	if (!finishedStartupVideos) {
		finishedStartupVideos = true;
		orig_Cmd_ExecuteStringWithPrivilegeCheck("mp3 loop media/gamestartup.mp3", true, 1);
		MH_RemoveHook((void*)addr_Cmd_ExecuteStringWithPrivilegeCheck);
	}
}

HMODULE WINAPI hooked_LoadLibraryA(LPCSTR lpLibFileName) {
	auto existingModule = GetModuleHandleA(lpLibFileName);
	auto ret = orig_LoadLibraryA(lpLibFileName);

	if (existingModule != ret && ret != nullptr && StrStrIA(lpLibFileName, "GameUI.dll")) {
		if (!MakeHook("GameUI.dll", sigs.ConnectToServer, hooked_ConnectToServer, (void**)&orig_ConnectToServer)) {
			ShowHookError("ConnectToServer", "music");
		}
	}

	return ret;
}

extern "C" __declspec(dllexport) void* __cdecl CreateInterface(const char* name, u32 * b) {
	auto addr = GetProcAddress(GetModuleHandle(engineDLL), "CreateInterface");
	return ((_CreateInterface)(addr))(name, b);
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
	switch (ul_reason_for_call) {
	case DLL_PROCESS_ATTACH:
	{
#ifdef _DEBUG
		AllocConsole();
		freopen_s((FILE**)stdout, "CONOUT$", "w", stdout);
#endif
		// determine which engine we're using
		char moduleName[MAX_PATH];
		GetModuleFileNameA(hModule, moduleName, MAX_PATH);
		char* dll = nullptr;
		int indexOfLastSlash = 0;
		for (size_t i = 0; i < strlen(moduleName); i++) {
			if (moduleName[i] == '\\') indexOfLastSlash = i;
		}
		dll = (moduleName + indexOfLastSlash + 1);

		if (StrStrIA(dll, "hl.fix")) {
			engineDLL = "hw.dll";
			isHW = true;
		} else {
			// assume software engine
			engineDLL = "sw.dll";
			isHW = false;
		}

		LoadLibrary(engineDLL);

		// fix up dll names within the engine
		// this allows renderer switching to function properly
		MakePatch(engineDLL, "hw.dll", "hl.fix");
		MakePatch(engineDLL, "hw.dll", "hl.fix");
		MakePatch(engineDLL, "sw.dll", "sw.fix");
		MakePatch(engineDLL, "sw.dll", "sw.fix");

		if (StrStrIA(GetCommandLine(), "--no-fixes") == 0) {
			MH_Initialize();

			auto dos = (IMAGE_DOS_HEADER*)GetModuleHandle(engineDLL);
			auto nt = (IMAGE_NT_HEADERS*)((u8*)dos + dos->e_lfanew);

			isPreAnniversary = nt->FileHeader.TimeDateStamp < 1700000000;

			if (isPreAnniversary) {
				// swap out new sigs for old sigs
				sigs.ConnectToServer = oldsigs.ConnectToServer;
				sigs.SaveGameSlot = oldsigs.SaveGameSlot;
				sigs.R_BuildLightMap = oldsigs.R_BuildLightMap;
				sigs.sub_1D08FF0 = oldsigs.sub_1D08FF0;
				sigs.Con_Printf = oldsigs.Con_Printf;
				sigs.Q_strncmp = oldsigs.Q_strncmp;
				sigs.Host_GetMaxClients = oldsigs.Host_GetMaxClients;
			}

			fixStartupMusic = StrStrIA(GetCommandLine(), "--no-startup-music-fix") == 0;

			if (StrStrIA(GetCommandLine(), "--no-music-fix") == 0) {
				// GetInteralCDAudio is too tiny to make a unique signature for it
				// so instead we use the signature of the function below it
				GetInteralCDAudio = (_GetInteralCDAudio)(FindSig(engineDLL, sigs.sub_1D08FF0) - 0x10);
				if (!GetInteralCDAudio) ShowHookError("GetInteralCDAudio", "music");

				// on pre-anniversary versions of the game, we use the function above it
				if (isPreAnniversary)
					GetInteralCDAudio = (_GetInteralCDAudio)((u32)GetInteralCDAudio + 0x20);

				MakeHook(LoadLibraryA, hooked_LoadLibraryA, (void**)&orig_LoadLibraryA);
			}

			if (StrStrIA(GetCommandLine(), "--no-quicksave-fix") == 0) {
				if (!MakeHook(engineDLL, sigs.SaveGameSlot, hooked_SaveGameSlot, (void**)&orig_SaveGameSlot)) {
					ShowHookError("SaveGameSlot", "quick save history");
				}
			}

			if (isHW && StrStrIA(GetCommandLine(), "--no-overbright-fix") == 0) {
				addr_R_BuildLightMap = FindSig(engineDLL, sigs.R_BuildLightMap);

				if (!addr_R_BuildLightMap) {
					ShowHookError("R_BuildLightMap", "overbright");
				} else {
					if (isPreAnniversary) {
						if (!MakeHook(engineDLL, sigs.R_BuildLightMap, hooked_R_BuildLightMap, (void**)&orig_R_BuildLightMap)) {
							ShowHookError("R_BuildLightMap", "overbright");
						}
					} else {
						Cvar_HookVariable = (_Cvar_HookVariable)FindSig(engineDLL, sigs.Cvar_HookVariable);

						if (!Cvar_HookVariable) {
							ShowHookError("Cvar_HookVariable", "overbright");
						} else {
							if (!MakeHook(engineDLL, sigs.R_Init, hooked_R_Init, (void**)&orig_R_Init)) {
								ShowHookError("R_Init", "overbright");
							}
						}
					}
				}
			}

			if (isHW && StrStrIA(GetCommandLine(), "--no-sky-fix") == 0) {
				orig_Q_strncmp = (Q_strncmp)FindSig(engineDLL, sigs.Q_strncmp);
				if (!orig_Q_strncmp) {
					ShowHookError("Q_strncmp", "Condition Zero skybox");
				} else {
					// in order to avoid potential side effects of hooking Q_strncmp
					// we will instead patch the function call so that it calls us instead
					// this is super brittle so make super sure that this is the right place to patch
					u32 addr_R_NewMap = FindSig(engineDLL, sigs.R_NewMap);
					u32 offset = isPreAnniversary ? 0x11F : 0x11D;
					if (addr_R_NewMap && *(u8*)(addr_R_NewMap + offset) == 0xE8) {
						// found the start of a call, so far so good
						if (RelativeToAbsolute(*(u32*)(addr_R_NewMap + offset + 1), addr_R_NewMap + offset + 5) == (u32)orig_Q_strncmp) {
							// and the call goes to Q_strncmp, probably safe to patch now
							// patches are done byte-by-byte so the address needs to be in little endian
							u32 addr = AbsoluteToRelative((u32)hooked_Q_strncmp, addr_R_NewMap + offset + 5);
							MakePatch((void*)(addr_R_NewMap + offset + 1), (u8*)&addr, 4);
						} else {
							ShowHookError("R_NewMap Part 2", "Condition Zero skybox");
						}
					} else {
						ShowHookError("R_NewMap", "Condition Zero skybox");
					}
				}
			}

			if (!isPreAnniversary && StrStrIA(GetCommandLine(), "--no-startup-video-music-fix") == 0) {
				addr_Cmd_ExecuteStringWithPrivilegeCheck = FindSig(engineDLL, sigs.Cmd_ExecuteStringWithPrivilegeCheck);
				if (!MakeHook(engineDLL, sigs.Cmd_ExecuteStringWithPrivilegeCheck, hooked_Cmd_ExecuteStringWithPrivilegeCheck, (void**)&orig_Cmd_ExecuteStringWithPrivilegeCheck)) {
					ShowHookError("Cmd_ExecuteStringWithPrivilegeCheck", "music during startup videos");
				} else if (!MakeHook(engineDLL, sigs.PlayStartupSequence, hooked_PlayStartupSequence, (void**)&orig_PlayStartupSequence)) {
					ShowHookError("PlayStartupSequence", "music during startup videos");
				}
			}

			persistMusicInMP = StrStrIA(GetCommandLine(), "--persist-music-in-mp") != 0;

			if (!persistMusicInMP) {
				Host_GetMaxClients = (_Host_GetMaxClients)FindSig(engineDLL, sigs.Host_GetMaxClients);
				if (!Host_GetMaxClients) ShowHookError("Host_GetMaxClients", "music persisting in multiplayer");
			}

			if (StrStrIA(GetCommandLine(), "--no-multitexture") != 0) {
				MakePatch(engineDLL, "GL_ARB_multitexture", "FIX_YOUR_DRIVER_AMD");
				MakePatch(engineDLL, "GL_SGIS_multitexture", "FIX_YOUR_DRIVER_AMD");
			}

			MakeHook(engineDLL, sigs.Host_Version_f, hooked_Host_Version_f, (void**)&orig_Host_Version_f);
			Con_Printf = (_Con_Printf)FindSig(engineDLL, sigs.Con_Printf);
		}
		break;
	}
	case DLL_PROCESS_DETACH:
		FreeLibrary(GetModuleHandle(engineDLL));

		if (StrStrIA(GetCommandLine(), "--no-fixes") == 0) {
			MH_Uninitialize();
		}
		break;
	}
	return TRUE;
}

