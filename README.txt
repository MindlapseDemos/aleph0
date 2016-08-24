Cross-compile on GNU/Linux
--------------------------

source owdev file with contents (change WATCOM var as necessary):

  export WATCOM=$HOME/devel/ow
  export PATH=$WATCOM/binl:$PATH
  export INCLUDE=$WATCOM/h:$INCLUDE

Run wmake to build. Needs dos4gw.exe and wstub.exe in current dir or PATH

Simply running ./demo.exe will invoke magic of the ancients to start wine,
which will in turn start dosbox, which will execute the DOS binary!
