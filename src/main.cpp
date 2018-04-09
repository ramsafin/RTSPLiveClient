#include <GroupsockHelper.hh>
#include "liveMedia.hh"
#include "BasicUsageEnvironment.hh"
#include "SDLVideoSink.hpp"
#include "StreamClientState.hpp"
#include "H265RTSPClient.hpp"

// RTSP 'response handlers':
void continueAfterDESCRIBE(RTSPClient *rtspClient, int resultCode, char *sdp);

void continueAfterSETUP(RTSPClient *rtspClient, int resultCode, char *resultString);

void continueAfterPLAY(RTSPClient *rtspClient, int resultCode, char *resultString);

// Other event handler functions:

// called when a stream's subsession (e.g., audio or video substream) ends
void subsessionAfterPlaying(void *clientData);

// called when a RTCP "BYE" is received for a subsession
void subsessionByeHandler(void *clientData);

// The main streaming routine (for each "rtsp://" URL):
void openURL(UsageEnvironment &env, char const *progName, char const *rtspURL);

// Used to iterate through each stream's 'subsessions', setting up each one:
void setupNextSubsession(RTSPClient *rtspClient);

// Used to shut down and close a stream (including its "RTSPClient" object):
void shutdownStream(RTSPClient *rtspClient, int exitCode = 1);

// A function that outputs a string that identifies each stream (for debugging output).  Modify this if you wish:
UsageEnvironment &operator<<(UsageEnvironment &env, const RTSPClient &rtspClient) {
    return env << "[URL:\"" << rtspClient.url() << "\"]: ";
}

// A function that outputs a string that identifies each subsession (for debugging output).  Modify this if you wish:
UsageEnvironment &operator<<(UsageEnvironment &env, const MediaSubsession &subsession) {
    return env << subsession.mediumName() << "/" << subsession.codecName();
}

char eventLoopWatchVariable = 0;

int main(int argc, char **argv) {

    // Begin by setting up our usage environment:
    TaskScheduler *scheduler = BasicTaskScheduler::createNew();
    UsageEnvironment *env = BasicUsageEnvironment::createNew(*scheduler);

    openURL(*env, "first camera", "rtsp://localhost:8554/camera");
//    openURL(*env, "second camera", "rtsp://localhost:8554/camera_2");

    // All subsequent activity takes place within the event loop:
    env->taskScheduler().doEventLoop(&eventLoopWatchVariable);
    // This function call does not return, unless, at some point in time, "eventLoopWatchVariable" gets set to something non-zero.

    env->reclaim();

    delete scheduler;

    return 0;
}

#define RTSP_CLIENT_VERBOSITY_LEVEL 1 // by default, print verbose output from each "RTSPClient"

void openURL(UsageEnvironment &env, char const *progName, char const *rtspURL) {

    RTSPClient *rtspClient = H265RTSPClient::createNew(env, rtspURL, RTSP_CLIENT_VERBOSITY_LEVEL, progName);

    if (rtspClient == nullptr) {
        env << "Failed to create a RTSP client for URL \"" << rtspURL << "\": " << env.getResultMsg() << "\n";
        return;
    }

    // Next, send a RTSP "DESCRIBE" command, to get a SDP description for the stream.
    // Note that this command - like all RTSP commands - is sent asynchronously; we do not block, waiting for a response.
    // Instead, the following function call returns immediately, and we handle the RTSP response later, from within the event loop:
    rtspClient->sendDescribeCommand(continueAfterDESCRIBE);
}


// Implementation of the RTSP 'response handlers':

