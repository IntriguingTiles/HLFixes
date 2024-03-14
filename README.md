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
|`--persist-music-in-mp`|Allows music to continue playing between multiplayer level changes.|

## How do I know if it's working?
Run `version` in console. If HLFixes is installed correctly, it should say `Patched with HLFixes` at the end of the output.

## Can I use this in my mod?
Absolutely! The HLFixes installer produces three binaries that you'll want to ship with your mod:

1. `hl.exe` - the patched version of the launcher that will load HLFixes
2. `hl.fix` - the hardware renderer variant of HLFixes
3. `sw.fix` - the software renderer variant of HLFixes

HLFixes is licensed under the MIT license, so you will also need to include the full text of the [license](https://github.com/IntriguingTiles/HLFixes/blob/master/LICENSE) somewhere in your mod's files. This is not optional. I suggest putting the license in a file called `LICENSE_HLFixes.txt` but you can name it whatever you want.

Note that this section is only relevant if you're making a standalone mod (i.e. a mod on Steam).