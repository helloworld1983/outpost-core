/*
 * Copyright (c) 2014-2018, German Aerospace Center (DLR)
 *
 * This file is part of the development version of OUTPOST.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Authors:
 * - 2014-2018, Fabian Greif (DLR RY-AVS)
 */

#include "configurable_event_listener.h"

void
unittest::ConfigurableEventListener::OnTestEnd(const testing::TestInfo& test_info)
{
    if (showInlineFailures && test_info.result()->Failed())
    {
        testing::internal::ColoredPrintf(testing::internal::COLOR_RED, "[  FAILED  ]");
        printf(" %s.%s\n\n", test_info.test_case_name(), test_info.name());
    }
    else if (showSuccesses && !test_info.result()->Failed())
    {
        eventListener->OnTestEnd(test_info);
    }
}

unittest::ConfigurableEventListener*
unittest::registerConfigurableEventListener()
{
    // Remove the default listener
    testing::TestEventListeners& listeners = testing::UnitTest::GetInstance()->listeners();
    auto defaultPrinter = listeners.Release(listeners.default_result_printer());

    // Add our listener, by default everything is off, like:
    // [==========] Running 149 tests from 53 test cases.
    // [==========] 149 tests from 53 test cases ran. (1 ms total)
    // [ PASSED ] 149 tests.
    ConfigurableEventListener* listener = new ConfigurableEventListener(defaultPrinter);
    listeners.Append(listener);

    return listener;
}
