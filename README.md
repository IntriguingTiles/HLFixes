# HLFixes
This is a hack for GoldSrc that aims to fix some bugs.

## Bugs fixed
- gl_overbright not functioning when gl_use_shaders is 0
- Music stopping when a level transition occurs
- Main menu music continuing to play while in-game
- Quick save history not functioning
- Broken skies in Condition Zero Deleted Scenes when texture sorting is on

## [Installation Guide](https://hgrunt.xyz/hlfixes.html#install-guide)

## Launch options
|Option|Description|
|-|-|
|`--no-fixes`|Causes HLFixes to not apply any fixes.|
|`--no-music-fix`|Causes HLFixes to not apply music fixes.|
|`--no-startup-music-fix`|Causes HLFixes to not apply the main menu music fix.|
|`--no-quicksave-fix`|Causes HLFixes to not apply the quick save history fix.|
|`--no-overbright-fix`|Causes HLFixes to not apply the overbright fix.|
|`--no-sky-fix`|Causes HLFixes to not apply the CZDS sky fix.|
|`--no-startup-video-music-fix`|Causes HLFixes to not apply the music playing during startup video fix.|

## How do I know if it's working?
Run `version` in console. If HLFixes is installed correctly, it should say `Patched with HLFixes` at the end of the output.

## How does it work?
The installer patches the Half-Life launcher to load HLFixes instead of the engine. HLFixes then loads the engine and applies the fixes.