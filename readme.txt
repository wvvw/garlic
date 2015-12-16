GARLIC is a custom generator of Torchat ID's and onion domains.

How to compile 'Garlic' from scratch
====================================

Requirements:
-------------
(1) The source files included with this package.
(2) OpenSSL
(3) ActivePerl
(4) Netwide Assembler - NASM
(5) A C++ compiler - I'm using VSN 2005. Earlier versions work too.

How it's done, very briefly:
----------------------------
OpenSSL must be compiled to a multi-threaded, static library. Doing
this with an assembler yields a faster running binary. Instructions
can be found in the file install.w32 located in the topmost folder
of the OpenSSL source.

DOS commands will look like:

> perl Configure VC-WIN32
> ms\do_nasm
> nmake -f ms\nt.mak install

OpenSSL header files need to be placed in a subfolder of your
compiler's include folder named 'openssl'. The compiled lib, 
libeay32.lib, gets placed in your compiler's lib folder. These are
gotten from the folders inc32 and out32 which appear after OpenSSL
is compiled. Note: My copies of the openssl subfolder and libeay32.lib
are included for convenience if you don't wish to compile you own.
The lib file was compiled with the microsoft assembler, masm.

Open a console window (DOS prompt) and change to Garlic's directory.
Set your compiler's environment using vsvars32.bat if it's not
already set in Windows. The default location for this file is
"C:\Program files\Microsoft Visual Studio 8\Common7\Tools\vsvars32.bat".

Run MSC.BAT to compile the executable.

As long as the same DOS window is kept open, you will not require
to run vsvars32.bat more than the first time. For subsequent
compiles just run MSC.BAT again.


How Garlic works
================
Originally, Garlic was a port of Cowboy Bebop's onionhash. His code was
duplicated in a windows GUI. It was robust and yielded 100% usable keys.
The windows version actually ran slightly faster than the console
version of onionhash before any tweaks were applied. Now, Garlic is a
fork of Shallot with the bad key bug corrected.

The OpenSSL library is used throughout to process a large series of
keys, only the first of which is actually randomly generated - a
relatively slow process. Once a pattern match is found, the library
is again used to verify the key's integrity.

Onion domains are brute forced. A typical run with the original Garlic
would process from 30 to 45 million domains per minute per processor
core depending on the machine's speed. A four-core processor would have
gone through 180 million domains a minute seeking a match. Now, a four-
core processor can process a billion domains per minute.

Shallot - an alternate app
==========================
Shallot is a console app by `Orum based on onionhash but optimized
to work several times faster. From 90 to 160 million domains per
core can be processed each minute depending on machine architecture.
Shallot shortcuts the openSSL routines in several places, but
unfortunately, fails with pattern matches of more than five
characters. Slight alteration of the code can yield five good keys
out of six with six or seven char patterns, but all fail at eight.
Shallot routines have only recently been incorporated into Garlic
once the cause of the bad keys was fully understood and corrected.
The current Garlic runs 5.75 times faster than the original. I see
over 250 million tries per processor core, and with all 4 cores
searching, I get slightly in excess of 1 billion tries per minute.


Program description
===================
Garlic has a fixed window size determined essentially by the fixed
system font that displays the private key. It can't be made smaller
than this and there is no need for it to be larger.

Garlic launches with a random key/domain. Other random domains can be
generated with a button press. That button becomes disabled the moment
a search for a pattern is initiated.

Searches are run in separate threads at idle priority. Garlic will
use all the available processor time of however many cores you desire
to use by assigning the same number of threads, but will not take away
from other programs to do so. It can run in the background minimized
to the system tray while you do other things. If a result is obtained
while the program is minimized, Garlic will open back up to draw your
attention.

Domains and keys can be copied out of their respective windows and
pasted elsewhere. For domains of six or more characters, these are
optionally saved automatically to log files for future retrieval. The
files, named per the search terms, are placed in a subfolder if
separate logs are chosen. The default is a single file, garlic.log,
placed in the same folder as the executable. Domains of five or fewer
characters are found so quickly there is no need to log them as the
log file(s) would quickly grow out of all bounds from just playing
around with the program.

A result you wish to use immediately can be saved directly. Garlic
will save the two files, hostname & private_key to the folder you
select. Tor and Torchat will recognize these files when creating
your onion or torchat ID.

Garlic minimizes to the notification area (systray). It will restore
its icon in case of an explorer crash so you won't be left with an
orphaned program running.

An animation runs while Garlic searches. This is a backgound animation
provided by windows that doesn't take away processor power from your
search. Elapsed search time is shown and updates every five seconds.

Projected search times are based on your computer's speed which is
benchmarked at startup. Benchmarking causes a small startup delay
of around a second-and-a-half the first time you run Garlic. If
you think you got a poor benchmark the first time, you can un-tic
'Remember benchmark' and try again & again if need be until you're
satisfied with the speed shown. Don't forget to re-tic the setting.

Garlic generates a small ini file in the same folder as the
executable the first time it is run. This is used to store the
benchmarked speed. Subsequent runs will not suffer the second-and-
a-half delay. The number of threads used is likewise saved. Search
terms are kept by default but this can be turned off. An option to
recall the window position is available as is an option to disable
logging.

Choosing more threads than you have processor cores will lower the
indicated seach times but will not yield those shorter times. Only
choose as many threads as you have cores. Search results arrive at
random times and can vary from almost instantaneously to five or six
times projected. Projected times are the average between results for
a very large number of searches and serve only as a rough guide of
what to expect.

Garlic, as with other programs like it, does a probabilistic search.
Progress is not lost when you stop the program. If your search is
supposed to take on average 19 hours, this does not require a solid
19 hours of running. The result is just as likely to occur with 19
separate 1-hour sessions.
