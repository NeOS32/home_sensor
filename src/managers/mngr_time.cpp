// SPDX-FileCopyrightText: 2021 Sebastian Serewa <neos32.project@gmail.com>
//
// SPDX-License-Identifier: Apache-2.0

#include "my_common.h"

//static debug_level_t uDebugLevel = DEBUG_LOG;

u32 getSecondsFromNumberAndScale(char number, char scale) {
    if ((number > 9) || (number < 0))
        number = 0;
    else
        switch (scale) {
        case 'S': // Seconds
            return ((u32)number);

        case 'M': // Minutes
            return ((u32)60 * number);

        case 'T': // TenMinutes
            return ((u32)10 * 60 * number);

        case 'H': // Hours
            return ((u32)60 * 60 * number);

        default:
            return ((u32)0);
        }

    return (number);
}

u8 getDecodedChannelNum(u8 uRawNumber) {
    // INTENTION: to convert an ascii char into a valid integer. Here are the rules:
    // for values <= 9 return itself
    // for values <= 'A'..'Z' return 10 + offset of given character starting from 'A'

    if (uRawNumber <= 9)
        return uRawNumber;

    uRawNumber += '0';
    uRawNumber -= 'A';

    return 10 + uRawNumber; // we're returning base + delta. This gives us 10 + 25 (90-65=25) => 35 values
}
