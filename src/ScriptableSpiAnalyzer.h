#ifndef SPI_ANALYZER_H
#define SPI_ANALYZER_H

#include <Analyzer.h>
#include "ScriptableSpiAnalyzerResults.h"
#include "ScriptableSpiSimulationDataGenerator.h"

#define FRAMEDATA_PREFIX "frame"
#define BUBBLE_PREFIX "bubble"
#define MARKER_PREFIX "marker"

#define MOSI_PREFIX "mosi"
#define MISO_PREFIX "miso"

#define UNIT_SEPARATOR '\t'
#define LINE_SEPARATOR '\n'

#define CMD_BUBBLE "bubble"
#define CMD_MARKER "marker"

class ScriptableSpiAnalyzerSettings;
class ScriptableSpiAnalyzer : public Analyzer2
{
public:
	ScriptableSpiAnalyzer();
	virtual ~ScriptableSpiAnalyzer();
	virtual void SetupResults();
	virtual void WorkerThread();

	virtual U32 GenerateSimulationData( U64 newest_sample_requested, U32 sample_rate, SimulationChannelDescriptor** simulation_channels );
	virtual U32 GetMinimumSampleRateHz();

	virtual const char* GetAnalyzerName() const;
	virtual bool NeedsRerun();

	// Script communication functions
	bool GetScriptResponse(
		const char* outBuffer,
		uint outBufferLength,
		char* inBuffer,
		uint inBufferLength
	);
	AnalyzerResults::MarkerType GetMarkerType(char* buffer, uint bufferLength);

protected: //functions
	bool SendOutputLine(const char* buffer, uint bufferLength);
	bool GetInputLine(char* buffer, uint bufferLength);

	void Setup();
	void AdvanceToActiveEnableEdge();
	bool IsInitialClockPolarityCorrect();
	void AdvanceToActiveEnableEdgeWithCorrectClockPolarity();
	bool WouldAdvancingTheClockToggleEnable();
	void GetWord();

#pragma warning( push )
#pragma warning( disable : 4251 ) //warning C4251: 'SerialAnalyzer::<...>' : class <...> needs to have dll-interface to be used by clients of class
protected:  //vars
	std::auto_ptr< ScriptableSpiAnalyzerSettings > mSettings;
	std::auto_ptr< ScriptableSpiAnalyzerResults > mResults;
	bool mSimulationInitilized;
	ScriptableSpiSimulationDataGenerator mSimulationDataGenerator;

	AnalyzerChannelData* mMosi; 
	AnalyzerChannelData* mMiso;
	AnalyzerChannelData* mClock;
	AnalyzerChannelData* mEnable;

	U64 mCurrentSample;
	AnalyzerResults::MarkerType mArrowMarker;
	std::vector<U64> mArrowLocations;

	pid_t commandPid = 0;
	int inpipefd[2];
	int outpipefd[2];

#pragma warning( pop )
};

extern "C" ANALYZER_EXPORT const char* __cdecl GetAnalyzerName();
extern "C" ANALYZER_EXPORT Analyzer* __cdecl CreateAnalyzer( );
extern "C" ANALYZER_EXPORT void __cdecl DestroyAnalyzer( Analyzer* analyzer );

#endif //SPI_ANALYZER_H