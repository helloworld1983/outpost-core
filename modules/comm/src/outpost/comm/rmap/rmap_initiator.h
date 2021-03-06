/*
 * Copyright (c) 2017, German Aerospace Center (DLR)
 *
 * This file is part of the development version of OUTPOST.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Authors:
 * - 2017, Muhammad Bassam (DLR RY-AVS)
 * - 2017, Fabian Greif (DLR RY-AVS)
 * - 2018, Jan Malburg (DLR RY-AVS)
 */

#ifndef OUTPOST_COMM_RMAP_INITIATOR_H_
#define OUTPOST_COMM_RMAP_INITIATOR_H_

#include "rmap_packet.h"
#include "rmap_status.h"
#include "rmap_transaction.h"

#include <outpost/hal/spacewire.h>
#include <outpost/rtos.h>
#include <outpost/smpc.h>
#include <outpost/support/heartbeat.h>
#include <outpost/time/duration.h>

namespace outpost
{
namespace comm
{
/**
 * RMAP initiator.
 *
 * This class is a RMAP front-end which provides the interface / access methods
 * to the user for sending and receiving RMAP packets to other SpW targets. The
 * only thing required by this class from the user is the information of the
 * RMAP target. This information is provided by the RmapTargetNode and it's list.
 * The list will be filled statically and stored as it's pointer.
 *
 * The reception of RMAP packet is handled by separate thread being supplied by
 * the initiator for any asynchronous incoming packets due to some delayed transport.
 *
 * \author  Muhammad Bassam
 */
class RmapInitiator : public outpost::rtos::Thread
{
    friend class TestingRmap;

    static constexpr outpost::time::Duration receiveTimeout = outpost::time::Seconds(5);

public:
    enum Operation
    {
        operationRead = 0x01,
        operationWrite = 0x02
    };

    struct ErrorCounters
    {
        ErrorCounters() :
            mDiscardedReceivedPackets(0),
            mNonRmapPacketReceived(0),
            mErrorneousReplyPackets(0),
            mErrorInStoringReplyPacket(0)
        {
        }

        size_t mDiscardedReceivedPackets;
        size_t mNonRmapPacketReceived;
        size_t mErrorneousReplyPackets;
        size_t mErrorInStoringReplyPacket;
    };

    /**
     * Buffer handling for providing data to the user for received reply
     * packets.
     *
     * TODO: Extend this to partial data buffering for storing multiple received
     * data on the single buffer with the help of data indexing
     * */
    struct Buffer
    {
        Buffer() : mLength(0)
        {
            memset(mData, 0, sizeof(mData));
        }

        bool
        addData(uint8_t* buffer, uint16_t len)
        {
            bool result = false;

            if (len < sizeof(mData))
            {
                mLength = len;
                memcpy(mData, buffer, mLength);
                result = true;
            }
            return result;
        }

        void
        getData(uint8_t* buffer)
        {
            memcpy(buffer, mData, mLength);
            memset(mData, 0, sizeof(mData));
            mLength = 0;
        }

    private:
        uint8_t mData[1024];
        uint16_t mLength;
    };

    struct TransactionsList
    {
        TransactionsList() : mTransactions(), mIndex(0)
        {
        }

        ~TransactionsList()
        {
        }

        uint8_t
        getActiveTransactions()
        {
            uint8_t active = 0;

            for (uint8_t i = 0; i < rmap::maxConcurrentTransactions; i++)
            {
                if (mTransactions[i].getTransactionID() != 0)
                {
                    active++;
                }
            }
            return active;
        }

        void
        addTransaction(RmapTransaction* trans)
        {
            mTransactions[mIndex] = *trans;
            mIndex = (mIndex == (rmap::maxConcurrentTransactions - 1) ? 0 : mIndex + 1);
        }

        void
        removeTransaction(uint16_t tid)
        {
            for (uint8_t i = 0; i < rmap::maxConcurrentTransactions; i++)
            {
                if (mTransactions[i].getTransactionID() == tid)
                {
                    mTransactions[i].reset();
                    break;
                }
            }
        }

