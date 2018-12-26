#pragma once

#include "AnalyzerResults.h"
#include <vector>
#include <string>

//#define SUBPROCESS_DEBUG

#define BUBBLE_PREFIX "bubble"
#define MARKER_PREFIX "marker"
#define TABULAR_PREFIX "tabular"
#define FEATURE_PREFIX "feature"

#define UNIT_SEPARATOR '\t'
#define LINE_SEPARATOR '\n'

class EnrichableAnalyzerSubprocess {
	public:
		struct Marker {
			Marker(U8 sampleNumber, std::string channelName, AnalyzerResults::MarkerType markerType);

			U8 sampleNumber;
			std::string channelName;
			AnalyzerResults::MarkerType markerType;
		};

		EnrichableAnalyzerSubprocess();
		virtual ~EnrichableAnalyzerSubprocess();

		void SetParserCommand(std::string);

		std::vector<Marker> EmitMarker(U64 packetId, U64 frameIndex, Frame& frame, U32 sampleCount);
		std::vector<std::string> EmitBubble(U64 packetId, U64 frameIndex, Frame& frame, std::string channelName);
		std::vector<std::string> EmitTabular(U64 packetId, U64 frameIndex, Frame& frame);

		bool MarkerEnabled();
		bool BubbleEnabled();
		bool TabularEnabled();

		void Start();
		void Stop(int exitCode=0);
	protected:
		void Terminate();

		bool GetScriptResponse(
			const char* outBuffer,
			unsigned outBufferLength,
			char* inBuffer,
			unsigned inBufferLength
		);
		bool SendOutputLine(const char* buffer, unsigned bufferLength);
		bool GetInputLine(char* buffer, unsigned bufferLength);
		void LockSubprocess();
		void UnlockSubprocess();
		bool GetFeatureEnablement(const char* feature);
		AnalyzerResults::MarkerType GetMarkerType(char* buffer, unsigned bufferLength);

		std::string parserCommand;
		bool enabled;

		bool featureMarker;
		bool featureBubble;
		bool featureTabular;

		pid_t commandPid = 0;
		int inpipefd[2];
		int outpipefd[2];
};
