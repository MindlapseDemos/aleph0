Building and running on DOS
---------------------------
Make sure you have Watcom or OpenWatcom installed, and the appropriate env-vars
set (the watcom installer automatically adds them to autoexec.bat by default).

Run wmake to build. Needs dos4gw.exe in current dir.

The demo requires VESA Bios Extensions (VBE) 2.0. If your graphics card doesn't
support VBE 2.0 or greater, then make sure to run the `univbe` TSR first, or
the demo will fail to find a usable LFB video mode.


Cross-compile on GNU/Linux
--------------------------
source owdev script with contents (change WATCOM var as necessary):

  export WATCOM=$HOME/devel/ow
  export PATH=$WATCOM/binl:$PATH
  export INCLUDE=$WATCOM/h:$INCLUDE

Run wmake to build. Needs dos4gw.exe and wstub.exe in current dir or PATH

Simply running ./demo.exe might invoke magic of the ancients to start wine,
which will in turn start dosbox, which will execute the DOS binary! If the gods
are slumbering in valhalla, just typing `dosbox demo.exe` should do the trick.


Building and running with the SDL backend
-----------------------------------------
Run make to build (assuming make on your system is GNU make), or use the visual
studio 2013 project on Windows.

The SDL backend will scale the framebuffer up, by the factor specified in the
`FBSCALE` environment variable. So run the demo as: `FBSCALE=3 ./demo` for
a 3x scale factor, or just export the `FBSCALE` env var in the shell you are
going to use for running the demo. The default scale factor is 2x.
