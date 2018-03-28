#include "StreamClientState.hpp"

StreamClientState::StreamClientState() : iter(nullptr), session(nullptr), subsession(nullptr),
                                         streamTimerTask(nullptr), duration(0.0) {
}

StreamClientState::~StreamClientState() {
    delete iter;
    if (session != nullptr) {
        // We also need to delete "session", and unschedule "streamTimerTask" (if set)
        UsageEnvironment &env = session->envir(); // alias

        env.taskScheduler().unscheduleDelayedTask(streamTimerTask);
        Medium::close(session);
    }
}