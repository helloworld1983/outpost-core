/*
 * Copyright (c) 2013, German Aerospace Center (DLR)
 *
 * This file is part of libCOBC 0.6.
 *
 * It is distributed under the terms of the GNU General Public License with a
 * linking exception. See the file "LICENSE" for the full license governing
 * this code.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 */
// ----------------------------------------------------------------------------

#include "clock.h"

cobc::time::SpacecraftElapsedTime
cobc::rtos::SystemClock::now() const
{
    // convert to microseconds
    uint64_t us = 0;

    return cobc::time::SpacecraftElapsedTime::afterEpoch(cobc::time::Microseconds(us));
}
