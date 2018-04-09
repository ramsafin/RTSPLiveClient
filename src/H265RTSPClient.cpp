#include "H265RTSPClient.hpp"

H265RTSPClient *H265RTSPClient::createNew(UsageEnvironment &env, char const *rtspURL, int verbosityLevel,
                                        char const *applicationName, portNumBits tunnelOverHTTPPortNum) {
    return new H265RTSPClient(env, rtspURL, verbosityLevel, applicationName, tunnelOverHTTPPortNum);
}

H265RTSPClient::H265RTSPClient(UsageEnvironment &env, char const *rtspURL, int verbosityLevel,
                             char const *applicationName, portNumBits tunnelOverHTTPPortNum)
        : RTSPClient(env, rtspURL, verbosityLevel, applicationName, tunnelOverHTTPPortNum, -1) {}

H265RTSPClient::~H265RTSPClient() = default;
