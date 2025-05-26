# autorestool

`autorestool` sets your screen resolution, launches a game, then sets it back to what it was when the game closes.

## For normal people
1. Download [autorestool.exe](https://github.com/Davipb/autorestool/releases/download/latest/autorestool.exe) and place it on the same folder as the game executable
2. Create a shortcut to autorestool.exe and place it on your Desktop (right click file -> Create Shortcut)
3. Open the shortcut properties (right click -> properties) and add the following values to the **end** of the "Target" field in the "Shortcut" tab (DO NOT REPLACE what's already there, just add to the end):

```
run --width <width> --height <height> --program <program.exe>
```

Replace `<width>` with your desired screen resolution width, `<height>` with the desired height, and `<program>` with the name of the game executable.
For example, if you wanted to play Baldur's Gate 3 (`bg3_dx11.exe`) in 4K (3840x2160), your "Target" field would look something like this:

```
"C:\Program Files (x86)\Steam\steamapps\common\Baldurs Gate 3\bin\autorestool.exe" run --width 3840 --height 2160 --program bg3_dx11.exe
```

**Done!** Use this shortcut to launch the game.

### Usage notes
* By default, `autorestool` will change the resolution of your main display, as defined on the Windows settings. You can customize this, but it's a bit more involved (see the `--adapter` option in the nerd section)
* If your game needs to run as admin, you should run the tool as admin too.

## For the nerds

```
autorestool.exe <run|list> ...
```

### run

```
autorestool.exe run --width <uint> --height <uint> --program <str> [--adapter <str>] [--dir <str>]
```

* **--width**: Required, sets screen width during game execution
* **--height**: Required, sets screen height during game execution
* **--program**: Required, defines which executable will be launched (& waited for) when the tool is executed. This can be either an absolute or relative path. If it's relative, it'll be interpreted relative to the current working directory.
* **--adapter**: Optional, defines which display adapter will have its resolution changed while the game is running. Use `list` to get a list of available display adapters. This is usually in the format `\\.\DISPLAY1`, `\\.\DISPLAY2`, etc.
* **--dir**: Optional, sets the working directory of the game when it's launched. If not specified, uses the tool's own working directory.

### list

```
autorestool.exe list
```

If the first parameter is `list`, the tool will list all display adapters in a message box and exit with no further action.
This can be useful to set the `--adapter` parameter in run mode.
