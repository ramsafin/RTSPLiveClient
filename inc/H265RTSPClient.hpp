#ifndef LIVERTSPCLIENT_HEVCRTSPCLIENT_HPP
#define LIVERTSPCLIENT_HEVCRTSPCLIENT_HPP

#include <liveMedia.hh>
#include "StreamClientState.hpp"


class H265RTSPClient : public RTSPClient {
public:
    static H265RTSPClient *createNew(UsageEnvironment &env, char const *rtspURL, int verbosityLevel = 0,
                                    char const *applicationName = nullptr, portNumBits tunnelOverHTTPPortNum = 0);

protected:

    H265RTSPClient(UsageEnvironment &env, char const *rtspURL, int verbosityLevel, char const *applicationName,
                  portNumBits tunnelOverHTTPPortNum);

    ~H265RTSPClient() override;

public:
    StreamClientState scs;
};

#endif //LIVERTSPCLIENT_HEVCRTSPCLIENT_HPP
