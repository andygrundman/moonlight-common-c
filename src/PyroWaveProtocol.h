#pragma once

#include <string.h>

// Bitstream baseline used by the vendored PyroWave codec. Later API/build-only
// commits do not change this ID; update it when the codec wire format changes.
#define PYROWAVE_BITSTREAM_ID "186f0393"

typedef enum _PYROWAVE_BITSTREAM_ID_STATUS {
    PYROWAVE_BITSTREAM_ID_MISSING,
    PYROWAVE_BITSTREAM_ID_MATCH,
    PYROWAVE_BITSTREAM_ID_MISMATCH,
    PYROWAVE_BITSTREAM_ID_INVALID
} PYROWAVE_BITSTREAM_ID_STATUS;

// SDP is NUL-terminated by the RTSP parser. Accept only a complete attribute
// line containing eight hexadecimal digits, and normalize its case for logging.
static inline PYROWAVE_BITSTREAM_ID_STATUS checkPyroWaveBitstreamId(const char* sdp, char hostId[9]) {
    const char* attribute = "a=x-ss-pyrowave.bitstream:";
    const char* position = sdp;
    hostId[0] = '\0';
    while ((position = strstr(position, attribute)) != NULL) {
        if (position == sdp || position[-1] == '\n') {
            position += strlen(attribute);
            for (int i = 0; i < 8; i++) {
                char digit = position[i];
                if (digit >= 'A' && digit <= 'F') {
                    digit += 'a' - 'A';
                }
                if (!((digit >= '0' && digit <= '9') || (digit >= 'a' && digit <= 'f'))) {
                    hostId[0] = '\0';
                    return PYROWAVE_BITSTREAM_ID_INVALID;
                }
                hostId[i] = digit;
            }
            hostId[8] = '\0';
            if (position[8] != '\r' && position[8] != '\n' && position[8] != '\0') {
                return PYROWAVE_BITSTREAM_ID_INVALID;
            }
            return strcmp(hostId, PYROWAVE_BITSTREAM_ID) == 0 ?
                PYROWAVE_BITSTREAM_ID_MATCH : PYROWAVE_BITSTREAM_ID_MISMATCH;
        }
        position++;
    }
    return PYROWAVE_BITSTREAM_ID_MISSING;
}
