#ifndef LIVERTSPCLIENT_HEVCRTSPCLIENT_HPP
#define LIVERTSPCLIENT_HEVCRTSPCLIENT_HPP

#include <liveMedia.hh>
#include "StreamClientState.hpp"


class HEVCRTSPClient : public RTSPClient {
public:
    static HEVCRTSPClient *createNew(UsageEnvironment &env, char const *rtspURL, int verbosityLevel = 0,
                                    char const *applicationName = nullptr, portNumBits tunnelOverHTTPPortNum = 0);

protected:

    HEVCRTSPClient(UsageEnvironment &env, char const *rtspURL, int verbosityLevel, char const *applicationName,
                  portNumBits tunnelOverHTTPPortNum);

    ~HEVCRTSPClient() override;

public:
    StreamClientState scs;
};

#endif //LIVERTSPCLIENT_HEVCRTSPCLIENT_HPP