        RmapTransaction*
        getTransaction(uint16_t tid)
        {
            bool found = false;
            uint8_t i;

            for (i = 0; i < rmap::maxConcurrentTransactions; i++)
            {
                if (mTransactions[i].getTransactionID() == tid)
                {
                    found = true;
                    break;
                }
            }

            if (found)
            {
                return &mTransactions[i];
            }
            else
            {
                return NULL;
            }
        }

        bool
        isTransactionIdUsed(uint16_t tid)
        {
            bool used = false;
            uint8_t i;

            for (i = 0; i < rmap::maxConcurrentTransactions; i++)
            {
                if (mTransactions[i].getTransactionID() == tid)
                {
                    used = true;
                    break;
                }
            }

            return used;
        }

        RmapTransaction*
        getFreeTransaction()
        {
            for (uint8_t i = 0; i < rmap::maxConcurrentTransactions; i++)
            {
                if (mTransactions[i].getState() == RmapTransaction::notInitiated)
                {
                    return &mTransactions[i];
                }
            }
            return NULL;
        }

        RmapTransaction mTransactions[rmap::maxConcurrentTransactions];
        uint8_t mIndex;
    };

    //--------------------------------------------------------------------------
    RmapInitiator(hal::SpaceWire& spw,
                  RmapTargetsList* list,
                  uint8_t priority,
                  size_t stackSize,
                  outpost::support::parameter::HeartbeatSource heartbeatSource);
    ~RmapInitiator();

    /**
     * Writes remote memory. For blocking write, the method blocks the current
     * thread and waits for the desired reply until specific time interval. For
     * asynchronous write non blocking mode should be used, which can be done by
     * providing zero timeout in the parameter and the command shall not expect
     * reply.
     *
     * @param targetNodeName
     *      Name of the target which is commands, targets are preinitialized list
     *      and searched against the name provided
     *
     * @param memoryAddress
     *      Actual remote memory address where the data is being written
     *
     * @param data
     *      Pointer to the data bytes
     *
     * @param length
     *      Length of the data bytes being written
     *
     * @param timeout
     *      Timeout in case of blocking transaction, otherwise use
     *      outpost::time::Duration::zero()
     *
     * @return
     *      True in case of successful operations, false for any errors encountered
     */
    bool
    write(const char* targetNodeName,
          uint32_t memoryAddress,
          outpost::Slice<const uint8_t> data,
          outpost::time::Duration timeout = outpost::time::Seconds(1));

    /**
     * Writes remote memory. For blocking write, the method blocks the current
     * thread and waits for the desired reply until specific time interval. For
     * asynchronous write non blocking mode should be used, which can be done by
     * providing zero timeout in the parameter and the command shall not expect
     * reply.
     *
     * @param targetNode
     *      Reference to the target node object found from the list
     *
     * @param memoryAddress
     *      Actual remote memory address where the data is being written
     *
     * @param data
     *      Pointer to the data bytes
     *
     * @param length
     *      Length of the data bytes being written
     *
     * @param timeout
     *      Timeout in case of blocking transaction, otherwise use
     *      outpost::time::Duration::zero()
     *
     * @return
     *      True in case of successful operations, false for any errors encountered
     */
    bool
    write(RmapTargetNode& targetNode,
          uint32_t memoryAddress,
          outpost::Slice<const uint8_t> data,
          outpost::time::Duration timeout = outpost::time::Seconds(1));

    /**
     * Read from remote memory. The method blocks the current
     * thread and waits for the desired reply until specific time interval.
     *
     * @param targetNodeName
     *      Name of the target which is commands, targets are preinitialized list
     *      and searched against the name provided
     *
     * @param memoryAddress
     *      Actual remote memory address where the data is being written
     *
     * @param buffer
     *      Pointer to the buffer where received data bytes will be stored
     *
     * @param length
     *      Length of the data bytes being read
     *
     * @param timeout
     *      Timeout for the SpW read operation
     *
     * @return
     *      True in case of successful operations, false for any errors encountered
     */
    bool
    read(const char* targetNodeName,
         uint32_t memoryAddress,
         uint8_t* buffer,
         uint32_t length,
         outpost::time::Duration timeout = outpost::time::Duration::maximum());

