#ifndef SPI_ANALYZER_RESULTS
#define SPI_ANALYZER_RESULTS

#include <AnalyzerResults.h>
#include "EnrichableAnalyzerSubprocess.h"

#define SPI_ERROR_FLAG ( 1 << 0 )

class EnrichableSpiAnalyzer;
class EnrichableSpiAnalyzerSettings;

class EnrichableSpiAnalyzerResults : public AnalyzerResults
{
public:
	EnrichableSpiAnalyzerResults( EnrichableSpiAnalyzer* analyzer, EnrichableSpiAnalyzerSettings* settings, EnrichableAnalyzerSubprocess* subprocess );
	virtual ~EnrichableSpiAnalyzerResults();

	virtual void GenerateBubbleText( U64 frame_index, Channel& channel, DisplayBase display_base );
	virtual void GenerateExportFile( const char* file, DisplayBase display_base, U32 export_type_user_id );

	virtual void GenerateFrameTabularText(U64 frame_index, DisplayBase display_base );
	virtual void GeneratePacketTabularText( U64 packet_id, DisplayBase display_base );
	virtual void GenerateTransactionTabularText( U64 transaction_id, DisplayBase display_base );

protected: //functions

protected:  //vars
	EnrichableSpiAnalyzerSettings* mSettings;
	EnrichableSpiAnalyzer* mAnalyzer;
	EnrichableAnalyzerSubprocess* mSubprocess;
};

#endif //SPI_ANALYZER_RESULTS
