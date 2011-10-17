This program is Copyright 2011 Ben Greenwood, and is distributed
under the terms of the GNU General Public License.

Normal installation:
Right-click on lagarith.inf and select install to install the codec. If you
do not see an install option, make sure you extracted the files, and are using
the extracted lagarith.inf file.

Installing the 32bit version on Windows 64
Extract all the files to a folder
Open up a command prompt (start->run: cmd)
Change directories to sysWOW64 ("cd C:\windows\syswow64")
Type the following (replace example_path with the actual path to the extracted files), then hit enter: rundll32.exe setupapi.dll,InstallHinfSection DefaultInstall 0 c:\example_path\lagarith.inf
This method can also be used for installing other 32bit codecs such as Huffyuv that install via a .inf file.

To install under Vista 64, you will need to run cmd.exe as administrator


If you find any errors in my codec, please let me know, my email is brg0853@rit.edu
and AIM is "slrlagsalot"

Format constraints:
All formats require a MMX capable processor.
YUY2 and YV12 must have a width divisible by 4.
YV12 requires an even height.
UYVY and YV16 cannot be downsampled.
