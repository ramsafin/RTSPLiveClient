#ifndef LIVERTSPCLIENT_STREAMCLIENTSTATE_HPP
#define LIVERTSPCLIENT_STREAMCLIENTSTATE_HPP

#include <liveMedia.hh>

class StreamClientState {
public:

    StreamClientState();

    virtual ~StreamClientState();

public:
    MediaSubsessionIterator *iter;
    MediaSession *session;
    MediaSubsession *subsession;
    TaskToken streamTimerTask;
    double duration;
};

#endif //LIVERTSPCLIENT_STREAMCLIENTSTATE_HPP
