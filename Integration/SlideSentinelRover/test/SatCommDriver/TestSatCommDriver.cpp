//
// Created by Noah on 4/22/2020.
//

#if defined(UNIT_TEST) && defined(ARDUINO)

#include "../test_common/DebugEventBus.h"
#include "unity.h"
#include "SatCommDriver.h"

void start() {
    using Driver = SatComm::Driver<DebugEventBus>;

    DebugEventBus::reset();
    Driver::reset();
    Driver::start();
    Driver::dispatch(PowerUp{});
    const auto start = millis();
    while (DebugEventBus::empty() && millis() - start < 5000)
        Driver::dispatch(Update{});
    // check we've powered up successfully
    DEBUG_BUS_POP_EVENT(DebugEventBus, SatComm::DriverReady);
}

// NOTE: This test can take up to 2 minutes to fail!
void TestInitial() {
    using Driver = SatComm::Driver<DebugEventBus>;

    DebugEventBus::reset();
    Driver::reset();
    Driver::start();
    Driver::dispatch(PowerUp{});
    // we also need to send an update event so the driver can verify the signal
    // run for 5 seconds and fail
    const auto start = millis();
    while (DebugEventBus::empty() && millis() - start < 5000)
        Driver::dispatch(Update{});

    // If the driver powers up successfully, we should recieve a driverready event
    // else the driver will panic

    DEBUG_BUS_POP_EVENT(DebugEventBus, SatComm::DriverReady);
    TEST_ASSERT_TRUE(DebugEventBus::empty());
}

void DangerousTestTransmit() {
    using Driver = SatComm::Driver<DebugEventBus>;

    start();
    // Transmit a packet over satellite!
    const SatComm::Packet send{ { 'A', 'l', 's', 'o', ' ', 'h', 'e', 'r', 'e', '!', '\0' }, 11 };
    Driver::dispatch(SatComm::StartSendReceive{ send });
    // run for 5 seconds or event
    const auto start2 = millis();
    while (DebugEventBus::empty() && millis() - start2 < 5000)
        Driver::dispatch(Update{});
    // check to see that the transmission succeed
    DEBUG_BUS_POP_EVENT(DebugEventBus, SatComm::SendSuccess);
}

void DangerousTestRecieve() {
    using Driver = SatComm::Driver<DebugEventBus>;

    start();
    Driver::dispatch(SatComm::StartReceive{});
    // run for 5 seconds or event
    const auto start2 = millis();
    while (DebugEventBus::empty() && millis() - start2 < 5000)
        Driver::dispatch(Update{});
    // check to see if we received successfully
    DEBUG_BUS_POP_EVENT(DebugEventBus, SatComm::Success);
}

void process() {
    UNITY_BEGIN();
    RUN_TEST(TestInitial);
    // RUN_TEST(DangerousTestTransmit);
    // RUN_TEST(DangerousTestRecieve);
    UNITY_END();
}

#include "../test_common/TestMain.h"

#endif