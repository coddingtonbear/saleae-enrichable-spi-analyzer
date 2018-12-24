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

See the "examples" directory for some basic examples of functional scripts, and
if your language of choice is Python, see the "Python Module" section below.

All interaction between your script and Saleae is over standard out and in.
During different phases of data processing, different types of messages
will be emitted.  All messages must be replied to with at least one
line of output; that line may be empty if your script does not want
to handle a given message type.

### Bubbles

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

### Tabular

For each frame found by Saleae, your script will receive on stdin the
following tab-delimited fields ending with a newline character:

* "tabular"
* frame index: A hexadecimal integer indicating this frame's index.
* starting sample ID: A hexadecimal integer indicating the frame's
  starting sample ID.
* ending sample ID: A hexadecimal integer indicating the frame's
  ending sample ID.
* mosi value: A hexadecimal integer indicating the frame's mosi value. 
* miso value: A hexadecimal integer indicating the frame's miso value. 

Example:

```
tabular	ab6f	3ae3012	3ae309b9	c6	fa
```

Your script should respond with the text to show in the tabular results
(on the bottom right side of the UI). If the above frame was a write
to IOCTL with a value of 0xfa; you could display this on the table as:

```
IOCTL (Write): fa
```

If you would not like to set a value, return an empty line.

### Markers

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


## Python Module

If you're hoping to put together an analyzer as quickly as possible, using
the included python module is probably your best path forward.

### Installation

Use of this Python module requires at least Python 3.4.

From within the `python` directory:

```
pip install .
```

### Use

Using this is as simple as creating your own module somewhere that subclasses
`saleae_scriptable_spi_analyzer.ScriptableSpiAnalyzer` with methods for the
features you'd like to use; here is a basic example:

```python
import sys

from saleae_scriptable_spi_analyzer import (
    ScriptableSpiAnalyzer, Channel, Marker, MarkerType
)


class MySimpleAnalyzer(ScriptableSpiAnalyzer):
    def get_bubble_text(
        self,
        frame_index: int,
        start_sample: int,
        end_sample: int,
        direction: Channel,
        value: int
    ) -> str:
        return (
            "This message will be displayed above every frame in the blue bubble"
        )

    def get_markers(
        self,
        frame_index: int,
        sample_count: int,
        start_sample: int,
        end_sample: int,
        mosi_value: int,
        miso_value: int
    ) -> List[Marker]:
        markers = []

        if(miso_value == 0xff) {
            # This will show a "Stop" marker on the zeroth sample
            # of the frame on the MISO channel when its value is 0xff.
            markers.append(
                Marker(0, Channel.MISO, MarkerType.Stop)
            )
        }

        return markers

if __name__ == '__main__':
    MySimpleAnalyzer(sys.argv[1:])
```

The following methods can be implemented for interacting with Saleae:

* `get_bubble_text(frame_index, start_sample, end_sample, direction, value)`: Set the bubble text (the text shown in blue above
  the frame) for this frame.  By default, no bubble is shown.
* `get_markers(frame_index, sample_count, start_sample, end_sample, mosi_value, miso_value)`: Return markers to display at given sample points.  By default, no markers are displayed.
* `get_tabular(frame_index, start_sample, end_sample, mosi_value, miso_value)`: Data to display in the tabular "Decoded Protocols" section.  By default, uses the bubble text for each channel.


See the example `custom_class.py` for an example of this in use.
