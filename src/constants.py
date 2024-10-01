#!/usr/bin/env python3

from enum import IntEnum, unique

# Special ports used for
CPU_PORT = 255
DROP_PORT = 511

# Custom protocol number in IPv4 for our verification header
PROTO_VERIFICATION = 0x91

# Maximum number of hops in the recorded traces
TRACE_LENGTH = 8

# Number of seconds to wait for a switch to start
SWITCH_START_TIMEOUT = 10


# Priority for match-action table entries
@unique
class Priority(IntEnum):
    LOW = 1
    MEDIUM = 2
    HIGH = 3