void continueAfterDESCRIBE(RTSPClient *rtspClient, int resultCode, char *sdp) {

    do {
        UsageEnvironment &env = rtspClient->envir(); // alias
        StreamClientState &scs = ((H265RTSPClient *) rtspClient)->scs; // alias

        if (resultCode != 0) {
            env << *rtspClient << "Failed to get an SDP description: " << sdp << "\n";
            delete[] sdp;
            break;
        }


        env << *rtspClient << "Got a SDP description:\n" << sdp << "\n";

        // Create a media session object from this SDP description:
        scs.session = MediaSession::createNew(env, sdp);

        delete[] sdp; // because we don't need it anymore

        if (scs.session == nullptr) {
            env << *rtspClient << "Failed to create a MediaSession object from the SDP description: "
                << env.getResultMsg() << "\n";
            break;
        } else if (!scs.session->hasSubsessions()) {
            env << *rtspClient << "This session has no media subsessions (i.e., no \"m=\" lines)\n";
            break;
        }

        // Then, create and set up our data source objects for the session.  We do this by iterating over the session's 'subsessions',
        // calling "MediaSubsession::initiate()", and then sending a RTSP "SETUP" command, on each one.
        // (Each 'subsession' will have its own data source.)
        scs.iter = new MediaSubsessionIterator(*scs.session);
        setupNextSubsession(rtspClient);
        return;
    } while (0);

    // An unrecoverable error occurred with this stream.
    shutdownStream(rtspClient);
}

// By default, we request that the server stream its data using RTP/UDP.
// If, instead, you want to request that the server stream via RTP-over-TCP, change the following to True:
#define REQUEST_STREAMING_OVER_TCP False

void setupNextSubsession(RTSPClient *rtspClient) {

    UsageEnvironment &env = rtspClient->envir(); // alias
    StreamClientState &scs = ((H265RTSPClient *) rtspClient)->scs; // alias

    scs.subsession = scs.iter->next();

    if (scs.subsession != nullptr) {
        if (!scs.subsession->initiate()) {
            env << *rtspClient << "Failed to initiate the \"" << *scs.subsession << "\" subsession: "
                << env.getResultMsg() << "\n";
            setupNextSubsession(rtspClient); // give up on this subsession; go to the next one
        } else {
            env << *rtspClient << "Initiated the \"" << *scs.subsession << "\" subsession (";
            if (scs.subsession->rtcpIsMuxed()) {
                env << "client port " << scs.subsession->clientPortNum();
            } else {
                env << "client ports " << scs.subsession->clientPortNum() << "-" << scs.subsession->clientPortNum() + 1;
            }
            env << ")\n";

            // Continue setting up this subsession, by sending a RTSP "SETUP" command:
            rtspClient->sendSetupCommand(*scs.subsession, continueAfterSETUP, False, REQUEST_STREAMING_OVER_TCP);
        }
        return;
    }

    // We've finished setting up all of the subsessions.  Now, send a RTSP "PLAY" command to start the streaming:
    if (scs.session->absStartTime() != nullptr) {
        // Special case: The stream is indexed by 'absolute' time, so send an appropriate "PLAY" command:
        rtspClient->sendPlayCommand(*scs.session, continueAfterPLAY, scs.session->absStartTime(),
                                    scs.session->absEndTime());
    } else {
        scs.duration = scs.session->playEndTime() - scs.session->playStartTime();
        rtspClient->sendPlayCommand(*scs.session, continueAfterPLAY);
    }
}

void continueAfterSETUP(RTSPClient *rtspClient, int resultCode, char *resultString) {
    do {
        UsageEnvironment &env = rtspClient->envir(); // alias
        StreamClientState &scs = ((H265RTSPClient *) rtspClient)->scs; // alias

        if (resultCode != 0) {
            env << *rtspClient << "Failed to set up the \"" << *scs.subsession << "\" subsession: " << resultString
                << "\n";
            break;
        }

        env << *rtspClient << "Set up the \"" << *scs.subsession << "\" subsession (";
        if (scs.subsession->rtcpIsMuxed()) {
            env << "client port " << scs.subsession->clientPortNum();
        } else {
            env << "client ports " << scs.subsession->clientPortNum() << "-" << scs.subsession->clientPortNum() + 1;
        }
        env << ")\n";

        // Having successfully setup the subsession, create a data sink for it, and call "startPlaying()" on it.
        // (This will prepare the data sink to receive data; the actual flow of data from the client won't start happening until later,
        // after we've sent a RTSP "PLAY" command.)

        scs.subsession->sink = DummySink::createNew(env, *scs.subsession, rtspClient->url());
        // perhaps use your own custom "MediaSink" subclass instead

        if (scs.subsession->sink == nullptr) {
            env << *rtspClient << "Failed to create a data sink for the \"" << *scs.subsession
                << "\" subsession: " << env.getResultMsg() << "\n";
            break;
        }

        env << *rtspClient << "Created a data sink for the \"" << *scs.subsession << "\" subsession\n";
        scs.subsession->miscPtr = rtspClient; // a hack to let subsession handler functions get the "RTSPClient" from the subsession
        scs.subsession->sink->startPlaying(*(scs.subsession->readSource()),
                                           subsessionAfterPlaying, scs.subsession);
        // Also set a handler to be called if a RTCP "BYE" arrives for this subsession:
        if (scs.subsession->rtcpInstance() != nullptr) {
            scs.subsession->rtcpInstance()->setByeHandler(subsessionByeHandler, scs.subsession);
        }
    } while (0);
    delete[] resultString;

    // Set up the next subsession, if any:
    setupNextSubsession(rtspClient);
}

