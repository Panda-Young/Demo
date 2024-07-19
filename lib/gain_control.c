/***************************************************************************
 * Description: Gain control
 * version: 0.1.0
 * Author: Panda-Young
 * Date: 2024-07-13 12:40:09
 * Copyright (c) 2024 by Panda-Young, All Rights Reserved.
 **************************************************************************/

#include "gain_control.h"
#include <math.h>
#include "../Logger.h"

// Define minimum and maximum values for the gain factor
#define MIN_GAIN_FACTOR 0.00001f // A value close to 0 but not 0, to avoid division by zero or extremely small gains
#define MAX_GAIN_FACTOR 10000.0f // A large value, but kept within practical limits

float dBChangeToFactor(float dBChange)
{
    // Check the validity of dBChange
    if (isnan(dBChange) || isinf(dBChange)) {
        LOG_MSG_CF(LOG_ERROR, "Invalid dBChange value: %f, returning default value: 1.0", dBChange);
        // If dBChange is NaN or infinite, return an error value or a default value
        return 1.0f; // Alternatively, one could choose to return MIN_GAIN_FACTOR or MAX_GAIN_FACTOR, depending on the requirement
    }

    // Calculate the gain factor
    float factor = powf(10.0f, dBChange / 20.0f);

    // Limit the range of the gain factor
    if (factor < MIN_GAIN_FACTOR) {
        factor = MIN_GAIN_FACTOR;
    } else if (factor > MAX_GAIN_FACTOR) {
        factor = MAX_GAIN_FACTOR;
    }

    return factor;
}
