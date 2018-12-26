#include "EnrichableAnalyzerSubprocess.h"

#include <iostream>
#include <sstream>
#include <string>
#include <mutex>

#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <wordexp.h>

std::mutex subprocessLock;

EnrichableAnalyzerSubprocess::EnrichableAnalyzerSubprocess():
	enabled(false),
	featureMarker(true),
	featureBubble(true),
	featureTabular(true),
	parserCommand("")
{
}

EnrichableAnalyzerSubprocess::~EnrichableAnalyzerSubprocess()
{
}

std::vector<EnrichableAnalyzerSubprocess::Marker> EnrichableAnalyzerSubprocess::EmitMarker(
	U64 packetId,
	U64 frameIndex,
	Frame& frame,
	U32 sampleCount
) {
	std::vector<EnrichableAnalyzerSubprocess::Marker> markers;

	if(! (enabled && featureMarker)) {
		return markers;
	}

	std::stringstream outputStream;

	outputStream << MARKER_PREFIX;
	outputStream << UNIT_SEPARATOR;
	outputStream << std::hex << packetId;
	outputStream << UNIT_SEPARATOR;
	outputStream << std::hex << frameIndex;
	outputStream << UNIT_SEPARATOR;
	outputStream << std::hex << sampleCount;
	outputStream << UNIT_SEPARATOR;
	outputStream << std::hex << frame.mStartingSampleInclusive;
	outputStream << UNIT_SEPARATOR;
	outputStream << std::hex << frame.mEndingSampleInclusive;
	outputStream << UNIT_SEPARATOR;
	outputStream << std::hex << (U64)frame.mType;
	outputStream << UNIT_SEPARATOR;
	outputStream << std::hex << (U64)frame.mFlags;
	outputStream << UNIT_SEPARATOR;
	outputStream << std::hex << (U64)frame.mData1;
	outputStream << UNIT_SEPARATOR;
	outputStream << std::hex << (U64)frame.mData2;
	outputStream << LINE_SEPARATOR;

	std::string outputValue = outputStream.str();

	LockSubprocess();
	SendOutputLine(
		outputValue.c_str(),
		outputValue.length()
	);
	char markerMessage[256];
	while(true) {
		GetInputLine(
			markerMessage,
			256
		);
		if(strlen(markerMessage) > 0) {
			char forever[256];
			strcpy(forever, markerMessage);

			char *sampleNumberStr = strtok(markerMessage, "\t");
			char *channelStr = strtok(NULL, "\t");
			char *markerTypeStr = strtok(NULL, "\t");

			if(sampleNumberStr != NULL && channelStr != NULL && markerTypeStr != NULL) {
				U64 sampleNumber = strtoll(sampleNumberStr, NULL, 16);

				markers.push_back(
					Marker(
						sampleNumber,
						channelStr,
						GetMarkerType(markerTypeStr, strlen(markerTypeStr))
					)
				);
			} else {
				std::cerr << "Unable to tokenize marker message input: \"";
				std::cerr << forever;
				std::cerr << "\"; input should be three tab-delimited fields: ";
				std::cerr << "sample_number\tchannel\tmarker_type\n";

				std::cerr << "Disabling analyzer subprocess.\n";
				enabled = false;
				return markers;
			}
		} else {
			break;
		}
	}
	UnlockSubprocess();

	return markers;
}

std::vector<std::string> EnrichableAnalyzerSubprocess::EmitBubble(U64 packetId, U64 frameIndex, Frame& frame, std::string channelName) {
	std::vector<std::string> bubbles;

	if(! (enabled && featureBubble)) {
		return bubbles;
	}

	std::stringstream outputStream;
	outputStream << BUBBLE_PREFIX;
	outputStream << UNIT_SEPARATOR;
	outputStream << std::hex << packetId;
	outputStream << UNIT_SEPARATOR;
	outputStream << std::hex << frameIndex;
	outputStream << UNIT_SEPARATOR;
	outputStream << std::hex << frame.mStartingSampleInclusive;
	outputStream << UNIT_SEPARATOR;
	outputStream << std::hex << frame.mEndingSampleInclusive;
	outputStream << UNIT_SEPARATOR;
	outputStream << std::hex << (U64)frame.mType;
	outputStream << UNIT_SEPARATOR;
	outputStream << std::hex << (U64)frame.mFlags;
	outputStream << UNIT_SEPARATOR;
	outputStream << channelName;
	outputStream << UNIT_SEPARATOR;
	outputStream << std::hex << frame.mData1;
	outputStream << LINE_SEPARATOR;
	std::string value = outputStream.str();

	LockSubprocess();
	SendOutputLine(value.c_str(), value.length());
	char bubbleText[256];
	while(true) {
		GetInputLine(
			bubbleText,
			256
		);
		if(strlen(bubbleText) > 0) {
			bubbles.push_back(bubbleText);
		} else {
			break;
		}
	}
	UnlockSubprocess();

	return bubbles;
}

