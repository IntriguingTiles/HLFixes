#include <string_view>
#include <Windows.h>
#include <MinHook.h>
#include "types.h"
#include "hookutil.h"

using namespace std::literals::string_view_literals;

struct {
	std::string_view ConnectToServer = "\x56\x8B\xF1\x8B\x0D\x2A\x2A\x2A\x2A\x57\x85\xC9"sv;
	std::string_view SaveGameSlot = "\x55\x8B\xEC\x81\xEC\x78\x02\x00\x00"sv;
	std::string_view R_BuildLightMap = "\x55\x8B\xEC\x83\xEC\x1C\xD9\x05\x2A\x2A\x2A\x2A\xD8\x1D\x2A\x2A\x2A\x2A\xDF\xE0"sv;
} sigs;

typedef int(__fastcall* ConnectToServer)(void* _this, void* edx, const char* game, int b, int c);
typedef bool(__cdecl* SaveGameSlot)(char* save, char* comment);
typedef void* (__cdecl* _CreateInterface)(const char* name, u32* b);
typedef int(__cdecl* R_BuildLightMap)(int a1, int a2, int a3);
typedef HMODULE(WINAPI* _LoadLibraryA)(LPCSTR lpLibFileName);

ConnectToServer orig_ConnectToServer = nullptr;
SaveGameSlot orig_SaveGameSlot = nullptr;
R_BuildLightMap orig_R_BuildLightMap = nullptr;
_LoadLibraryA orig_LoadLibraryA = nullptr;

u32 addr_R_BuildLightMap = 0;

int __fastcall hooked_ConnectToServer(void* _this, void* edx, const char* game, int b, int c) {
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

HMODULE WINAPI hooked_LoadLibraryA(LPCSTR lpLibFileName) {
	auto ret = orig_LoadLibraryA(lpLibFileName);

	if (strstr(lpLibFileName, "GameUI.dll")) {
		MakeHook("GameUI.dll", sigs.ConnectToServer, hooked_ConnectToServer, (void**)&orig_ConnectToServer);
	}

	return ret;
}

extern "C" __declspec(dllexport) void* __cdecl CreateInterface(const char* name, u32 * b) {
	auto addr = GetProcAddress(GetModuleHandle("hw.dll"), "CreateInterface");
	return ((_CreateInterface)(addr))(name, b);
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
	switch (ul_reason_for_call) {
	case DLL_PROCESS_ATTACH:
	{
		LoadLibrary("hw.dll");

		if (strstr(GetCommandLine(), "--no-fixes") == 0) {
			MH_Initialize();
			MakeHook(LoadLibraryA, hooked_LoadLibraryA, (void**)&orig_LoadLibraryA);
			MakeHook("hw.dll", sigs.SaveGameSlot, hooked_SaveGameSlot, (void**)&orig_SaveGameSlot);
			addr_R_BuildLightMap = FindSig("hw.dll", sigs.R_BuildLightMap);
			MakeHook("hw.dll", sigs.R_BuildLightMap, hooked_R_BuildLightMap, (void**)&orig_R_BuildLightMap);
		}
		break;
	}
	case DLL_PROCESS_DETACH:
		FreeLibrary(GetModuleHandle("hw.dll"));

		if (strstr(GetCommandLine(), "--no-fixes") == 0) {
			MH_Uninitialize();
		}
		break;
	}
	return TRUE;
}

