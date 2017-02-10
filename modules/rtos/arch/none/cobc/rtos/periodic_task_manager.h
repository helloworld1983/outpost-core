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

#ifndef COBC_RTOS_NONE_PERIODIC_TASK_MANAGER_H
#define COBC_RTOS_NONE_PERIODIC_TASK_MANAGER_H

#include <cobc/time/duration.h>

#include <cobc/rtos/mutex.h>
#include <cobc/rtos/failure_handler.h>

namespace cobc
{
    namespace rtos
    {
        /**
         * Helper class for Rate-Monotonic Scheduling (RMS).
         *
         * The POSIX implementation builds upon the clock_nanosleep() method
         * with CLOCK_MONOTONIC as clock source.
         *
         * \author    Fabian Greif
         * \ingroup    rtos
         */
        class PeriodicTaskManager
        {
        public:
            struct Status
            {
                enum Type
                {
                    /// Period has not been started
                    idle,

                    /// Period is currently running
                    running,

                    /// Period has expired
                    timeout
                };
            };

            PeriodicTaskManager();

            ~PeriodicTaskManager();

            /**
             * Start next period.
             *
             * If the PeriodicTaskManager is running, the calling thread will
             * be blocked for the remainder of the outstanding period and,
             * upon completion of that period, the period will be reinitialized
             * with the specified period.
             *
             * If the PeriodicTaskManager is not currently running and has
             * not expired, it is initiated with a length of period ticks and
             * the calling task returns immediately.
             *
             * If the PeriodicTaskManager has expired before the thread invokes
             * the \c nextPeriod method, the period will be initiated with a
             * length of *period* and the calling task returns immediately with
             * a timeout error status.
             *
             * \param  period
             *     Length of the next period. Can be different from the
             *     previous one.
             *
             * \retval    Status::running
             *     Period is currently running.
             * \retval  Status::timeout
             *     Last period was missed, this may require some different
             *     handling from the user.
             */
            Status::Type
            nextPeriod(time::Duration period);

            /**
             * Check the status of the current period.
             *
             * \retval  Status::idle
             *     Period has not been started.
             * \retval    Status::running
             *     Period is currently running.
             * \retval  Status::timeout
             *     Last period was missed, this may require some different
             *     handling from the user.
             */
            Status::Type
            status();

            /**
             * Period measurement is stopped.
             *
             * Can be restarted with the invocation of \c nextPeriod.
             */
            void
            cancel();

        private:
        };
    }
}

#endif // COBC_RTOS_NONE_PERIODIC_TASK_MANAGER_H
