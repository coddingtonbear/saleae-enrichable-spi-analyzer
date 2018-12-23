#ifndef SPI_ANALYZER_RESULTS
#define SPI_ANALYZER_RESULTS

#include <AnalyzerResults.h>

#define SPI_ERROR_FLAG ( 1 << 0 )

#define FRAMEDATA_PREFIX "frame"
#define RESULT_PREFIX "result"

#define MOSI_PREFIX "mosi"
#define MISO_PREFIX "miso"

#define UNIT_SEPARATOR '\t'
#define LINE_SEPARATOR '\n'

#define CMD_BUBBLE "bubble"

class ScriptableSpiAnalyzer;
class ScriptableSpiAnalyzerSettings;

class ScriptableSpiAnalyzerResults : public AnalyzerResults
{
public:
	ScriptableSpiAnalyzerResults( ScriptableSpiAnalyzer* analyzer, ScriptableSpiAnalyzerSettings* settings );
	virtual ~ScriptableSpiAnalyzerResults();

	virtual void GenerateBubbleText( U64 frame_index, Channel& channel, DisplayBase display_base );
	virtual void GenerateExportFile( const char* file, DisplayBase display_base, U32 export_type_user_id );

	virtual void GenerateFrameTabularText(U64 frame_index, DisplayBase display_base );
	virtual void GeneratePacketTabularText( U64 packet_id, DisplayBase display_base );
	virtual void GenerateTransactionTabularText( U64 transaction_id, DisplayBase display_base );

protected: //functions
	bool GetInputLine(char* buffer, uint bufferLength);
	bool HandleInput();

protected:  //vars
	ScriptableSpiAnalyzerSettings* mSettings;
	ScriptableSpiAnalyzer* mAnalyzer;

	pid_t commandPid = 0;
	int inpipefd[2];
	int outpipefd[2];
};

#endif //SPI_ANALYZER_RESULTS
