#include <string_view>
#include <Windows.h>
#include <MinHook.h>
#include "types.h"
#include "hookutil.h"

using namespace std::literals::string_view_literals;

struct {
	std::string_view ConnectToServer = "\x56\x8B\xF1\x8B\x0D\x2A\x2A\x2A\x2A\x57\x85\xC9"sv;
	std::string_view Con_Printf = "\x55\x8B\xEC\xB8\x00\x10\x00\x00\xE8\x2A\x2A\x2A\x2A\x8B\x4D\x08"sv;
	std::string_view SaveGameSlot = "\x55\x8B\xEC\x81\xEC\x78\x02\x00\x00"sv;
} sigs;

typedef void(*_Con_Printf)(const char* fmt, ...);
typedef int(__fastcall* ConnectToServer)(void* _this, void* edx, const char* game, int b, int c);
typedef bool(__cdecl* SaveGameSlot)(char* save, char* comment);

_Con_Printf Con_Printf = nullptr;
ConnectToServer orig_ConnectToServer = nullptr;
SaveGameSlot orig_SaveGameSlot = nullptr;

int __fastcall hooked_ConnectToServer(void* _this, void* edx, const char* game, int b, int c) {
	return orig_ConnectToServer(_this, edx, "valve", b, c);
}

bool __cdecl hooked_SaveGameSlot(char* save, char* comment) {
	if (!_stricmp(save, "quick.sav")) return orig_SaveGameSlot((char*)"quick", comment);
	else return orig_SaveGameSlot(save, comment);
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
	switch (ul_reason_for_call) {
	case DLL_PROCESS_ATTACH:
	{
		MH_Initialize();
		Con_Printf = (_Con_Printf)FindSig("hw.dll", sigs.Con_Printf);
		Con_Printf("HLFixes: Fixing gl_overbright...\n");
		// todo: calculate this dynamically
		// it seems like goldsrc isn't getting updates anymore so this is fine for now
		u8* gl_texsort = (u8*)((u32)GetModuleHandle("hw.dll") + 0x1471C0);
		*gl_texsort = 1;
		Con_Printf("HLFixes: Fixing music...\n");
		MakeHook("GameUI.dll", sigs.ConnectToServer, hooked_ConnectToServer, (void**)&orig_ConnectToServer);
		Con_Printf("HLFixes: Fixing quick save history...\n");
		MakeHook("hw.dll", sigs.SaveGameSlot, hooked_SaveGameSlot, (void**)&orig_SaveGameSlot);
		break;
	}
	case DLL_PROCESS_DETACH:
		MH_Uninitialize();
		break;
	}
	return TRUE;
}

