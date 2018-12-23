# Scriptable SPI Analyzer for Saleae Logic

The built-in SPI Analyzer for the Saleae Logic provides you with only a
few basic options for how to display the transferred bytes -- as ascii text,
or in one of several numeric formats.  What if you're working with an
SPI device that encodes more than just integer or text data into those bytes, or,
even more likely, stores multiple values in each byte that will require you
to either do the math in your head or export the data for post-processing?
Isn't that really the sort of thing computers are great at doing, though?

This "Scriptable" SPI analyzer allows you to define a simple external script
to execute that can provide its own values to display for each SPI frame;
you won't have to do the math in your head anymore!

## Installation

### MacOS

Dependencies:
- XCode with command line tools
- CMake 3.11+

Installing command line tools after XCode is installed:
```
xcode-select --install
```

Then open XCode, open Preferences from the main menu, go to locations, and select the only option under 'Command line tools'.

Installing CMake on MacOS:

1. Download the binary distribution for MacOS, `cmake-*-Darwin-x86_64.dmg`
2. Install the usual way by dragging into applications.
3. Open a terminal and run the following:
```
/Applications/CMake.app/Contents/bin/cmake-gui --install
```
*Note: Errors may occur if older versions of CMake are installed.*

Building the analyzer:
```
mkdir build
cd build
cmake ..
cmake --build .
```

### Ubuntu 16.04

Dependencies:
- CMake 3.11+
- gcc 4.8+

Misc dependencies:

```
sudo apt-get install build-essential
```

Building the analyzer:
```
mkdir build
cd build
cmake ..
cmake --build .
```

### Windows

Dependencies:
- Visual Studio 2015 Update 3
- CMake 3.11+

**Visual Studio 2015**

*Note - newer versions of Visual Studio should be fine.*

Setup options:
- Programming Languages > Visual C++ > select all sub-components.

Note - if CMake has any problems with the MSVC compiler, it's likely a component is missing.

**CMake**

Download and install the latest CMake release here.
https://cmake.org/download/

Building the analyzer:
```
mkdir build
cd build
cmake ..
```

Then, open the newly created solution file located here: `build\spi_analyzer.sln`



## Protocol

See the "examples" directory for some basic examples of functional scripts.

### Saleae to your script

During different phases of data processing, different types of messages
will be emitted.  All messages expect a single line reply ending in a newline;
not responding with at least an empty line will cause Saleae to hang
while waiting for input!

#### Bubbles

For each frame found by Saleae, your script will receive on stdin the following
tab-delimited fields ending with a newline character:

* "bubble"
* frame index: A hexadecimal integer indicating this frame's index.
* starting sample ID: A hexadecimal integer indicating the frame's
  starting sample ID.
* ending sample ID: A hexadecimal integer indicating the frame's
  ending sample ID.
* "mosi" or "miso"
* value: A hexadecimal integer indicating the frame's value. 

Example:

```
bubble	ab6f	3ae3012	3ae309b9	mosi	c6
```

Your script should respond with the text to display above this frame
in the analyzer; if the above message data indicates that this frame
is a write to IOCTL, you could respond with this, for example:

```
IOCTL (Write)
```

If you would not like to set a value, return an empty line.

#### Markers

For every sample point, your script will receive on stdin the following
tab-delimited fields ending with a newline character:

* "marker"
* frame index: A hexadecimal integer indicating this frame's index.
* sample count: A hexadecimal number of samples taken as part of this frame.
* starting sample ID: A hexadecimal integer indicating the frame's
  starting sample ID.
* ending sample ID: A hexadecimal integer indicating the frame's
  ending sample ID.
* mosi value: A hexadecimal integer indicating the frame's mosi value. 
* miso value: A hexadecimal integer indicating the frame's miso value. 

Example:

```
marker	ab6f	8	3ae3012	3ae309b9	c6	fa
```

Your script should respond with any number lines, each composed of
 three tab-separated values; send an empty line to finish.

* sample number: The (hexadecimal) sample number (within this frame) at which to
  display this marker.
* "miso" or "mosi"
* marker type (see below)

Possible marker types are:

* "Dot"
* "ErrorDot"
* "Square"
* "ErrorSquare"
* "UpArrow"
* "DownArrow"
* "X"
* "ErrorX"
* "Start"
* "Stop"
* "One"
* "Zero"

For example; if you want to show "DownArrow" on the first sample of miso,
and "Stop" on the fourth sample of mosi:

```
0	miso	DownArrow
4	mosi	Stop

```

If you would not like to set a marker on any sample, return an empty line.

### Your script to Saleae

For each frame, your script is expected to respond with any number of
lines followed by an empty line; all terminated by a newline character.
Even if your script would like no value to be displayed, an empty
line must be written!

Commands:

* bubble	TEXT: Display TEXT in the bubble above this frame.
* marker	TYPE	SAMPLE_NUMBER: Show a marker of TYPE at the specified SAMPLE_NUMBER.

You can respond with any number of commands per frame.

Example:

```
bubble	IOCTL Register (Write): 328
```

More commands may be added in the future.
