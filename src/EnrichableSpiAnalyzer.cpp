
#include "EnrichableSpiAnalyzer.h"
#include "EnrichableSpiAnalyzerSettings.h"
#include <AnalyzerChannelData.h>

#include <iostream>
#include <sstream>
#include <string>
#include <mutex>

#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/prctl.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <wordexp.h>

 
//enum SpiBubbleType { SpiData, SpiError };

std::mutex subprocessLock;

EnrichableSpiAnalyzer::EnrichableSpiAnalyzer()
:	Analyzer2(),
	mSettings( new EnrichableSpiAnalyzerSettings() ),
	mSimulationInitilized( false ),
	mMosi( NULL ),
	mMiso( NULL ),
	mClock( NULL ),
	mEnable( NULL ),
	featureMarker(true),
	featureBubble(true),
	featureTabular(true)
{	
	SetAnalyzerSettings( mSettings.get() );
}

EnrichableSpiAnalyzer::~EnrichableSpiAnalyzer()
{
	KillThread();
}

void EnrichableSpiAnalyzer::SetupResults()
{
	mResults.reset( new EnrichableSpiAnalyzerResults( this, mSettings.get() ) );
	SetAnalyzerResults( mResults.get() );

	if( mSettings->mMosiChannel != UNDEFINED_CHANNEL )
		mResults->AddChannelBubblesWillAppearOn( mSettings->mMosiChannel );
	if( mSettings->mMisoChannel != UNDEFINED_CHANNEL )
		mResults->AddChannelBubblesWillAppearOn( mSettings->mMisoChannel );
}