std::vector<std::string> EnrichableAnalyzerSubprocess::EmitTabular(U64 packetId, U64 frameIndex, Frame& frame) {
	std::vector<std::string> lines;

	if(! (enabled && featureBubble)) {
		return lines;
	}

	std::stringstream outputStream;

	outputStream << TABULAR_PREFIX;
	outputStream << UNIT_SEPARATOR;
	outputStream << std::hex << packetId;
	outputStream << UNIT_SEPARATOR;
	outputStream << std::hex << frameIndex;
	outputStream << UNIT_SEPARATOR;
	outputStream << std::hex << frame.mStartingSampleInclusive;
	outputStream << UNIT_SEPARATOR;
	outputStream << std::hex << frame.mEndingSampleInclusive;
	outputStream << UNIT_SEPARATOR;
	outputStream << std::hex << (U64)frame.mType;
	outputStream << UNIT_SEPARATOR;
	outputStream << std::hex << (U64)frame.mFlags;
	outputStream << UNIT_SEPARATOR;
	outputStream << std::hex << frame.mData1;
	outputStream << UNIT_SEPARATOR;
	outputStream << std::hex << frame.mData2;
	outputStream << LINE_SEPARATOR;

	std::string value = outputStream.str();

	LockSubprocess();
	SendOutputLine(value.c_str(), value.length());
	char tabularText[512];
	while(true) {
		GetInputLine(
			tabularText,
			512
		);
		if(strlen(tabularText) > 0) {
			lines.push_back(tabularText);
		} else {
			break;
		}
	}
	UnlockSubprocess();

	return lines;
}

bool EnrichableAnalyzerSubprocess::MarkerEnabled() {
	return featureMarker;
}

bool EnrichableAnalyzerSubprocess::BubbleEnabled() {
	return featureBubble;
}

bool EnrichableAnalyzerSubprocess::TabularEnabled() {
	return featureTabular;
}

void EnrichableAnalyzerSubprocess::SetParserCommand(std::string cmd) {
	parserCommand = cmd;
	enabled = true;
}

void EnrichableAnalyzerSubprocess::Start() {
	if(!parserCommand.length()) {
		std::cerr << "No parser command defined; aborting subprocess.\n";
		Terminate();
	} else {
		std::cerr << "Starting analyzer subprocess: ";
		std::cerr << parserCommand;
		std::cerr << "\n";
	}


	if(pipe(inpipefd) < 0) {
		std::cerr << "Failed to create input pipe: ";
		std::cerr << errno;
		std::cerr << "\n";
		Terminate();
	}
	if(pipe(outpipefd) < 0) {
		std::cerr << "Failed to create output pipe: ";
		std::cerr << errno;
		std::cerr << "\n";
		Terminate();
	}
	std::cerr << "Starting fork...\n";
	commandPid = fork();

	if(commandPid == 0) {
		std::cerr << "Forked...\n";
		if(dup2(outpipefd[0], STDIN_FILENO) < 0) {
			std::cerr << "Failed to redirect STDIN: ";
			std::cerr << errno;
			std::cerr << "\n";
			exit(errno);
		}
		if(dup2(inpipefd[1], STDOUT_FILENO) < 0) {
			std::cerr << "Failed to redirect STDOUT: ";
			std::cerr << errno;
			std::cerr << "\n";
			exit(errno);
		}

		wordexp_t cmdParsed;
		char *args[25];

		wordexp(parserCommand.c_str(), &cmdParsed, 0);
		int i;
		for(i = 0; i < cmdParsed.we_wordc; i++) {
			args[i] = cmdParsed.we_wordv[i];
		}
		args[i] = (char*)NULL;

		close(inpipefd[0]);
		close(inpipefd[1]);
		close(outpipefd[0]);
		close(outpipefd[1]);

		execvp(args[0], args);

		std::cerr << "Failed to spawn analyzer subprocess!\n";
	} else {
		close(inpipefd[1]);
		close(outpipefd[0]);
	}

	// Check script to see which features are enabled;
	// * 'no': This feature can be skipped.  This is used to improve
	//   performance by allowing the script to not receive messages for
	//   features it does not support.
	// * 'yes': Send messages of this type.
	// * Anything else: Send messages of this type.  This might be surprising,
	//   but it's more important to me that the default case be simple
	//   than the default case be high-performance.   Scripts are expected
	//   to respond to even unhandled messages.
	featureBubble = GetFeatureEnablement(BUBBLE_PREFIX);
	featureMarker = GetFeatureEnablement(MARKER_PREFIX);
	featureTabular = GetFeatureEnablement(TABULAR_PREFIX);
}

