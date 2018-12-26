# Enrichable SPI Analyzer for Saleae Logic

[![Build Status](https://travis-ci.org/coddingtonbear/saleae-enrichable-spi-analyzer.svg?branch=master)](https://travis-ci.org/coddingtonbear/saleae-enrichable-spi-analyzer)

The built-in SPI Analyzer for the Saleae Logic provides you with only a few basic options for how to display the transferred bytes -- as ascii text, or in one of several numeric formats.
What if you're working with an SPI device that encodes more than just integer or text data into those bytes, or even stores multiple values in each byte that will require you to either do the math in your head, export the data for post-processing, or display the frame as binary bits so you can directly look at the parts that matter to you?
That's the sort of thing computers are great at doing; why don't we just let your computer do that?

This "Enrichable" SPI analyzer allows you to define a simple external script written in your favorite language that can provide its own text and markers to display for each SPI frame.
Now you can focus on solving your actual problem instead of interpreting inscrutible hex values.

## Related

* [python-saleae-enrichable-analyzer](https://github.com/coddingtonbear/python-saleae-enrichable-analyzer): A Python module that intends to make it extremely easy for you to write your own enrichment script.
* [saleae-enrichable-i2c-analyzer](https://github.com/coddingtonbear/saleae-enrichable-i2c-analyzer): A version of the Saleae I2C analyzer that supports enrichment.

## Getting Started

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

### Ubuntu 18.04

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

Unfortunately, Windows is not currently supported due to the fact that this library relies upon Posix interfaces like `pipe` and `fork`.
If you would like to add support for Windows, it should be as easy as implementing Windows-compatible versions of the following functions:

* `void EnrichableAnalyzerSubprocess::Start()`
* `void EnrichableAnalyzerSubprocess::Stop()`
* `bool EnrichableAnalyzerSubprocess::SendOutputLine(const char* buffer, unsigned bufferLength)`
* `bool EnrichableAnalyzerSubprocess::GetInputLine(char* buffer, unsigned bufferLength)`

There _are_ Windows equivalents of the aforementioned `pipe` and `fork`
(see more information here: https://support.microsoft.com/en-us/help/190351/how-to-spawn-console-processes-with-redirected-standard-handles),
so although that is better left to somebody more familiar with Windows,
it likely isn't a huge task for somebody who is!

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
and if your language of choice is Python, see [this repository](https://github.com/coddingtonbear/python-saleae-enrichable-analyzer).

All interaction between your script and Saleae Logic is over stdin and stdout.
During different phases of data processing, different types of messages will be received by your script from Saleae Logic.
All messages must be replied to with at least one line of output,
but that line may be empty if you have no desire to handle the received message type.

### Bubbles

![Bubbles](https://s3-us-west-2.amazonaws.com/coddingtonbear-public/github/saleae-enrichable-spi-analyzer/bubbles_2.png)

For each frame _displayed_, your script will receive on stdin the following tab-delimited fields ending with a newline character:

* "bubble"
* packet id: A hexadecimal integer indicating the packet that this
  frame is a member of.
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
bubble	84ac	ab6f	3ae3012	3ae309b9	0	0	mosi	c6
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

For each frame _analyzed_, your script will receive on stdin the following tab-delimited fields ending with a newline character:

* "tabular"
* packet id: A hexadecimal integer indicating the packet that this
  frame is a member of.
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
tabular	84ac	ab6f	3ae3012	3ae309b9	0	0	c6	fa
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

For every frame _analyzed_, your script will receive on stdin the following tab-delimited fields ending with a newline character:

* "marker"
* packet id: A hexadecimal integer indicating the packet that this
  frame is a member of.
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
marker	84ac	ab6f	8	3ae3012	3ae309b9	0	0	c6	fa
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
Upon launching your script, your script will be given the opportunity to disable specific message types by responding appropriately.

For message types that can be disabled by this analyzer, your script will receive the following tab-delimited fields ending with a newline character:

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

## Frame Types

Unlike some protocols, SPI does not have multiple types of frames;
that does not mean that all frames are equal --
very often frames are sent in a specific sequence.
To make it easy to identify the function of a given frame,
this field will return the frame's index in its packet.

## Frame Flags

There are currently no flags set by this analyzer,
but flags may be added in the future.
