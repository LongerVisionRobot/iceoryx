// Copyright (c) 2020 by Apex.AI Inc. All rights reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "iceoryx_binding_c/enums.h"
#include "iceoryx_binding_c/event_info.h"
#include "iceoryx_binding_c/runtime.h"
#include "iceoryx_binding_c/subscriber.h"
#include "iceoryx_binding_c/types.h"
#include "iceoryx_binding_c/user_trigger.h"
#include "iceoryx_binding_c/wait_set.h"
#include "sleep_for.h"
#include "topic_data.h"

#include <signal.h>
#include <stdbool.h>
#include <stdio.h>

#define NUMBER_OF_TRIGGER 3
#define NUMBER_OF_SUBSCRIBER 2

iox_user_trigger_storage_t shutdownTriggerStorage;
iox_user_trigger_t shutdownTrigger;

static void sigHandler(int signalValue)
{
    (void)signalValue;

    iox_user_trigger_trigger(shutdownTrigger);
}

int main()
{
    iox_runtime_init("/iox-c-ex-waitset-individual");

    iox_ws_storage_t waitSetStorage;
    iox_ws_t waitSet = iox_ws_init(&waitSetStorage);
    shutdownTrigger = iox_user_trigger_init(&shutdownTriggerStorage);

    // attach shutdownTrigger with no callback to handle CTRL+C
    iox_user_trigger_enable_trigger_event(shutdownTrigger, waitSet, 0, NULL);

    //// register signal after shutdownTrigger since we are using it in the handler
    signal(SIGINT, sigHandler);

    // array where the subscriber are stored
    iox_sub_storage_t subscriberStorage[NUMBER_OF_SUBSCRIBER];
    iox_sub_t subscriber[NUMBER_OF_SUBSCRIBER];

    // create two subscribers, subscribe to the service and attach them to the waitset
    uint64_t historyRequest = 1U;
    subscriber[0] = iox_sub_init(&(subscriberStorage[0]), "Radar", "FrontLeft", "Counter", historyRequest);
    subscriber[1] = iox_sub_init(&(subscriberStorage[1]), "Radar", "FrontLeft", "Counter", historyRequest);

    iox_sub_subscribe(subscriber[0], 256);
    iox_sub_subscribe(subscriber[1], 256);

    iox_sub_attach_event(subscriber[0], waitSet, SubscriberEvent_HAS_NEW_SAMPLES, 0, NULL);
    iox_sub_attach_event(subscriber[1], waitSet, SubscriberEvent_HAS_NEW_SAMPLES, 0, NULL);


    uint64_t missedElements = 0U;
    uint64_t numberOfTriggeredConditions = 0U;

    // array where all event infos from iox_ws_wait will be stored
    iox_event_info_t eventArray[NUMBER_OF_TRIGGER];

    // event loop
    bool keepRunning = true;
    while (keepRunning)
    {
        numberOfTriggeredConditions = iox_ws_wait(waitSet, eventArray, NUMBER_OF_TRIGGER, &missedElements);

        for (uint64_t i = 0U; i < numberOfTriggeredConditions; ++i)
        {
            iox_event_info_t event = eventArray[i];

            if (iox_event_info_does_originate_from_user_trigger(event, shutdownTrigger))
            {
                // CTRL+c was pressed -> exit
                keepRunning = false;
            }
            // process sample received by subscriber1
            else if (iox_event_info_does_originate_from_subscriber(event, subscriber[0]))
            {
                const void* chunk;
                if (iox_sub_get_chunk(subscriber[0], &chunk))
                {
                    printf("subscriber 1 received: %u\n", ((struct CounterTopic*)chunk)->counter);

                    iox_sub_release_chunk(subscriber[0], chunk);
                }
            }
            // dismiss sample received by subscriber2
            else if (iox_event_info_does_originate_from_subscriber(event, subscriber[1]))
            {
                // We need to release the samples to reset the event hasNewSamples
                // otherwise the WaitSet would notify us in `iox_ws_wait()` again
                // instantly.
                iox_sub_release_queued_chunks(subscriber[1]);
                printf("subscriber 2 received something - dont care\n");
            }
        }
    }

    // cleanup all resources
    for (uint64_t i = 0U; i < NUMBER_OF_SUBSCRIBER; ++i)
    {
        iox_sub_unsubscribe((iox_sub_t) & (subscriberStorage[i]));
        iox_sub_deinit((iox_sub_t) & (subscriberStorage[i]));
    }

    iox_ws_deinit(waitSet);
    iox_user_trigger_deinit(shutdownTrigger);


    return 0;
}