void EnrichableSpiAnalyzer::WorkerThread()
{
	Setup();

	AdvanceToActiveEnableEdgeWithCorrectClockPolarity();

	if(pipe(inpipefd) < 0) {
		std::cerr << "Failed to create input pipe: ";
		std::cerr << errno;
		std::cerr << "\n";
		exit(errno);
	}
	if(pipe(outpipefd) < 0) {
		std::cerr << "Failed to create output pipe: ";
		std::cerr << errno;
		std::cerr << "\n";
		exit(errno);
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

		prctl(PR_SET_PDEATHSIG, SIGINT);

		wordexp_t cmdParsed;
		char *args[25];

		wordexp(mSettings->mParserCommand, &cmdParsed, 0);
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

	for( ; ; )
	{
		GetWord();
		CheckIfThreadShouldExit();
	}

	close(inpipefd[0]);
	close(outpipefd[1]);

	kill(commandPid, SIGINT);
}

bool EnrichableSpiAnalyzer::GetFeatureEnablement(const char* feature) {
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

void EnrichableSpiAnalyzer::LockSubprocess() {
	subprocessLock.lock();
}

void EnrichableSpiAnalyzer::UnlockSubprocess() {
	subprocessLock.unlock();
}

void EnrichableSpiAnalyzer::AdvanceToActiveEnableEdgeWithCorrectClockPolarity()
{
	mResults->CommitPacketAndStartNewPacket();
	mResults->CommitResults();
	
	AdvanceToActiveEnableEdge();

	for( ; ; )
	{
		if( IsInitialClockPolarityCorrect() == true )  //if false, this function moves to the next active enable edge.
			break;
	}
}

void EnrichableSpiAnalyzer::Setup()
{
	bool allow_last_trailing_clock_edge_to_fall_outside_enable = false;
	if( mSettings->mDataValidEdge == AnalyzerEnums::LeadingEdge )
		allow_last_trailing_clock_edge_to_fall_outside_enable = true;

	if( mSettings->mClockInactiveState == BIT_LOW )
	{
		if( mSettings->mDataValidEdge == AnalyzerEnums::LeadingEdge )
			mArrowMarker = AnalyzerResults::UpArrow;
		else
			mArrowMarker = AnalyzerResults::DownArrow;

	}else
	{
		if( mSettings->mDataValidEdge == AnalyzerEnums::LeadingEdge )
			mArrowMarker = AnalyzerResults::DownArrow;
		else
			mArrowMarker = AnalyzerResults::UpArrow;
	}


	if( mSettings->mMosiChannel != UNDEFINED_CHANNEL )
		mMosi = GetAnalyzerChannelData( mSettings->mMosiChannel );
	else
		mMosi = NULL;

	if( mSettings->mMisoChannel != UNDEFINED_CHANNEL )
		mMiso = GetAnalyzerChannelData( mSettings->mMisoChannel );
	else
		mMiso = NULL;


	mClock = GetAnalyzerChannelData( mSettings->mClockChannel );

	if( mSettings->mEnableChannel != UNDEFINED_CHANNEL )
		mEnable = GetAnalyzerChannelData( mSettings->mEnableChannel );
	else
		mEnable = NULL;

}

void EnrichableSpiAnalyzer::AdvanceToActiveEnableEdge()
{
	if( mEnable != NULL )
	{
		if( mEnable->GetBitState() != mSettings->mEnableActiveState )
		{
			mEnable->AdvanceToNextEdge();
		}else
		{
			mEnable->AdvanceToNextEdge();
			mEnable->AdvanceToNextEdge();
		}
		mCurrentSample = mEnable->GetSampleNumber();
		mClock->AdvanceToAbsPosition( mCurrentSample );
		packetFrameIndex = 0;
	}else
	{
		mCurrentSample = mClock->GetSampleNumber();
	}
}

bool EnrichableSpiAnalyzer::IsInitialClockPolarityCorrect()
{
	if( mClock->GetBitState() == mSettings->mClockInactiveState )
		return true;

	mResults->AddMarker( mCurrentSample, AnalyzerResults::ErrorSquare, mSettings->mClockChannel );

	if( mEnable != NULL )
	{
		Frame error_frame;
		error_frame.mStartingSampleInclusive = mCurrentSample;

		mEnable->AdvanceToNextEdge();
		mCurrentSample = mEnable->GetSampleNumber();

		error_frame.mEndingSampleInclusive = mCurrentSample;
		error_frame.mFlags = SPI_ERROR_FLAG | DISPLAY_AS_ERROR_FLAG;
		mResults->AddFrame( error_frame );
		mResults->CommitResults();
		ReportProgress( error_frame.mEndingSampleInclusive );

		//move to the next active-going enable edge
		mEnable->AdvanceToNextEdge();
		mCurrentSample = mEnable->GetSampleNumber();
		mClock->AdvanceToAbsPosition( mCurrentSample );

		return false;
	}else
	{
		mClock->AdvanceToNextEdge();  //at least start with the clock in the idle state.
		mCurrentSample = mClock->GetSampleNumber();
		return true;
	}
}

bool EnrichableSpiAnalyzer::WouldAdvancingTheClockToggleEnable()
{
	if( mEnable == NULL )
		return false;

	U64 next_edge = mClock->GetSampleOfNextEdge();
	bool enable_will_toggle = mEnable->WouldAdvancingToAbsPositionCauseTransition( next_edge );

	if( enable_will_toggle == false )
		return false;
	else
		return true;
}

void EnrichableSpiAnalyzer::GetWord()
{
	//we're assuming we come into this function with the clock in the idle state;

	U32 bits_per_transfer = mSettings->mBitsPerTransfer;

	DataBuilder mosi_result;
	U64 mosi_word = 0;
	mosi_result.Reset( &mosi_word, mSettings->mShiftOrder, bits_per_transfer );

	DataBuilder miso_result;
	U64 miso_word = 0;
	miso_result.Reset( &miso_word, mSettings->mShiftOrder, bits_per_transfer );

	U64 first_sample = 0;
	bool need_reset = false;

	mArrowLocations.clear();
	ReportProgress( mClock->GetSampleNumber() );

	for( U32 i=0; i<bits_per_transfer; i++ )
	{
		if( i == 0 )
			CheckIfThreadShouldExit();

		//on every single edge, we need to check that enable doesn't toggle.
		//note that we can't just advance the enable line to the next edge, becuase there may not be another edge

		if( WouldAdvancingTheClockToggleEnable() == true )
		{
			AdvanceToActiveEnableEdgeWithCorrectClockPolarity();  //ok, we pretty much need to reset everything and return.
			return;
		}
		
		mClock->AdvanceToNextEdge();
		if( i == 0 )
			first_sample = mClock->GetSampleNumber();

		if( mSettings->mDataValidEdge == AnalyzerEnums::LeadingEdge )
		{
			mCurrentSample = mClock->GetSampleNumber();
			if( mMosi != NULL )
			{
				mMosi->AdvanceToAbsPosition( mCurrentSample );
				mosi_result.AddBit( mMosi->GetBitState() );
			}
			if( mMiso != NULL )
			{
				mMiso->AdvanceToAbsPosition( mCurrentSample );
				miso_result.AddBit( mMiso->GetBitState() );
			}
			mArrowLocations.push_back( mCurrentSample );
		}


		// ok, the trailing edge is messy -- but only on the very last bit.
		// If the trialing edge isn't doesn't represent valid data, we want to allow the enable line to rise before the clock trialing edge -- and still report the frame
		if( ( i == ( bits_per_transfer - 1 ) ) && ( mSettings->mDataValidEdge != AnalyzerEnums::TrailingEdge ) )
		{
			//if this is the last bit, and the trailing edge doesn't represent valid data
			if( WouldAdvancingTheClockToggleEnable() == true )
			{
				//moving to the trailing edge would cause the clock to revert to inactive.  jump out, record the frame, and them move to the next active enable edge
				need_reset = true;
				break;
			}
			
			//enable isn't going to go inactive, go ahead and advance the clock as usual.  Then we're done, jump out and record the frame.
			mClock->AdvanceToNextEdge();
			break;
		}
		
		//this isn't the very last bit, etc, so proceed as normal
		if( WouldAdvancingTheClockToggleEnable() == true )
		{
			AdvanceToActiveEnableEdgeWithCorrectClockPolarity();  //ok, we pretty much need to reset everything and return.
			return;
		}

		mClock->AdvanceToNextEdge();

		if( mSettings->mDataValidEdge == AnalyzerEnums::TrailingEdge )
		{
			mCurrentSample = mClock->GetSampleNumber();
			if( mMosi != NULL )
			{
				mMosi->AdvanceToAbsPosition( mCurrentSample );
				mosi_result.AddBit( mMosi->GetBitState() );
			}
			if( mMiso != NULL )
			{
				mMiso->AdvanceToAbsPosition( mCurrentSample );
				miso_result.AddBit( mMiso->GetBitState() );
			}
			mArrowLocations.push_back( mCurrentSample );
		}
		
	}

	Frame result_frame;
	result_frame.mStartingSampleInclusive = first_sample;
	result_frame.mEndingSampleInclusive = mClock->GetSampleNumber();
	result_frame.mData1 = mosi_word;
	result_frame.mData2 = miso_word;
	result_frame.mFlags = 0;
	result_frame.mType = packetFrameIndex++;
	U64 frame_index = mResults->AddFrame( result_frame );

	//save the resuls:
	U32 count = mArrowLocations.size();
	for( U32 i=0; i<count; i++ ) {
		mResults->AddMarker(
			mArrowLocations[i], mArrowMarker, mSettings->mClockChannel
		);
	}

	if(featureMarker) {
		U64 packet_id = mResults->GetPacketContainingFrameSequential( frame_index );
		std::stringstream outputStream;

		outputStream << MARKER_PREFIX;
		outputStream << UNIT_SEPARATOR;
		if(packet_id != INVALID_RESULT_INDEX) {
			outputStream << std::hex << packet_id;
		}
		outputStream << UNIT_SEPARATOR;
		outputStream << std::hex << frame_index;
		outputStream << UNIT_SEPARATOR;
		outputStream << std::hex << count;
		outputStream << UNIT_SEPARATOR;
		outputStream << std::hex << result_frame.mStartingSampleInclusive;
		outputStream << UNIT_SEPARATOR;
		outputStream << std::hex << result_frame.mEndingSampleInclusive;
		outputStream << UNIT_SEPARATOR;
		outputStream << std::hex << (U64)result_frame.mType;
		outputStream << UNIT_SEPARATOR;
		outputStream << std::hex << (U64)result_frame.mFlags;
		outputStream << UNIT_SEPARATOR;
		outputStream << std::hex << mosi_word;
		outputStream << UNIT_SEPARATOR;
		outputStream << std::hex << miso_word;
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
					Channel* channel = NULL;
					if(strcmp(channelStr, "mosi") == 0) {
						channel = &mSettings->mMosiChannel;
					} else if (strcmp(channelStr, "miso") == 0) {
						channel = &mSettings->mMisoChannel;
					}
					if(channel != NULL) {
						mResults->AddMarker(
							mArrowLocations[sampleNumber],
							GetMarkerType(markerTypeStr, strlen(markerTypeStr)),
							*channel
						);
					}
				} else {
					std::cerr << "Unable to tokenize marker message input: \"";
					std::cerr << forever;
					std::cerr << "\"; input should be three tab-delimited fields: ";
					std::cerr << "sample_number\tchannel\tmarker_type\n";
				}
			} else {
				break;
			}
		}
		UnlockSubprocess();
	}
	
	mResults->CommitResults();

	if( need_reset == true )
		AdvanceToActiveEnableEdgeWithCorrectClockPolarity();
}

