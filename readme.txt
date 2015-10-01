PiMenu for Raspberry Pi by Squid
================================

INTRODUCTION
------------
This is a simple menu for running the various emulators I've written for the 
Raspberry Pi (MAME4ALL, PiSNES, PiFBA). It is also easily configurable to add 
other emulators as required.

Only detected emulators will be displayed, by default it looks in the locations 
used by the Pi Store versions.

CONTROLS
--------
Keyboard arrow keys, return or control to start.
Joystick button 0 or start to start a game.

All controls are configurable by editing the "pimenu.cfg" file.

INSTALLATION
------------
To run PiMenu simple run the "pimenu" executable. 

    pimenu          -> Game binary
    pimenu.cfg      -> configuration file
    ICON[01234].png -> Default ICONs used for the game emulators

It will work in X-Windows or in the Console.

CONFIGURATION
-------------
You can add/edit emulators in the [General] section of the pimenu.cfg file. 

Each emulator to appear requires an "icon_command_" section pointing to the 
executable and an equivalent icon in the pimenu directory, called 
"ICON3.png" (for example the number 3 emulator). The number needs to match 
the icon and the "icon_command_". 
Optionally you can add a "icon_args_" section to pass arguments to a command.

The icons must be a PNG file of size 384x384 with an alpha mask.

You can run bash scripts, example here to poweroff the Pi:

    icon_command_3=/usr/games/poweroff.sh
    icon_args_3=

The "kioskmode" can be set to "1" to stop the menu quitting.

CHANGE LOG
----------

September 2015:

  * Now uses larger PNG icons with alpha mask instead of BMP.
  * DPAD can be defined as buttons.

February 18, 2015:

  * Fixed corruption of icon (dispmanx bug) going off the left of the screen.
  * Controls default to XBOX360 Controller.

August 24, 2013:

  * Added arguments option in the config file.
  * Added zoom on start/finish.

August 12, 2013:

  * Change to the executable directory before executing binary.

August 5, 2013:

  * Initial release.

Copyright 2015 Squid
