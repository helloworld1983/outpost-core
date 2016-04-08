/*
 * Copyright (c) 2016, German Aerospace Center (DLR)
 * All Rights Reserved.
 *
 * See the file "LICENSE" for the full license governing this code.
 */
// ----------------------------------------------------------------------------

#ifndef COBC_RTOS_RTEMS_INTERVAL_H
#define COBC_RTOS_RTEMS_INTERVAL_H

#include <rtems.h>
#include <cobc/time/duration.h>

namespace cobc
{
namespace rtos
{
namespace rtems
{

/**
 * Converts a duration into a RTEMS interval.
 *
 * An RTEMS interval needs to be at least 1 tick long, otherwise it would
 * specify an infinite interval.
 */
static inline rtems_interval
getInterval(cobc::time::Duration duration)
{
    rtems_interval interval;
    if (duration == cobc::time::Duration::infinity())
    {
        interval = 0;
    }
    else
    {
        interval = (rtems_clock_get_ticks_per_second()
                  * duration.milliseconds()) / time::Duration::millisecondsPerSecond;
        if (interval < 1)
        {
            interval = 1;
        }
    }
    return interval;
}

}
}
}

#endif