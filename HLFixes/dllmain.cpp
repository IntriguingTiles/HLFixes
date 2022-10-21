#include <string_view>
#include <Windows.h>
#include <Shlwapi.h>
#include <MinHook.h>
#include <chrono>
#include <format>
#include "types.h"
#include "hookutil.h"

using namespace std::literals::string_view_literals;

struct {
	std::string_view ConnectToServer = "\x56\x8B\xF1\x8B\x0D\x2A\x2A\x2A\x2A\x57\x85\xC9"sv;
	std::string_view SaveGameSlot = "\x55\x8B\xEC\x81\xEC\x78\x02\x00\x00"sv;
	std::string_view R_BuildLightMap = "\x55\x8B\xEC\x83\xEC\x1C\xD9\x05\x2A\x2A\x2A\x2A\xD8\x1D\x2A\x2A\x2A\x2A\xDF\xE0"sv;
	std::string_view sub_1D08FF0 = "\xA1\x2A\x2A\x2A\x2A\x8B\x00\xC3"sv;
	std::string_view Host_Version_f = "\x68\x2A\x2A\x2A\x2A\x68\x2A\x2A\x2A\x2A\x6A\x30\x68\x2A\x2A\x2A\x2A"sv;
	std::string_view Con_Printf = "\x55\x8B\xEC\xB8\x00\x10\x00\x00\xE8\x2A\x2A\x2A\x2A\x8B\x4D\x08"sv;
} sigs;

typedef int(__fastcall* ConnectToServer)(void* _this, void* edx, const char* game, int b, int c);
typedef bool(__cdecl* SaveGameSlot)(char* save, char* comment);
typedef void* (__cdecl* _CreateInterface)(const char* name, u32* b);
typedef int(__cdecl* R_BuildLightMap)(int a1, int a2, int a3);
typedef u32(*_GetInteralCDAudio)();
typedef void(*Host_Version_f)();
typedef void(*_Con_Printf)(const char* format, ...);
typedef HMODULE(WINAPI* _LoadLibraryA)(LPCSTR lpLibFileName);

ConnectToServer orig_ConnectToServer = nullptr;
SaveGameSlot orig_SaveGameSlot = nullptr;
R_BuildLightMap orig_R_BuildLightMap = nullptr;
_LoadLibraryA orig_LoadLibraryA = nullptr;
_GetInteralCDAudio GetInteralCDAudio = nullptr; // "Interal" is a typo on valve's part
Host_Version_f orig_Host_Version_f = nullptr;
_Con_Printf Con_Printf = nullptr;

u32 addr_R_BuildLightMap = 0;

int __fastcall hooked_ConnectToServer(void* _this, void* edx, const char* game, int b, int c) {
	u32 g_CDAudio = GetInteralCDAudio();
	if (g_CDAudio) {
		char* curTrack = (char*)(g_CDAudio + 0x3D3);
		if (StrStrIA(curTrack, "media\\gamestartup.mp3") != 0) {
			// ConnectToServer runs "mp3 stop" if the game name isn't "valve"
			// take advantage of this to stop the main menu music from playing in-game
			return orig_ConnectToServer(_this, edx, "hlfixes", b, c);
		}
	}
	return orig_ConnectToServer(_this, edx, "valve", b, c);
}

bool __cdecl hooked_SaveGameSlot(char* save, char* comment) {
	if (!_stricmp(save, "quick.sav")) return orig_SaveGameSlot((char*)"quick", comment);
	else return orig_SaveGameSlot(save, comment);
}

int __cdecl hooked_R_BuildLightMap(int a1, int a2, int a3) {
	u8** gl_texsort = (u8**)(addr_R_BuildLightMap + 0x1A);
	**gl_texsort = 1;
	return orig_R_BuildLightMap(a1, a2, a3);
}

void hooked_Host_Version_f() {
	orig_Host_Version_f();
	Con_Printf("Patched with HLFixes (built " __TIME__ " " __DATE__ ")\n");
}

HMODULE WINAPI hooked_LoadLibraryA(LPCSTR lpLibFileName) {
	auto ret = orig_LoadLibraryA(lpLibFileName);

	if (StrStrIA(lpLibFileName, "GameUI.dll")) {
		MakeHook("GameUI.dll", sigs.ConnectToServer, hooked_ConnectToServer, (void**)&orig_ConnectToServer);
	}

	return ret;
}

extern "C" __declspec(dllexport) void* __cdecl CreateInterface(const char* name, u32* b) {
	auto addr = GetProcAddress(GetModuleHandle("hw.dll"), "CreateInterface");
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
		LoadLibrary("hw.dll");

		if (StrStrIA(GetCommandLine(), "--no-version-check") == 0) {
			// check version of hw.dll
			u32 base = (u32)GetModuleHandle("hw.dll");

			auto dos = (IMAGE_DOS_HEADER*)base;
			auto nt = (IMAGE_NT_HEADERS*)((u8*)dos + dos->e_lfanew);

			if (nt->FileHeader.TimeDateStamp != 1597869516) {
				// doesn't match the latest steam version
				using namespace std::chrono;
				std::string msg = "Your version of Half-Life ("
					+ std::format("{:%Y-%m-%d}", sys_seconds{ (seconds)(nt->FileHeader.TimeDateStamp) })
					+ ") has not been tested with HLFixes (expected version "
					+ std::format("{:%Y-%m-%d}", sys_seconds{ 1597869516s })
					+ "). There may be crashes or broken features.\n\n"
					+ "If HLFixes seems to work, you can silence this warning by adding \"--no-version-check\" "
					+ "to your launch options (note the two dashes at the start).";
				MessageBox(nullptr, msg.c_str(), "HLFixes", MB_ICONWARNING | MB_OK);
			}
		}

		if (StrStrIA(GetCommandLine(), "--no-fixes") == 0) {
			MH_Initialize();
			MakeHook(LoadLibraryA, hooked_LoadLibraryA, (void**)&orig_LoadLibraryA);
			MakeHook("hw.dll", sigs.SaveGameSlot, hooked_SaveGameSlot, (void**)&orig_SaveGameSlot);
			addr_R_BuildLightMap = FindSig("hw.dll", sigs.R_BuildLightMap);
			MakeHook("hw.dll", sigs.R_BuildLightMap, hooked_R_BuildLightMap, (void**)&orig_R_BuildLightMap);
			// GetInteralCDAudio is too tiny to make a unique signature for it
			// so instead we use the signature of the function above it
			GetInteralCDAudio = *(_GetInteralCDAudio)(FindSig("hw.dll", sigs.sub_1D08FF0) + 0x10);
			MakeHook("hw.dll", sigs.Host_Version_f, hooked_Host_Version_f, (void**)&orig_Host_Version_f);
			Con_Printf = (_Con_Printf)FindSig("hw.dll", sigs.Con_Printf);
		}
		break;
	}
	case DLL_PROCESS_DETACH:
		FreeLibrary(GetModuleHandle("hw.dll"));

		if (StrStrIA(GetCommandLine(), "--no-fixes") == 0) {
			MH_Uninitialize();
		}
		break;
	}
	return TRUE;
}