void EnrichableAnalyzerSubprocess::Stop(int exitCode) {
	if(enabled) {
		close(inpipefd[0]);
		close(outpipefd[1]);

		kill(commandPid, SIGINT);

		exit(exitCode);
	}
}

void EnrichableAnalyzerSubprocess::Terminate() {
	Stop(1);
	enabled = false;
}

bool EnrichableAnalyzerSubprocess::GetFeatureEnablement(const char* feature) {
	std::stringstream outputStream;
	char result[16];
	std::string value;

	outputStream << FEATURE_PREFIX;
	outputStream << UNIT_SEPARATOR;
	outputStream << feature;
	outputStream << LINE_SEPARATOR;
	value = outputStream.str();

	GetScriptResponse(
		value.c_str(),
		value.length(),
		result,
		16
	);
	if(strcmp(result, "no") == 0) {
		std::cerr << "message type \"";
		std::cerr << feature;
		std::cerr << "\" disabled\n";
		return false;
	}
	return true;
}

void EnrichableAnalyzerSubprocess::LockSubprocess() {
	subprocessLock.lock();
}

void EnrichableAnalyzerSubprocess::UnlockSubprocess() {
	subprocessLock.unlock();
}

bool EnrichableAnalyzerSubprocess::GetScriptResponse(
	const char* outBuffer,
	unsigned outBufferLength,
	char* inBuffer,
	unsigned inBufferLength
) {
	bool result;

	LockSubprocess();
	SendOutputLine(outBuffer, outBufferLength);
	result = GetInputLine(inBuffer, inBufferLength);
	UnlockSubprocess();

	return result;
}

bool EnrichableAnalyzerSubprocess::SendOutputLine(const char* buffer, unsigned bufferLength) {
	#ifdef SUBPROCESS_DEBUG
		std::cerr << ">> ";
		std::cerr << buffer;
	#endif
	write(outpipefd[1], buffer, bufferLength);

	return true;
}

bool EnrichableAnalyzerSubprocess::GetInputLine(char* buffer, unsigned bufferLength) {
	unsigned bufferPos = 0;
	bool result = false;

	#ifdef SUBPROCESS_DEBUG
		std::cerr << "<< ";
	#endif

	while(true) {
		int result = read(inpipefd[0], &buffer[bufferPos], 1);
		if(buffer[bufferPos] == '\n') {
			break;
		}

		#ifdef SUBPROCESS_DEBUG
			std::cerr << buffer[bufferPos];
		#endif

		bufferPos++;

		if(bufferPos == bufferLength - 1) {
			break;
		}
	}
	buffer[bufferPos] = '\0';

	#ifdef SUBPROCESS_DEBUG
		std::cerr << '\n';
	#endif

	if(strlen(buffer) > 0) {
		result = true;
	}

	return result;
}

AnalyzerResults::MarkerType EnrichableAnalyzerSubprocess::GetMarkerType(char* buffer, unsigned bufferLength) {
	AnalyzerResults::MarkerType markerType = AnalyzerResults::Dot;

	if(strncmp(buffer, "ErrorDot", strlen(buffer)) == 0) {
		markerType = AnalyzerResults::ErrorDot;
	} else if(strncmp(buffer, "Square", strlen(buffer)) == 0) {
		markerType = AnalyzerResults::Square;
	} else if(strncmp(buffer, "ErrorSquare", strlen(buffer)) == 0) {
		markerType = AnalyzerResults::ErrorSquare;
	} else if(strncmp(buffer, "UpArrow", strlen(buffer)) == 0) {
		markerType = AnalyzerResults::UpArrow;
	} else if(strncmp(buffer, "DownArrow", strlen(buffer)) == 0) {
		markerType = AnalyzerResults::DownArrow;
	} else if(strncmp(buffer, "X", strlen(buffer)) == 0) {
		markerType = AnalyzerResults::X;
	} else if(strncmp(buffer, "ErrorX", strlen(buffer)) == 0) {
		markerType = AnalyzerResults::ErrorX;
	} else if(strncmp(buffer, "Start", strlen(buffer)) == 0) {
		markerType = AnalyzerResults::Start;
	} else if(strncmp(buffer, "Stop", strlen(buffer)) == 0) {
		markerType = AnalyzerResults::Stop;
	} else if(strncmp(buffer, "One", strlen(buffer)) == 0) {
		markerType = AnalyzerResults::One;
	} else if(strncmp(buffer, "Zero", strlen(buffer)) == 0) {
		markerType = AnalyzerResults::Zero;
	} else if(strncmp(buffer, "Dot", strlen(buffer)) == 0) {
		markerType = AnalyzerResults::Dot;
	}

	return markerType;
}

EnrichableAnalyzerSubprocess::Marker::Marker(
	U8 _sampleNumber, std::string _channelName, AnalyzerResults::MarkerType _markerType
) {
	sampleNumber = _sampleNumber;
	channelName = _channelName;
	markerType = _markerType;
}
