# Enrichable SPI Analyzer for Saleae Logic

The built-in SPI Analyzer for the Saleae Logic provides you with only a few basic options for how to display the transferred bytes -- as ascii text, or in one of several numeric formats.
What if you're working with an SPI device that encodes more than just integer or text data into those bytes,
or even more likely, stores multiple values in each byte that will require you to either do the math in your head or export the data for post-processing?
That's the sort of thing computers are great at doing; why not just let your computer do that?

This "Enrichable" SPI analyzer allows you to define a simple external script that can provide its own values to display for each SPI frame;
you won't have to do the math in your head anymore!

## Compiling

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

## Use

1. Copy `libenrichable_spi_analyzer.so` to your Saleae Logic search path
   (see https://support.saleae.com/faq/technical-faq/setting-up-developer-directory for more detail)
   and restart Saleae Logic.
2. From the Analyzers menu, select "SPI (Enrichable)".
3. Configure the Analyzer as you would normally, but fill-in a value for "Enrichment Script".
   If you do not already have such a script, read below;
   they are very easy to write.
4. Begin capturing data!

## Protocol

See the "examples" directory for some basic examples of functional scripts,
and if your language of choice is Python, see the "Python Module" section below.

All interaction between your script and Saleae Logic is over stdin and stdout.
During different phases of data processing, different types of messages will be received by your script from Saleae Logic.
All messages must be replied to with at least one line of output,
but that line may be empty if you have no desire to handle the received message type.

### Bubbles

![Bubbles](https://s3-us-west-2.amazonaws.com/coddingtonbear-public/github/saleae-enrichable-spi-analyzer/bubbles_2.png)

For each frame found by Saleae Logic, your script will receive on stdin the following tab-delimited fields ending with a newline character:

* "bubble"
* frame index: A hexadecimal integer indicating this frame's index.
* starting sample ID: A hexadecimal integer indicating the frame's
  starting sample ID.
* ending sample ID: A hexadecimal integer indicating the frame's
  ending sample ID.
* type: A hexadecimal integer indicating the frame's type;
* flags: A hexadecimal integer indicating the frame's flags.
* "mosi" or "miso"
* value: A hexadecimal integer indicating the frame's value. 

Example:

```
bubble	ab6f	3ae3012	3ae309b9	0	0	mosi	c6
```

If you would like a bubble to appear, your script should respond with at least one message to display above this frame in the analyzer.
Given that space available for displaying your message may vary depending upon how far zoomed-in you are,
it is recommended that you return multiple messages of differing levels of verbosity.
Send an empty newline to finish your list of messages, and if you would not like a bubble displayed at all, send only an empty line.
For example, if the above message data indicates that this frame is a read of the RXLVL register on channel A,
you could respond with this:

```
Request read of RXLVL on Channel A
(R) RXLVL Ch A
R RXLVL [A]
0xc6

```

If you would not like to set a value, return an empty line.

### Tabular

![Tabular](https://s3-us-west-2.amazonaws.com/coddingtonbear-public/github/saleae-enrichable-spi-analyzer/tabular_2.png)

For each frame found by Saleae Logic, your script will receive on stdin the following tab-delimited fields ending with a newline character:

* "tabular"
* frame index: A hexadecimal integer indicating this frame's index.
* starting sample ID: A hexadecimal integer indicating the frame's
  starting sample ID.
* ending sample ID: A hexadecimal integer indicating the frame's
  ending sample ID.
* type: A hexadecimal integer indicating the frame's type;
* flags: A hexadecimal integer indicating the frame's flags.
* mosi value: A hexadecimal integer indicating the frame's mosi value.
* miso value: A hexadecimal integer indicating the frame's miso value.

Example:

```
tabular	ab6f	3ae3012	3ae309b9	0	0	c6	fa
```

Your script should respond with any lines you would like to appear in the tabular results on the bottom right side of the UI.
End your list of lines by sending an empty line.
For example, if the above frame was a read of the RXLVL register on channel A,
you could respond with the below to show that in the tabular results:

```
(R) RXLVL Ch A
```

If you would not like to set a value, return an empty line.

### Markers

![Markers](https://s3-us-west-2.amazonaws.com/coddingtonbear-public/github/saleae-enrichable-spi-analyzer/markers_3.png)

For every sample point, your script will receive on stdin the following tab-delimited fields ending with a newline character:

* "marker"
* frame index: A hexadecimal integer indicating this frame's index.
* sample count: A hexadecimal number of samples taken as part of this frame.
* starting sample ID: A hexadecimal integer indicating the frame's
  starting sample ID.
* ending sample ID: A hexadecimal integer indicating the frame's
  ending sample ID.
* type: A hexadecimal integer indicating the frame's type;
* flags: A hexadecimal integer indicating the frame's flags.
* mosi value: A hexadecimal integer indicating the frame's mosi value.
* miso value: A hexadecimal integer indicating the frame's miso value.

Example:

```
marker	ab6f	8	3ae3012	3ae309b9	0	0	c6	fa
```

Your script should respond with any number lines, each composed of three tab-separated values;
send an empty line to finish.

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

See the above screenshot for examples.

For example; if you want to show "DownArrow" on the first sample of miso,
and "Stop" on the fourth sample of mosi:

```
0	miso	DownArrow
4	mosi	Stop

```

If you would not like to set a marker on any sample, return an empty line.

### Feature (Enablement)

For either performance reasons or expediency, you might want to receive messages of only certain types.
Upon launching your script, your script will be given the opportunity to disable specific message types by repsonding appropriately.

For message types that can be disabled by this analyzer, your script will recieve the following tab-delimited fields ending with a newline character:

* "feature"
* "bubble", "marker", or "tabular"

To prevent messages of the indicated type from being generated and sent by the analyzer, you can respond with "no"; responding with any other value (including an empty line) indicates that messages of that type should continue to be generated and sent.

For example, if your script does not intend to add markers; you can improve performance by disabling markers.
To do that, when you receive the following message:

```
feature marker
```

respond with:

```
no
```

Features that are disabled will revert to the standard SPI Analyzer implementation of that feature.

Even if you intend to support only a subset of features, it is important that your script continue to respond with an empty newline when receiving an unexpected message -- new message types may be added at any time!

## Python Module

If you're hoping to put together an analyzer as quickly as possible,
using the included python module is probably your smoothest path forward.

### Installation

Use of this Python module requires at least Python 3.4.

From within the `python` directory:

```
pip install .
```

### Use

Using this is as simple as creating your own module somewhere that subclasses `saleae_enrichable_spi_analyzer.EnrichableSpiAnalyzer` with methods for the features you'd like to use;
here is a basic example:

```python
import sys

from saleae_enrichable_spi_analyzer import (
    EnrichableSpiAnalyzer, Channel, Marker, MarkerType
)


class MySimpleAnalyzer(EnrichableSpiAnalyzer):
    def get_bubble_text(
        self,
        frame_index: int,
        start_sample: int,
        end_sample: int,
        frame_type: int,
        flags: int,
        direction: Channel,
        value: int
    ) -> List[str]:
        return [
            "This message will be displayed above every frame in the blue bubble"
        ]

    def get_markers(
        self,
        frame_index: int,
        sample_count: int,
        start_sample: int,
        end_sample: int,
        frame_type: int,
        flags: int,
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

The following methods can be implemented for interacting with Saleae Logic:

* `get_bubble_text(frame_index, start_sample, end_sample, frame_type, flags, direction, value)`:
  Set the bubble text (the text shown in blue abov the frame) for this frame.
  By default, no bubble is shown.  It is recommended that you return multiple
  strings of varying lengths.
* `get_markers(frame_index, sample_count, start_sample, end_sample, frame_type, flags, mosi_value, miso_value)`:
  Return markers to display at given sample points.
  By default, no markers are displayed.
* `get_tabular(frame_index, start_sample, end_sample, frame_type, flags, mosi_value, miso_value)`:
  Data to display in the tabular "Decoded Protocols" section.
  By default, uses the bubble text for each channel.

To improve performance, you are also able to disable messages of specific types
by setting the following class-level boolean values:

* `ENABLE_MARKER`; setting this to false will instruct Saleae Logic to not
  send `marker` messages to your script.
* `ENABLE_BUBBLE`; setting this to false will instruct Saleae Logic to not
  send `bubble` messages to your script.
* `ENABLE_TABULAR`; setting this to false will instruct Saleae Logic to not
  send `tabular` messages to your script.

See the example `custom_class.py` for an example of this in use.
