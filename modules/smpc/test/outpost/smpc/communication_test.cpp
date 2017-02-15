/*
 * Copyright (c) 2014, German Aerospace Center (DLR)
 *
 * This file is part of outpost 0.6.
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

#include <stdint.h>

#include <outpost/smpc/topic.h>
#include <outpost/smpc/subscription.h>

#include <unittest/smpc/testing_subscription.h>

#include <cstring>
#include <unittest/harness.h>

struct Data
{
    uint32_t foo;
    uint16_t bar;
};

static bool
operator==(Data const& lhs, Data const& rhs)
{
    return ((lhs.foo == rhs.foo) &&
            (lhs.bar == rhs.bar));
}

class Component : public outpost::smpc::Subscriber
{
public:
    Component(outpost::smpc::Topic<const Data>& topic) :
        mReceived(false),
        mData({0, 0}),
        subscription(topic, this, &Component::onReceiveData)
    {
    }

    void
    onReceiveData(const Data* data)
    {
        mReceived = true;
        mData = *data;
    }

    bool mReceived;
    Data mData;

private:
    outpost::smpc::Subscription subscription;
};

TEST(CommunicationTest, receiveSingle)
{
    outpost::smpc::Topic<const Data> topic;
    Component component(topic);

    outpost::smpc::Subscription::connectSubscriptionsToTopics();

    Data data = {
        0x12345678,
        0x9876
    };

    topic.publish(data);

    ASSERT_TRUE(component.mReceived);
    EXPECT_EQ(data, component.mData);

    unittest::smpc::TestingSubscription::releaseAllSubscriptions();
}