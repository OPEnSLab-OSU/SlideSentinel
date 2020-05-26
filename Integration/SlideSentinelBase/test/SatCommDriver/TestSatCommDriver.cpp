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
    while (DebugEventBus::count() < 2 && millis() - start < 360000)
        Driver::dispatch(Update{});
    // check we've powered up successfully
    DEBUG_BUS_POP_EVENT(DebugEventBus, SatComm::SignalLost);
    DEBUG_BUS_POP_EVENT(DebugEventBus, SatComm::DriverReady);
    TEST_ASSERT_TRUE(DebugEventBus::empty());
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
    while (DebugEventBus::count() < 2 && millis() - start < 360000)
        Driver::dispatch(Update{});

    // If the driver powers up successfully, we should recieve a driverready event
    // else the driver will panic
    DEBUG_BUS_POP_EVENT(DebugEventBus, SatComm::SignalLost);
    DEBUG_BUS_POP_EVENT(DebugEventBus, SatComm::DriverReady);
    TEST_ASSERT_TRUE(DebugEventBus::empty());
}

void TestReset() {
    start();
    start();
}

void DangerousTestTransmit() {
    using Driver = SatComm::Driver<DebugEventBus>;

    start();
    // Transmit a packet over satellite!
    const SatComm::Packet send{ { 'A', 'l', 's', 'o', ' ', 'h', 'e', 'r', 'e', '!', '\0' }, 11 };
    Driver::dispatch(SatComm::StartSendReceive{ send });
    // run for 5 seconds or event
    const auto start2 = millis();
    while (DebugEventBus::empty() && millis() - start2 < 360000)
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
    while (DebugEventBus::empty() && millis() - start2 < 360000)
        Driver::dispatch(Update{});
    // check to see if we received successfully
    DEBUG_BUS_POP_EVENT(DebugEventBus, SatComm::Success);
}

void DangerousTestRingAlert() {
    using Driver = SatComm::Driver<DebugEventBus>;
    const SatComm::Packet recv{ { 'A', 'l', 's', 'o', ' ', 'h', 'e', 'r', 'e', '!', '\0' }, 11 };

    start();
    // run for 5 seconds or event
    const auto start2 = millis();
    while (DebugEventBus::empty() && millis() - start2 < 360000)
        Driver::dispatch(Update{});
    DEBUG_BUS_POP_EVENT(DebugEventBus, SatComm::RingAlert);
    TEST_ASSERT_TRUE(DebugEventBus::empty());
    // recieve!
    Driver::dispatch(SatComm::StartReceive{});
    // run for 5 seconds or event
    const auto start3 = millis();
    while (DebugEventBus::empty() && millis() - start3 < 360000)
        Driver::dispatch(Update{});
    const auto event = DEBUG_BUS_POP_EVENT(DebugEventBus, SatComm::SendReceiveSuccess);
    TEST_ASSERT_EQUAL(0, event.pending_packets);
    TEST_ASSERT_EQUAL(11, event.received.length);
    TEST_MESSAGE(event.received.bytes.data());
    TEST_ASSERT_TRUE(recv.bytes == event.received.bytes);
}

void process() {
    UNITY_BEGIN();
    // RUN_TEST(TestInitial);
    // RUN_TEST(TestReset);
    // RUN_TEST(DangerousTestTransmit);
    // RUN_TEST(DangerousTestRecieve);
    RUN_TEST(DangerousTestRingAlert);
    UNITY_END();
}

void ISBDConsoleCallback(IridiumSBD *device, char c)
{
  Serial.write(c);
}

void ISBDDiagsCallback(IridiumSBD *device, char c)
{
  Serial.write(c);
}

#include "../test_common/TestMain.h"

#endif