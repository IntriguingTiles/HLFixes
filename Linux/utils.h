#pragma once

#include <unordered_map>
#include <string_view>

#include "types.h"

void readSymbols(const char *lib, std::unordered_map<std::string, u32> &symbols);
bool getModuleAddress(const char *lib, u32* startOfModule = nullptr, u32* endOfModule = nullptr);
bool checkArg(const char *arg, int argc, char **argv);
u32 RelativeToAbsolute(u32 relAddr, u32 nextInstructionAddr);
u32 AbsoluteToRelative(u32 absAddr, u32 nextInstructionAddr);
u32 FindSig(u32 start, u32 end, u8 sig[], u32 siglen);
u32 FindSig(const char* dll, std::string_view sig);
void MakePatch(u32 addr, u8 patch[], u32 patchlen);
void MakePatch(const char* dll, std::string_view sig, std::string_view patch);