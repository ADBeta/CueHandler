# CueHandler

CueHandler is a very lightweight library for C++11 and later, that 
allows for simple, fast and reliable processing of .cue files, with ability
to pass the files inside the .cue file to external libraries for dumping or
processing etc.

This library is filly compatible with 32bit architectures - and puts reliability
and speed first.

**You must also copy the TeFiEd files**
Simply copy the .hpp and .cpp files to your project and `#include` them.  
TeFiEd is a standalone text editor with very high levels of functionality, by 
including it you can also use it to handle other text files, which can greatly 
improve your workflow. Check out [TeFiEd's GitHub here](https://github.com/ADBeta/TeFiEd)

**Note:** cue-handler is currently set up for PSX games, assuming 2352 bytes per
sector. I an working on eliminating this restriction in future versions.


----
## TODO
* remove depends on TeFiEd??
* add more FILE types as needed
* Create more helper funtions to simplify things like merging bin files etc

## Licence
<b> 2023 ADBeta </b>  
This software is under the GPL 2.0 Licence, please see LICENCE for information
