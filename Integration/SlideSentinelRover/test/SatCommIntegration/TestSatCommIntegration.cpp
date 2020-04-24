//
// Created by Noah on 4/23/2020.
//

#if defined(UNIT_TEST) && defined(ARDUINO)

#include "../test_common/DebugEventBus.h"
#include "unity.h"
#include "SatCommDriver.h"
#include "SatCommController.h"
#include "EventQueue.h"

void start() {
    using Machine = EventQueue<SatComm::Controller, SatComm::Driver>;
    using Controller = SatComm::Controller<Machine>;

    Machine::reset();
    Machine::start();

    // power up!
    Machine::dispatch(PowerUp{});

    // cycle until available or 5 seconds
    const auto start = millis();
    while (!Controller::connected() && millis() - start < 5000) {
        if (!Machine::next())
            TEST_FAIL_MESSAGE("Panicked!");
    }
    // test that we powered up
    TEST_ASSERT_TRUE(Controller::connected());
}

void TestPowerUpIntegration() {
    using Machine = EventQueue<SatComm::Controller, SatComm::Driver>;
    using Controller = SatComm::Controller<Machine>;

    start();
}

void DangerousTestSendData() {
    using Machine = EventQueue<SatComm::Controller, SatComm::Driver>;
    using Controller = SatComm::Controller<Machine>;

    start();

    // test sending data!
    const char message[] = "Hello world!";
    Controller::queue(message, sizeof(message));
    TEST_ASSERT_EQUAL(1, Controller::send_pending());
    Controller::send_now();
    // cycle until done or 5 seconds
    const auto start = millis();
    while (Controller::send_pending() && millis() - start < 5000) {
        if (!Machine::next())
            TEST_FAIL_MESSAGE("Panicked!");
    }
    TEST_ASSERT_EQUAL(0, Controller::send_pending());
}

void DangerousTestReceiveData() {
    using Machine = EventQueue<SatComm::Controller, SatComm::Driver>;
    using Controller = SatComm::Controller<Machine>;

    start();

    // cycle until done or 5 seconds
    const auto start = millis();
    while (millis() - start < 5000) {
        if (!Machine::next())
            TEST_FAIL_MESSAGE("Panicked!");
    }
    // not really much to test, so see if we're connected
    TEST_ASSERT_TRUE(Controller::connected());
}

void process() {
    UNITY_BEGIN();
    RUN_TEST(TestPowerUpIntegration);
    // RUN_TEST(DangerousTestTransmit);
    // RUN_TEST(DangerousTestRecieve);
    UNITY_END();
}

#include "../test_common/TestMain.h"

#endif