    /**
     * Read from remote memory. The method blocks the current
     * thread and waits for the desired reply until specific time interval.
     *
     * @param targetNode
     *      Reference to the target node object found from the list
     *
     * @param memoryAddress
     *      Actual remote memory address where the data is being written
     *
     * @param buffer
     *      Pointer to the buffer where received data bytes will be stored
     *
     * @param length
     *      Length of the data bytes being read
     *
     * @param timeout
     *      Timeout for the SpW read operation
     *
     * @return
     *      True in case of successful operations, false for any errors encountered
     */

    bool
    read(RmapTargetNode& rmapTargetNode,
         uint32_t memoryAddress,
         uint8_t* buffer,
         uint32_t length,
         outpost::time::Duration timeout = outpost::time::Duration::maximum());

    //--------------------------------------------------------------------------
    inline bool
    isReplyModeSet() const
    {
        return mReplyMode;
    }

    inline void
    setReplyMode()
    {
        mReplyMode = true;
    }

    inline void
    unsetReplyMode()
    {
        mReplyMode = false;
    }

    inline bool
    isIncrementMode() const
    {
        return mIncrementMode;
    }

    inline void
    setIncrementMode()
    {
        mIncrementMode = true;
    }

    inline void
    unsetIncrementMode()
    {
        mIncrementMode = false;
    }

    inline bool
    isVerifyMode() const
    {
        return mVerifyMode;
    }

    inline void
    setVerifyMode()
    {
        mVerifyMode = true;
    }

    inline void
    unsetVerifyMode()
    {
        mVerifyMode = false;
    }

    inline void
    setInitiatorLogicalAddress(uint8_t initiatorLogicalAddress)
    {
        mInitiatorLogicalAddress = initiatorLogicalAddress;
    }

    inline size_t
    getActiveTransactions()
    {
        return mTransactionsList.getActiveTransactions();
    }

    inline ErrorCounters
    getErrorCounters() const
    {
        return mCounters;
    }

    inline RmapPacket*
    getLatestDiscardedPacket() const
    {
        return mDiscardedPacket;
    }

private:
    virtual void
    run() override;

    void
    stop()
    {
        if (mStopped == false)
        {
            mStopped = true;
        }
    }

    inline bool
    isStopped()
    {
        return mStopped;
    }

    inline bool
    isStarted()
    {
        return !mStopped;
    }

    bool
    sendPacket(RmapTransaction* transaction, outpost::Slice<const uint8_t> data);

    bool
    receivePacket(RmapPacket* rxedPacket);

    void
    replyPacketReceived(RmapPacket* packet);

    RmapTransaction*
    resolveTransaction(RmapPacket* packet);

    uint16_t
    getNextAvailableTransactionID();

    //--------------------------------------------------------------------------
    hal::SpaceWire& mSpW;
    RmapTargetsList* mTargetNodes;
    outpost::rtos::Mutex mOperationLock;
    uint8_t mInitiatorLogicalAddress;
    bool mIncrementMode;
    bool mVerifyMode;
    bool mReplyMode;
    bool mStopped;
    uint16_t mTransactionId;
    TransactionsList mTransactionsList;

    /**
     * Discarded packet is stored immediately if no waiting transaction is
     * found with valid transaction ID for the received packet. It is made
     * available for the user in case its needed for being inspected. Discarded
     * packet will be invalidated as soon as there is new incoming packet at the
     * reception node.
     * */
    RmapPacket* mDiscardedPacket;

    ErrorCounters mCounters;
    Buffer mRxData;

    const outpost::support::parameter::HeartbeatSource mHeartbeatSource;
};

typedef outpost::Slice<const uint8_t> NonRmapDataType;
extern outpost::smpc::Topic<NonRmapDataType> nonRmapPacketReceived;

}  // namespace comm
}  // namespace outpost

#endif /* OUTPOST_COMM_RMAP_INITIATOR_H_ */
