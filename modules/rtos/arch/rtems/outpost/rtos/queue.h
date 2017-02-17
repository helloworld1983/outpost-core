/*
 * Copyright (c) 2014-2017, German Aerospace Center (DLR)
 *
 * This file is part of the development version of OUTPOST.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Authors:
 * - 2014-2017, Fabian Greif (DLR RY-AVS)
 */
// ----------------------------------------------------------------------------

#ifndef COBC_RTOS_RTEMS_QUEUE_H
#define COBC_RTOS_RTEMS_QUEUE_H

#include <stddef.h>
#include <stdint.h>

#include <rtems.h>

#include <outpost/time/duration.h>

namespace outpost
{
namespace rtos
{

/**
 * Atomic Queue.
 *
 * Can be used to exchange data between different threads.
 *
 * \author  Fabian Greif
 * \ingroup rtos
 */
template <typename T>
class Queue
{
public:
    /**
     * Create a Queue.
     *
     * \param numberOfItems
     *      The maximum number of items that the queue can contain.
     */
    Queue(size_t numberOfItems);

    /**
     * Destroy the queue.
     */
    ~Queue();

    /**
     * Send data to the queue.
     *
     * May trigger a thread rescheduling. The calling thread will be preempted
     * if a higher priority thread is unblocked as the result of this operation.
     *
     * \param data
     *      Reference to the item that is to be placed on the queue.
     *
     * \retval true     Value was successfully stored in the queue.
     * \retval false    Queue is full, data could not be appended.
     */
    bool
    send(const T& data);

    /**
     * Receive data from the queue.
     *
     * \param data
     *      Reference to the buffer into which the received item will be copied.
     * \param timeout
     *      Timeout in milliseconds resolution.
     *
     * \retval true     Value was received correctly and put in \p data.
     * \retval false    Timeout occurred, \p data was not changed.
     */
    bool
    receive(T& data, outpost::time::Duration timeout = outpost::time::Duration::infinity());

private:
    // disable copy constructor
    Queue(const Queue& other);

    // disable assignment operator
    Queue&
    operator=(const Queue& other);

    rtems_id id;
};

}
}

#include "queue_impl.h"

#endif // COBC_RTOS_RTEMS_QUEUE_H
