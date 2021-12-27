# xsnip - a minimal selection based screenshot utility for X11
Most screenshot utilities compatible with X are clumsy, use bloated toolkits, and often do not support selection regions or the clipboard. 
xsnip seeks to streamline the screenshot process in a number of ways. 
xsnip is compatible with any window manager, has no graphical interface, has automatic clipboard support, and is entirely based on selection regions over specific windows. 
While most screenshot tools require either a cli flag or a button to save images to the clipboard, xsnip selections made with the left mouse button will automatically be copied.

## Installation
xsnip requires xclip for clipboard support, which can usually be installed via your system package manager.

With xclip installed and repo cloned:

`make`

`make install` 

## Usage
Click and drag anywhere on the screen to make a selection.

A selection made with the left mouse button will be automatically copied to the clipboard.

A selection made with the right mouse button will be saved under ~/Pictures by default.

It is recommended that you assign xsnip to a keybind in your window manager for convenient use.

## Configuration
The default location for saved screenshots is ~/Pictures. To change this, change the `SAVEDIR` macro in `xsnip.c` and recompile.
