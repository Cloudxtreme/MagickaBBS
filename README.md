# MagickaBBS
Linux/FreeBSD bulletin board system (Should also work on NetBSD and OpenBSD, if it doesn't it's a bug)

As I lost the code to my initial BBS flea, I've decided to start over from scratch and this time I'm using git hub so I dont
lose it again!

Magicka is meant to be a modern (haha) BBS system, using modern technologies, like Sqlite, IRC, long filenames (gasp!) etc
while still retaining the classic BBS feel. ANSI & Telnet, and good old ZModem.

If you want to install Magicka BBS, follow these steps.

1. Ensure you have git, c compiler, sqlite-dev and gnu make
2. Clone the repo `git clone https://github.com/apamment/MagickaBBS`
3. Build JamLib

  `cd MagickaBBS/jamlib`
  
 
  `make -f Makefile.linux` (Linux) `gmake -f Makefile.linux` (*BSD)

3. Build libzmodem

  `cd MagickaBBS/Xmodem`
  
 
  `make` (Linux) `gmake` (*BSD)
  
4. Build LUA

	`cd MagickaBBS/lua`

	`make linux` (Linux) `gmake freebsd` (FreeBSD)
  
5. Build the BBS (You may have to adjust the Makefile for your system)

  `make -f Makefile.linux` (Linux) `gmake -f Makefile.freebsd` (FreeBSD)
     
6. Make a directory for logs.
	
	`mkdir logs`

7. Copy the config-default directory to a config directory.

  `cp -r config_default config`

8. Edit the config files and update essential information, like system paths and BBS name etc
9. Copy the ansi-default directory to the one specified in your system path

  eg.
  
  `cp -r ansi_default ansis`



10. Run Magicka BBS on a port over 1024 (Below require root, and we're not ready for that).

  `./magicka config/bbs.ini 2300`
  
11. Your BBS is now running on port 2300, log in and create yourself an account! (By default there is only one security level, you can add more, 
but you will need to use an SQLite Manager to modify users.sq3 and set security levels, as there is no user editor yet.

For information on how to configure your BBS, check the wiki https://github.com/apamment/MagickaBBS/wiki
