#pragma once

#include <unordered_map>
#include <string_view>

#include "types.h"

void readSymbols(const char *lib, std::unordered_map<std::string_view, u32> &symbols);
u32 getModuleAddress(const char *lib);
bool checkArg(const char *arg, int argc, char **argv);
u32 RelativeToAbsolute(u32 relAddr, u32 nextInstructionAddr);
u32 AbsoluteToRelative(u32 absAddr, u32 nextInstructionAddr);
void MakePatch(u32 addr, u8 patch[], u32 patchlen);