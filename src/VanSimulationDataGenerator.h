#ifndef VAN_SIMULATION_DATA_GENERATOR
#define VAN_SIMULATION_DATA_GENERATOR

#include <SimulationChannelDescriptor.h>
#include <string>
class VanAnalyzerSettings;

class VanSimulationDataGenerator
{
public:
	VanSimulationDataGenerator();
	~VanSimulationDataGenerator();

	void Initialize( U32 simulation_sample_rate, VanAnalyzerSettings* settings );
	U32 GenerateSimulationData( U64 newest_sample_requested, U32 sample_rate, SimulationChannelDescriptor** simulation_channel );

protected:
	VanAnalyzerSettings* mSettings;
	U32 mSimulationSampleRateHz;

protected:
	void CreateSerialByte();
	std::string mSerialText;
	U32 mStringIndex;

	SimulationChannelDescriptor mSerialSimulationData;

};
#endif //VAN_SIMULATION_DATA_GENERATOR