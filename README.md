# xsnip - a minimal and convenient screenshot utility for X11
Most screenshot utilities compatible with X are clumsy, use bloated toolkits, and often do not support selection regions or the clipboard. 
xsnip seeks to streamline the screenshot process in a number of ways. 
xsnip is compatible with any window manager, has no graphical interface, has automatic clipboard support, and is entirely based on selection regions over specific windows. 
While most screenshot tools require either a cli flag or a button to save images to the clipboard, xsnip selections made with the left mouse button will automatically be copied, saving time and input.

## Installation
xsnip requires xclip and libpng, both of which can usually be installed via your system package manager.

With dependencies installed and repo cloned:

`make`

`make install` 

## Usage

Click and drag anywhere on the screen to make a selection, or screenshot a window by clicking on it.

A selection made with the left mouse button will be automatically copied to the clipboard.

A selection made with the right mouse button will be saved under ~/Pictures by default.

You can exit the program without taking a screenshot with the esecape key.

It is recommended that you assign xsnip to a keybind in your window manager for convenient use.

## Configuration
The saved screenshot directory, poll rate, quit key, and cursor glyph can all be changed from their default values by editing the corresponding macros at the top of `xsnip.c` and recompiling.

If you experience strange behavior when the program is running, such as a black or blurry screen, run xsnip in opaque mode with the flag `-o`, or recompile with the `OPAQUE` macro set to `true` to set it as the default. When the default mode is opaque, you can run in transparent mode with the `-t` flag. 
You should compile in opaque mode if your window manager/compositor does not support transparency.
