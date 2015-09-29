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
    ICON[01234].bmp -> Default ICONs used for the game emulators

It will work in X-Windows or in the Console.

CONFIGURATION
-------------
You can add/edit emulators in the [General] section of the pimenu.cfg file. 

Each emulator to appear requires an "icon_command_" section pointing to the 
executable and an equivalent icon in the pimenu directory, called 
"ICON_3.bmp" (for example the number 3 emulator). The number needs to match 
the icon and the "icon_command_". Also make sure the "icon_count" matches the 
number of entries.
Optionally you can add a "icon_args_" section to pass arguments to a command.

The icon must be size 192x192 and uncompressed bitmap with a white 
background. The following ImageMagick command creates the correct format:

    convert ICON.png -resize 192x192 -compress none bmp3:ICON3.bmp

You can run bash scripts, example here to poweroff the Pi:

icon_command_3=sudo /sbin/reboot
icon_args_3=

The "kioskmode" can be set to "1" to stop the menu quitting.

CHANGE LOG
----------

September 2015:

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