bool EnrichableSpiAnalyzer::GetScriptResponse(
	const char* outBuffer,
	uint outBufferLength,
	char* inBuffer,
	uint inBufferLength
) {
	LockSubprocess();
	SendOutputLine(outBuffer, outBufferLength);
	GetInputLine(inBuffer, inBufferLength);
	UnlockSubprocess();
}

bool EnrichableSpiAnalyzer::SendOutputLine(const char* buffer, uint bufferLength) {
	//std::cerr << ">> ";
	//std::cerr << buffer;
	write(outpipefd[1], buffer, bufferLength);
}

bool EnrichableSpiAnalyzer::GetInputLine(char* buffer, uint bufferLength) {
	uint bufferPos = 0;

	//std::cerr << "<< ";

	while(true) {
		int result = read(inpipefd[0], &buffer[bufferPos], 1);
		if(buffer[bufferPos] == '\n') {
			break;
		}

		//std::cerr << buffer[bufferPos];

		bufferPos++;

		if(bufferPos == bufferLength - 1) {
			break;
		}
	}
	buffer[bufferPos] = '\0';

	//std::cerr << '\n';

	if(strlen(buffer) == 0) {
		return false;
	}
	return true;
}

AnalyzerResults::MarkerType EnrichableSpiAnalyzer::GetMarkerType(char* buffer, uint bufferLength) {
	if(strncmp(buffer, "ErrorDot", strlen(buffer)) == 0) {
		return AnalyzerResults::ErrorDot;
	} else if(strncmp(buffer, "Square", strlen(buffer)) == 0) {
		return AnalyzerResults::Square;
	} else if(strncmp(buffer, "ErrorSquare", strlen(buffer)) == 0) {
		return AnalyzerResults::ErrorSquare;
	} else if(strncmp(buffer, "UpArrow", strlen(buffer)) == 0) {
		return AnalyzerResults::UpArrow;
	} else if(strncmp(buffer, "DownArrow", strlen(buffer)) == 0) {
		return AnalyzerResults::DownArrow;
	} else if(strncmp(buffer, "X", strlen(buffer)) == 0) {
		return AnalyzerResults::X;
	} else if(strncmp(buffer, "ErrorX", strlen(buffer)) == 0) {
		return AnalyzerResults::ErrorX;
	} else if(strncmp(buffer, "Start", strlen(buffer)) == 0) {
		return AnalyzerResults::Start;
	} else if(strncmp(buffer, "Stop", strlen(buffer)) == 0) {
		return AnalyzerResults::Stop;
	} else if(strncmp(buffer, "One", strlen(buffer)) == 0) {
		return AnalyzerResults::One;
	} else if(strncmp(buffer, "Zero", strlen(buffer)) == 0) {
		return AnalyzerResults::Zero;
	} else if(strncmp(buffer, "Dot", strlen(buffer)) == 0) {
		return AnalyzerResults::Dot;
	}
	std::cerr << "Unrecognized marker type: ";
	std::cerr << buffer;
	std::cerr << "; using Dot instead.\n";
	return AnalyzerResults::Dot;
}

bool EnrichableSpiAnalyzer::NeedsRerun()
{
	return false;
}

U32 EnrichableSpiAnalyzer::GenerateSimulationData( U64 minimum_sample_index, U32 device_sample_rate, SimulationChannelDescriptor** simulation_channels )
{
	if( mSimulationInitilized == false )
	{
		mSimulationDataGenerator.Initialize( GetSimulationSampleRate(), mSettings.get() );
		mSimulationInitilized = true;
	}

	return mSimulationDataGenerator.GenerateSimulationData( minimum_sample_index, device_sample_rate, simulation_channels );
}


U32 EnrichableSpiAnalyzer::GetMinimumSampleRateHz()
{
	return 10000; //we don't have any idea, depends on the SPI rate, etc.; return the lowest rate.
}

const char* EnrichableSpiAnalyzer::GetAnalyzerName() const
{
	return "SPI (Enrichable)";
}

const char* GetAnalyzerName()
{
	return "SPI (Enrichable)";
}

Analyzer* CreateAnalyzer()
{
	return new EnrichableSpiAnalyzer();
}

void DestroyAnalyzer( Analyzer* analyzer )
{
	delete analyzer;
}
