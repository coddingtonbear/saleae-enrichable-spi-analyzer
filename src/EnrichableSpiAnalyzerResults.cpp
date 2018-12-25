#include "EnrichableSpiAnalyzerResults.h"
#include <AnalyzerHelpers.h>
#include "EnrichableSpiAnalyzer.h"
#include "EnrichableSpiAnalyzerSettings.h"

#include <iostream>
#include <sstream>

#pragma warning(disable: 4996) //warning C4996: 'sprintf': This function or variable may be unsafe. Consider using sprintf_s instead.

EnrichableSpiAnalyzerResults::EnrichableSpiAnalyzerResults( EnrichableSpiAnalyzer* analyzer, EnrichableSpiAnalyzerSettings* settings )
:	AnalyzerResults(),
	mSettings( settings ),
	mAnalyzer( analyzer )
{
}

EnrichableSpiAnalyzerResults::~EnrichableSpiAnalyzerResults()
{
}

void EnrichableSpiAnalyzerResults::GenerateBubbleText( U64 frame_index, Channel& channel, DisplayBase display_base )  //unrefereced vars commented out to remove warnings.
{
	ClearResultStrings();
	Frame frame = GetFrame( frame_index );

	std::stringstream outputStream;

	if( ( frame.mFlags & SPI_ERROR_FLAG ) == 0 )
	{
		if(mAnalyzer->featureBubble) {
			outputStream << BUBBLE_PREFIX;
			outputStream << UNIT_SEPARATOR;
			outputStream << std::hex << frame_index;
			outputStream << UNIT_SEPARATOR;
			outputStream << std::hex << frame.mStartingSampleInclusive;
			outputStream << UNIT_SEPARATOR;
			outputStream << std::hex << frame.mEndingSampleInclusive;
			outputStream << UNIT_SEPARATOR;
			outputStream << std::hex << (U64)frame.mType;
			outputStream << UNIT_SEPARATOR;
			outputStream << std::hex << (U64)frame.mFlags;
			outputStream << UNIT_SEPARATOR;

			char bubbleText[256];
			if( channel == mSettings->mMosiChannel )
			{
				outputStream << MOSI_PREFIX;
				outputStream << UNIT_SEPARATOR;
				outputStream << std::hex << frame.mData1;
				outputStream << LINE_SEPARATOR;

				std::string value = outputStream.str();
				mAnalyzer->GetScriptResponse(
					value.c_str(),
					value.length(),
					bubbleText,
					256
				);
			}
			else
			{
				outputStream << MISO_PREFIX;
				outputStream << UNIT_SEPARATOR;
				outputStream << std::hex << frame.mData2;
				outputStream << LINE_SEPARATOR;

				std::string value = outputStream.str();
				mAnalyzer->GetScriptResponse(
					value.c_str(),
					value.length(),
					bubbleText,
					256
				);
			}
			if(strlen(bubbleText) > 0) {
				AddResultString(bubbleText);
			}
		} else {
			if( channel == mSettings->mMosiChannel )
			{
				char number_str[128];
				AnalyzerHelpers::GetNumberString( frame.mData1, display_base, mSettings->mBitsPerTransfer, number_str, 128 );
				AddResultString( number_str );
			}else
			{
				char number_str[128];
				AnalyzerHelpers::GetNumberString( frame.mData2, display_base, mSettings->mBitsPerTransfer, number_str, 128 );
				AddResultString( number_str );
			}
		}
	}else
	{
			AddResultString( "Error" );
			AddResultString( "Settings mismatch" );
			AddResultString( "The initial (idle) state of the CLK line does not match the settings." );
	}
}

void EnrichableSpiAnalyzerResults::GenerateExportFile( const char* file, DisplayBase display_base, U32 /*export_type_user_id*/ )
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

void EnrichableSpiAnalyzerResults::GenerateFrameTabularText( U64 frame_index, DisplayBase display_base )
{
	ClearTabularText();
	Frame frame = GetFrame( frame_index );

	if(mAnalyzer->featureTabular) {
		std::stringstream outputStream;

		outputStream << TABULAR_PREFIX;
		outputStream << UNIT_SEPARATOR;
		outputStream << std::hex << frame_index;
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
		char tabularText[512];
		mAnalyzer->GetScriptResponse(
			value.c_str(),
			value.length(),
			tabularText,
			512
		);

		if(strlen(tabularText) > 0) {
			AddTabularText(tabularText);
		}
	} else {
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
}

void EnrichableSpiAnalyzerResults::GeneratePacketTabularText( U64 /*packet_id*/, DisplayBase /*display_base*/ )  //unrefereced vars commented out to remove warnings.
{
	ClearResultStrings();
	AddResultString( "not supported" );
}

void EnrichableSpiAnalyzerResults::GenerateTransactionTabularText( U64 /*transaction_id*/, DisplayBase /*display_base*/ )  //unrefereced vars commented out to remove warnings.
{
	ClearResultStrings();
	AddResultString( "not supported" );
}
