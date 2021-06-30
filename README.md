# HLFixes
This is a hack for GoldSrc that aims to fix some bugs.

## Bugs fixed
- gl_overbright not functioning
- Music stopping when a level transition occurs
- Quick save history not functioning

## Installing
0. Make sure Half-Life isn't running
1. Download the latest release from the [releases page](https://github.com/IntriguingTiles/HLFixes/releases)
2. Extract it somewhere
3. Run `Installer.exe`
4. Click on `Install`
5. If all goes well, the fixes should be applied

## How do I know if it's working?
Make a couple quick saves and then run `load quick01` in console. It should load one of the quick saves you just made.

## How does it work?
The installer patches the Half-Life launcher to load HLFixes instead of the engine. HLFixes then loads the engine and applies the fixes.