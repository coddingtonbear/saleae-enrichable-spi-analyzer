#ifndef SPI_ANALYZER_H
#define SPI_ANALYZER_H

#include <Analyzer.h>
#include "EnrichableSpiAnalyzerResults.h"
#include "EnrichableSpiSimulationDataGenerator.h"

#define BUBBLE_PREFIX "bubble"
#define MARKER_PREFIX "marker"
#define TABULAR_PREFIX "tabular"
#define FEATURE_PREFIX "feature"

#define MOSI_PREFIX "mosi"
#define MISO_PREFIX "miso"

#define UNIT_SEPARATOR '\t'
#define LINE_SEPARATOR '\n'

class EnrichableSpiAnalyzerSettings;
class EnrichableSpiAnalyzer : public Analyzer2
{
public:
	EnrichableSpiAnalyzer();
	virtual ~EnrichableSpiAnalyzer();
	virtual void SetupResults();
	virtual void WorkerThread();

	virtual U32 GenerateSimulationData( U64 newest_sample_requested, U32 sample_rate, SimulationChannelDescriptor** simulation_channels );
	virtual U32 GetMinimumSampleRateHz();

	virtual const char* GetAnalyzerName() const;
	virtual bool NeedsRerun();

	// Script communication functions
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

	bool featureMarker;
	bool featureBubble;
	bool featureTabular;
protected: //functions
	void StartSubprocess();
	void StopSubprocess();

	void Setup();
	void AdvanceToActiveEnableEdge();
	bool IsInitialClockPolarityCorrect();
	void AdvanceToActiveEnableEdgeWithCorrectClockPolarity();
	bool WouldAdvancingTheClockToggleEnable();
	void GetWord();

#pragma warning( push )
#pragma warning( disable : 4251 ) //warning C4251: 'SerialAnalyzer::<...>' : class <...> needs to have dll-interface to be used by clients of class
protected:  //vars
	std::auto_ptr< EnrichableSpiAnalyzerSettings > mSettings;
	std::auto_ptr< EnrichableSpiAnalyzerResults > mResults;
	bool mSimulationInitilized;
	EnrichableSpiSimulationDataGenerator mSimulationDataGenerator;

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
	U8 packetFrameIndex = 0;

#pragma warning( pop )
};

extern "C" ANALYZER_EXPORT const char* __cdecl GetAnalyzerName();
extern "C" ANALYZER_EXPORT Analyzer* __cdecl CreateAnalyzer( );
extern "C" ANALYZER_EXPORT void __cdecl DestroyAnalyzer( Analyzer* analyzer );

#endif //SPI_ANALYZER_H
