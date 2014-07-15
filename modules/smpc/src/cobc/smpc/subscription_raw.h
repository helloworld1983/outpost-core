/*
 * Copyright (c) 2013, German Aerospace Center (DLR)
 * All Rights Reserved.
 *
 * See the file "LICENSE" for the full license governing this code.
 */
// ----------------------------------------------------------------------------

#ifndef COBC_SMPC_SUBSCRIPTION_RAW_H
#define COBC_SMPC_SUBSCRIPTION_RAW_H

#include <stddef.h>

#include "subscriber.h"
#include "topic_raw.h"

namespace cobc
{
namespace smpc
{
/**
 * Subscription to a topic.
 *
 * Every component that wants to receive data from a topic needs to
 * Instantiate one or more objects of this class. Each instance binds
 * a topic to a member function of the subscribing class. Data send to
 * the topic will be forwarded to the given member function.
 *
 * To avoid the overhead of generating a copy of this class for every
 * type of topic and subscriber this class works internally with void
 * pointers.
 *
 * \ingroup smpc
 * \author  Fabian Greif
 */
class SubscriptionRaw : public ImplicitList<SubscriptionRaw>
{
public:
    friend class TopicRaw;

    /**
     * Constructor.
     *
     * Binds a subscriber to a topic.
     *
     * \param[in]    topic
     *         Raw Topic to subscribe to
     * \param[in]    subscriber
     *         Subscribing class. Must be a subclass of cobc::com::Subscriber.
     * \param[in]    function
     *         Member function pointer of the subscribing class.
     */
    template <typename S>
    SubscriptionRaw(TopicRaw& topic,
                    S * subscriber,
                    void (S::*function)(const void * message, size_t length)) :
        ImplicitList<SubscriptionRaw>(listOfAllSubscriptions, this),
        topic(&topic),
        nextTopicSubscription(0),
        subscriber(reinterpret_cast<Subscriber *>(subscriber)),
        function(reinterpret_cast<Function>(function))
    {
    }

    /**
     * Destroy the subscription
     *
     * \warning    The destruction and creation of subscriptions during the normal
     *             runtime is not thread-safe. If topics need to be
     *             destroyed outside the initialization of the application
     *             it is necessary to hold all other threads which
     *             might also create or destroy topics and/or subscriptions.
     */
    ~SubscriptionRaw();

    /**
     * Connect all subscriptions to it's assigned topic.
     *
     * Has to be called at program startup to initialize the
     * Publisher<>Subscriber protocol.
     *
     * \internal
     * Builds the internal linked lists.
     */
    static void
    connectSubscriptionsToTopics();

protected:
    /** Base-type to cast all member function pointers to. */
    typedef void (Subscriber::*Function)(const void *, size_t);

    /**
     * Relay message to the subscribing component.
     */
    inline void
    notify(const void * message, size_t length) const
    {
        (subscriber->*function)(message, length);
    }

    /**
     * Release all subscriptions.
     *
     * Counterpart to connectSubscriptionsToTopics(). Use of this
     * function is not thread-safe. To use halt all threads that
     * might create or destroy subscriptions or topics.
     */
    static void
    releaseAllSubscriptions();

private:
    // List of all subscriptions currently in the system
    static SubscriptionRaw * listOfAllSubscriptions;

    // Used by Subscription::connect to map the subscriptions to
    // their corresponding topics.
    TopicRaw * const topic;
    SubscriptionRaw * nextTopicSubscription;

    // Object and member function to forward a received message to
    Subscriber * const subscriber;
    Function const function;
};

}
}

#endif // COBC_SMPC_SUBSCRIPTION_RAW_H
