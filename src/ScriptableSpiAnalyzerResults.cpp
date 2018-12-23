#include "ScriptableSpiAnalyzerResults.h"
#include <AnalyzerHelpers.h>
#include "ScriptableSpiAnalyzer.h"
#include "ScriptableSpiAnalyzerSettings.h"

#include <iostream>
#include <sstream>
#include <string>

#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/prctl.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <wordexp.h>

#pragma warning(disable: 4996) //warning C4996: 'sprintf': This function or variable may be unsafe. Consider using sprintf_s instead.

ScriptableSpiAnalyzerResults::ScriptableSpiAnalyzerResults( ScriptableSpiAnalyzer* analyzer, ScriptableSpiAnalyzerSettings* settings )
:	AnalyzerResults(),
	mSettings( settings ),
	mAnalyzer( analyzer )
{
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
	commandPid = fork();

	if(commandPid == 0) {
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
		if(dup2(inpipefd[1], STDERR_FILENO) < 0) {
			std::cerr << "Failed to redirect STDERR: ";
			std::cerr << errno;
			std::cerr << "\n";
			exit(errno);
		}

		prctl(PR_SET_PDEATHSIG, SIGTERM);

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
}

ScriptableSpiAnalyzerResults::~ScriptableSpiAnalyzerResults()
{
	close(inpipefd[0]);
	close(outpipefd[1]);

	kill(commandPid, SIGKILL);
}

bool ScriptableSpiAnalyzerResults::GetInputLine(char* buffer, uint bufferLength) {
	uint bufferPos = 0;
	bool foundNewline = false;

	while(!foundNewline) {
		int result = read(inpipefd[0], &buffer[bufferPos], 1);
		if(result) {
			if(buffer[bufferPos] == '\n') {
				foundNewline = true;
			}

			bufferPos++;
		}
	}
	buffer[bufferPos] = '\0';

	if(strlen(buffer) == 1) {  // If only newline; return False
		return false;
	}
	return true;
}

bool ScriptableSpiAnalyzerResults::HandleInput() {
	bool gotResultString = false;

	char inputBuffer[512];
	while(GetInputLine(inputBuffer, 512)) {
		if(strncmp(inputBuffer, CMD_BUBBLE, strlen(CMD_BUBBLE)) == 0) {
			AddResultString(
				strchr(inputBuffer, UNIT_SEPARATOR) + 1
			);
			gotResultString = true;
		}
	}

	return gotResultString;
}

void ScriptableSpiAnalyzerResults::GenerateBubbleText( U64 frame_index, Channel& channel, DisplayBase display_base )  //unrefereced vars commented out to remove warnings.
{
	ClearResultStrings();
	Frame frame = GetFrame( frame_index );

	std::stringstream outputStream;

	if( ( frame.mFlags & SPI_ERROR_FLAG ) == 0 )
	{
		outputStream << RESULT_PREFIX;
		outputStream << UNIT_SEPARATOR;
		outputStream << std::hex << frame_index;
		outputStream << UNIT_SEPARATOR;
		outputStream << std::hex << frame.mStartingSampleInclusive;
		outputStream << UNIT_SEPARATOR;
		outputStream << std::hex << frame.mEndingSampleInclusive;
		outputStream << UNIT_SEPARATOR;

		bool gotResultString = false;
		if( channel == mSettings->mMosiChannel )
		{
			outputStream << MOSI_PREFIX;
			outputStream << UNIT_SEPARATOR;
			outputStream << std::hex << frame.mData1;
			outputStream << LINE_SEPARATOR;

			std::string value = outputStream.str();
			write(outpipefd[1], value.c_str(), value.length());

			gotResultString = HandleInput();

		}else
		{
			outputStream << MISO_PREFIX;
			outputStream << UNIT_SEPARATOR;
			outputStream << std::hex << frame.mData2;
			outputStream << LINE_SEPARATOR;

			std::string value = outputStream.str();
			write(outpipefd[1], value.c_str(), value.length());

			gotResultString = HandleInput();
		}
		if(!gotResultString) {
			AddResultString( " " );
		}
	}else
	{
			AddResultString( "Error" );
			AddResultString( "Settings mismatch" );
			AddResultString( "The initial (idle) state of the CLK line does not match the settings." );
	}
}

void ScriptableSpiAnalyzerResults::GenerateExportFile( const char* file, DisplayBase display_base, U32 /*export_type_user_id*/ )
{
	//export_type_user_id is only important if we have more than one export type.


	std::stringstream ss;
	void* f = AnalyzerHelpers::StartFile( file );

	U64 trigger_sample = mAnalyzer->GetTriggerSample();
	U32 sample_rate = mAnalyzer->GetSampleRate();

	ss << "Time [s],Packet ID,MOSI,MISO" << std::endl;

	bool mosi_used = true;
	bool miso_used = true;

	if( mSettings->mMosiChannel == UNDEFINED_CHANNEL )
		mosi_used = false;

	if( mSettings->mMisoChannel == UNDEFINED_CHANNEL )
		miso_used = false;

	U64 num_frames = GetNumFrames();
	for( U32 i=0; i < num_frames; i++ )
	{
		Frame frame = GetFrame( i );

		if( ( frame.mFlags & SPI_ERROR_FLAG ) != 0 )
			continue;
		
		char time_str[128];
		AnalyzerHelpers::GetTimeString( frame.mStartingSampleInclusive, trigger_sample, sample_rate, time_str, 128 );

		char mosi_str[128] = "";
		if( mosi_used == true )
			AnalyzerHelpers::GetNumberString( frame.mData1, display_base, mSettings->mBitsPerTransfer, mosi_str, 128 );

		char miso_str[128] = "";
		if( miso_used == true )
			AnalyzerHelpers::GetNumberString( frame.mData2, display_base, mSettings->mBitsPerTransfer, miso_str, 128 );

		U64 packet_id = GetPacketContainingFrameSequential( i ); 
		if( packet_id != INVALID_RESULT_INDEX )
			ss << time_str << "," << packet_id << "," << mosi_str << "," << miso_str << std::endl;
		else
			ss << time_str << ",," << mosi_str << "," << miso_str << std::endl;  //it's ok for a frame not to be included in a packet.
	
		AnalyzerHelpers::AppendToFile( (U8*)ss.str().c_str(), ss.str().length(), f );
		ss.str( std::string() );
							
		if( UpdateExportProgressAndCheckForCancel( i, num_frames ) == true )
		{
			AnalyzerHelpers::EndFile( f );
			return;
		}
	}

	UpdateExportProgressAndCheckForCancel( num_frames, num_frames );
	AnalyzerHelpers::EndFile( f );
}

void ScriptableSpiAnalyzerResults::GenerateFrameTabularText( U64 frame_index, DisplayBase display_base )
{
	ClearTabularText();
	Frame frame = GetFrame( frame_index );

	bool mosi_used = true;
	bool miso_used = true;

	if( mSettings->mMosiChannel == UNDEFINED_CHANNEL )
		mosi_used = false;

	if( mSettings->mMisoChannel == UNDEFINED_CHANNEL )
		miso_used = false;

	char mosi_str[128];
	char miso_str[128];

	std::stringstream ss;

	if( ( frame.mFlags & SPI_ERROR_FLAG ) == 0 )
	{
		if( mosi_used == true )
			AnalyzerHelpers::GetNumberString( frame.mData1, display_base, mSettings->mBitsPerTransfer, mosi_str, 128 );
		if( miso_used == true )
			AnalyzerHelpers::GetNumberString( frame.mData2, display_base, mSettings->mBitsPerTransfer, miso_str, 128 );

		if( mosi_used == true && miso_used == true )
		{
			ss << "MOSI: " << mosi_str << ";  MISO: " << miso_str;
		}else
		{
			if( mosi_used == true )
			{
				ss << "MOSI: " << mosi_str;
			}else
			{
				ss << "MISO: " << miso_str;
			}
		}
	}
	else
	{
		ss << "The initial (idle) state of the CLK line does not match the settings.";
	}

	AddTabularText( ss.str().c_str() );
}

void ScriptableSpiAnalyzerResults::GeneratePacketTabularText( U64 /*packet_id*/, DisplayBase /*display_base*/ )  //unrefereced vars commented out to remove warnings.
{
	ClearResultStrings();
	AddResultString( "not supported" );
}

void ScriptableSpiAnalyzerResults::GenerateTransactionTabularText( U64 /*transaction_id*/, DisplayBase /*display_base*/ )  //unrefereced vars commented out to remove warnings.
{
	ClearResultStrings();
	AddResultString( "not supported" );
}
