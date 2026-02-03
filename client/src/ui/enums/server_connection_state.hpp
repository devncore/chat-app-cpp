#pragma once

#include <enum.h>

BETTER_ENUM(ServerConnectionState, int,
    IDLE,
    CONNECTING,
    READY,
    TRANSIENT_FAILURE,
    SHUTDOWN,
    UNKNOWN
)