void continueAfterPLAY(RTSPClient *rtspClient, int resultCode, char *resultString) {

    Boolean success = False;

    do {
        UsageEnvironment &env = rtspClient->envir(); // alias
        StreamClientState &scs = ((H265RTSPClient *) rtspClient)->scs; // alias

        if (resultCode != 0) {
            env << *rtspClient << "Failed to start playing session: " << resultString << "\n";
            break;
        }

        env << *rtspClient << "Started playing session" << "\n";

        success = True;

    } while (0);

    delete[] resultString;

    if (!success) {
        // An unrecoverable error occurred with this stream.
        shutdownStream(rtspClient);
    }
}

// Implementation of the other event handlers:

void subsessionAfterPlaying(void *clientData) {
    auto subsession = (MediaSubsession *) clientData;
    auto rtspClient = (RTSPClient *) (subsession->miscPtr);

    // Begin by closing this subsession's stream:
    Medium::close(subsession->sink);
    subsession->sink = nullptr;

    // Next, check whether *all* subsessions' streams have now been closed:
    MediaSession &session = subsession->parentSession();
    MediaSubsessionIterator iter(session);
    while ((subsession = iter.next()) != nullptr) {
        if (subsession->sink != nullptr) return; // this subsession is still active
    }

    // All subsessions' streams have now been closed, so shutdown the client:
    shutdownStream(rtspClient);
}

void subsessionByeHandler(void *clientData) {
    auto subsession = (MediaSubsession *) clientData;
    auto rtspClient = (RTSPClient *) subsession->miscPtr;
    UsageEnvironment &env = rtspClient->envir(); // alias

    env << *rtspClient << R"(Received RTCP "BYE" on ")" << *subsession << " subsession\n";

    // Now act as if the subsession had closed:
    subsessionAfterPlaying(subsession);
}

void shutdownStream(RTSPClient *rtspClient, int exitCode) {

    UsageEnvironment &env = rtspClient->envir(); // alias
    StreamClientState &scs = ((H265RTSPClient *) rtspClient)->scs; // alias

    // First, check whether any subsessions have still to be closed:
    if (scs.session != nullptr) {
        Boolean someSubsessionsWereActive = False;
        MediaSubsessionIterator iter(*scs.session);
        MediaSubsession *subsession;

        while ((subsession = iter.next()) != nullptr) {
            if (subsession->sink != nullptr) {
                Medium::close(subsession->sink);
                subsession->sink = nullptr;

                if (subsession->rtcpInstance() != nullptr) {
                    subsession->rtcpInstance()->setByeHandler(nullptr,
                                                              nullptr); // in case the server sends a RTCP "BYE" while handling "TEARDOWN"
                }

                someSubsessionsWereActive = True;
            }
        }

        if (someSubsessionsWereActive) {
            // Send a RTSP "TEARDOWN" command, to tell the server to shutdown the stream.
            // Don't bother handling the response to the "TEARDOWN".
            rtspClient->sendTeardownCommand(*scs.session, nullptr);
        }
    }

    env << *rtspClient << "Closing the stream.\n";
    Medium::close(rtspClient);
    // Note that this will also cause this stream's "StreamClientState" structure to get reclaimed.

    eventLoopWatchVariable = 's';
}
