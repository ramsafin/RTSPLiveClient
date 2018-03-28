#include "HEVCRTSPClient.hpp"

HEVCRTSPClient *HEVCRTSPClient::createNew(UsageEnvironment &env, char const *rtspURL, int verbosityLevel,
                                        char const *applicationName, portNumBits tunnelOverHTTPPortNum) {
    return new HEVCRTSPClient(env, rtspURL, verbosityLevel, applicationName, tunnelOverHTTPPortNum);
}

HEVCRTSPClient::HEVCRTSPClient(UsageEnvironment &env, char const *rtspURL, int verbosityLevel,
                             char const *applicationName, portNumBits tunnelOverHTTPPortNum)
        : RTSPClient(env, rtspURL, verbosityLevel, applicationName, tunnelOverHTTPPortNum, -1) {}

HEVCRTSPClient::~HEVCRTSPClient() = default;
