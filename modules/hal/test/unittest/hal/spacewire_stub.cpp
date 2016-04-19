/*
 * Copyright (c) 2015, German Aerospace Center (DLR)
 *
 * This file is part of libCOBC 0.5.
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

#include "spacewire_stub.h"

using unittest::hal::SpaceWireStub;

SpaceWireStub::SpaceWireStub(size_t maximumLength) :
    mMaximumLength(maximumLength),
    mOpen(false),
    mUp(false)
{
}

SpaceWireStub::~SpaceWireStub()
{
}

size_t
SpaceWireStub::getMaximumPacketLength() const
{
    return mMaximumLength;
}

bool
SpaceWireStub::open()
{
    mOpen = true;
    return mOpen;
}

void
SpaceWireStub::close()
{
    mOpen = false;
    mUp = false;
}

bool
SpaceWireStub::up(cobc::time::Duration /*timeout*/)
{
    if (mOpen)
    {
        mUp = true;
    }
    return mUp;
}

void
SpaceWireStub::down(cobc::time::Duration /*timeout*/)
{
    mUp = false;
}

bool
SpaceWireStub::isUp()
{
    return mUp;
}


SpaceWireStub::Result::Type
SpaceWireStub::requestBuffer(TransmitBuffer*& buffer,
                             cobc::time::Duration /*timeout*/)
{
    Result::Type result = Result::success;

    std::unique_ptr<TransmitBufferEntry> entry(new TransmitBufferEntry(mMaximumLength));
    buffer = &entry->header;
    mTransmitBuffers.emplace(make_pair(&(entry->header), std::move(entry)));

    return result;
}

SpaceWireStub::Result::Type
SpaceWireStub::send(TransmitBuffer* buffer)
{
    Result::Type result = Result::success;
    if (mUp)
    {
        try
        {
            std::unique_ptr<TransmitBufferEntry>& entry = mTransmitBuffers.at(buffer);
            mSentPackets.emplace_back(Packet { std::vector<uint8_t>(&entry->buffer.front(),
                                                                    &entry->buffer.front() + entry->header.getLength()),
                                               entry->header.getEndMarker() });
            mTransmitBuffers.erase(buffer);
        }
        catch (std::out_of_range&)
        {
            result = Result::failure;
        }
    }
    else
    {
        result = Result::failure;
    }

    return result;
}

SpaceWireStub::Result::Type
SpaceWireStub::receive(ReceiveBuffer& buffer,
                       cobc::time::Duration /*timeout*/)
{
    Result::Type result = Result::success;
    if (mUp)
    {
        std::unique_ptr<ReceiveBufferEntry> entry(new ReceiveBufferEntry(std::move(mPacketsToReceive.front().data),
                                                                         mPacketsToReceive.front().end));
        buffer = entry->header;
        mReceiveBuffers.emplace(make_pair(entry->header.getData().begin(), std::move(entry)));
        mPacketsToReceive.pop_front();
    }
    else
    {
        result = Result::failure;
    }

    return result;
}

void
SpaceWireStub::releaseBuffer(const ReceiveBuffer& buffer)
{
    mReceiveBuffers.erase(buffer.getData().begin());
}

void
SpaceWireStub::flushReceiveBuffer()
{
    // Do nothing
}
