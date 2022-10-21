# HLFixes
This is a hack for GoldSrc that aims to fix some bugs.

## Bugs fixed
- gl_overbright not functioning
- Music stopping when a level transition occurs
- Main menu music continuing to play while in-game
- Quick save history not functioning

## Installing
0. Make sure Half-Life isn't running
1. Download the latest release from the [releases page](https://github.com/IntriguingTiles/HLFixes/releases)
2. Extract it somewhere
3. Run `Installer.exe`
4. Click on `Install`
5. If all goes well, the fixes should be applied

## Launch options
|Option|Description|
|-|-|
|`--no-fixes`|Causes HLFixes to not apply any fixes.|
|`--no-version-check`|Causes HLFixes to not check the engine version.|

## How do I know if it's working?
Run `version` in console. If HLFixes is installed correctly, it should say `Patched with HLFixes` at the end of the output.

## How does it work?
The installer patches the Half-Life launcher to load HLFixes instead of the engine. HLFixes then loads the engine and applies the fixes.