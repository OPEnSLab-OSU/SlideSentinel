//
// Created by Noah on 4/17/2020.
//

#ifdef UNIT_TEST

#include "EventQueue.h"
#include "../test_common/DebugEventBus.h"
#include "tinyfsm.h"
#include <array>
#include "unity.h"

struct TestEvent1 : tinyfsm::Event {};
struct TestEvent2 : tinyfsm::Event {
    int thing1 = 2;
    long thing2 = 84573;
    double thing3 = 2;
    uint8_t thing4 = 244;
    uint16_t thing5 = 2323;
    char name[15] = "TestEvent2";
};
struct TestEvent3 : tinyfsm::Event {
    std::array<uint16_t, 25> myarr{ { 6,2,43,435,245,23,3,123,322,23 } };
};

static void VerifyTestEvent2(const TestEvent2& ev) {
    constexpr TestEvent2 truth{};
    TEST_ASSERT_EQUAL_INT(truth.thing1, ev.thing1);
    TEST_ASSERT_EQUAL_INT32(truth.thing2, ev.thing2);
    TEST_ASSERT_EQUAL_FLOAT(truth.thing3, ev.thing3);
    TEST_ASSERT_EQUAL_UINT8(truth.thing4, ev.thing4);
    TEST_ASSERT_EQUAL_UINT16(truth.thing5, ev.thing5);
    TEST_ASSERT_EQUAL_CHAR_ARRAY(truth.name, ev.name, sizeof(truth.name));
}

void TestEventQueueInitial() {
    using TestEventQueue = EventQueue<DebugEventBusProxy>;

    TestEventQueue::reset();
    DebugEventBus::reset();

    TEST_ASSERT_EQUAL(0, TestEventQueue::queued());
    TEST_ASSERT_TRUE(DebugEventBus::empty());
}

void TestEventQueueReset() {
    using TestEventQueue = EventQueue<DebugEventBusProxy>;

    TestEventQueue::reset();
    DebugEventBus::reset();

    TestEventQueue::dispatch(TestEvent1());
    TestEventQueue::dispatch(TestEvent2());
    TestEventQueue::dispatch(TestEvent3());
    TEST_ASSERT_TRUE(DebugEventBus::empty());
    TEST_ASSERT_EQUAL(3, TestEventQueue::queued());

    TestEventQueue::reset();
    TEST_ASSERT_TRUE(DebugEventBus::empty());
    TEST_ASSERT_EQUAL(0, TestEventQueue::queued());
}

void TestEventQueuePanic() {
    using TestEventQueue = EventQueue<DebugEventBusProxy>;

    TestEventQueue::reset();
    DebugEventBus::reset();

    TestEventQueue::dispatch(TestEvent1());
    TestEventQueue::dispatch(Panic{});
    TestEventQueue::dispatch(TestEvent2());

    TEST_ASSERT_FALSE(TestEventQueue::next());
    TEST_ASSERT_TRUE(DebugEventBus::empty());
}

void TestEventQueueDispatch() {
    using TestEventQueue = EventQueue<DebugEventBusProxy>;

    TestEventQueue::reset();
    DebugEventBus::reset();

    TestEventQueue::dispatch(TestEvent1());
    TEST_ASSERT_TRUE(DebugEventBus::empty());
    TEST_ASSERT_EQUAL(1, TestEventQueue::queued());

    TestEventQueue::next();
    DEBUG_BUS_POP_EVENT(DebugEventBus, TestEvent1);
    TEST_ASSERT_EQUAL(0, TestEventQueue::queued());

    TestEventQueue::next();
    DEBUG_BUS_POP_EVENT(DebugEventBus, Update);

    TEST_ASSERT_TRUE(DebugEventBus::empty());
}

void TestEventQueueDispatchOrder() {
    using TestEventQueue = EventQueue<DebugEventBusProxy>;

    TestEventQueue::reset();
    DebugEventBus::reset();

    TestEventQueue::dispatch(TestEvent1());
    TestEventQueue::dispatch(TestEvent2());
    TestEventQueue::dispatch(TestEvent3());
    TEST_ASSERT_TRUE(DebugEventBus::empty());
    TEST_ASSERT_EQUAL(3, TestEventQueue::queued());

    TestEventQueue::next();
    TestEventQueue::next();
    TestEventQueue::next();
    TEST_ASSERT_EQUAL(0, TestEventQueue::queued());
    TestEventQueue::next();

    DEBUG_BUS_POP_EVENT(DebugEventBus, TestEvent1);
    VerifyTestEvent2(DEBUG_BUS_POP_EVENT(DebugEventBus, TestEvent2));
    TEST_ASSERT_TRUE(DEBUG_BUS_POP_EVENT(DebugEventBus, TestEvent3).myarr == TestEvent3().myarr);
    DEBUG_BUS_POP_EVENT(DebugEventBus, Update);
    TEST_ASSERT_TRUE(DebugEventBus::empty());
}

void TestEventQueueCircular() {
    using TestEventQueue = EventQueue<DebugEventBusProxy>;

    TestEventQueue::reset();
    DebugEventBus::reset();

    TestEventQueue::dispatch(TestEvent1());
    TestEventQueue::dispatch(TestEvent2());
    TestEventQueue::dispatch(TestEvent3());
    TEST_ASSERT_TRUE(DebugEventBus::empty());
    TEST_ASSERT_EQUAL(3, TestEventQueue::queued());

    for (size_t i = 0; i < 300; i++) {
        TestEventQueue::next();
        DEBUG_BUS_POP_EVENT(DebugEventBus, TestEvent1);
        TEST_ASSERT_TRUE(DebugEventBus::empty());

        TestEventQueue::dispatch(TestEvent1());
        TEST_ASSERT_EQUAL(3, TestEventQueue::queued());

        TestEventQueue::next();
        VerifyTestEvent2(DEBUG_BUS_POP_EVENT(DebugEventBus, TestEvent2));
        TEST_ASSERT_TRUE(DebugEventBus::empty());

        TestEventQueue::dispatch(TestEvent2());
        TEST_ASSERT_EQUAL(3, TestEventQueue::queued());

        TestEventQueue::next();
        TEST_ASSERT_TRUE(DEBUG_BUS_POP_EVENT(DebugEventBus, TestEvent3).myarr == TestEvent3().myarr);
        TEST_ASSERT_TRUE(DebugEventBus::empty());

        TestEventQueue::dispatch(TestEvent3());
        TEST_ASSERT_EQUAL(3, TestEventQueue::queued());
    }

    TestEventQueue::next();
    TestEventQueue::next();
    TestEventQueue::next();
    TEST_ASSERT_EQUAL(0, TestEventQueue::queued());
    TestEventQueue::next();

    DEBUG_BUS_POP_EVENT(DebugEventBus, TestEvent1);
    VerifyTestEvent2(DEBUG_BUS_POP_EVENT(DebugEventBus, TestEvent2));
    TEST_ASSERT_TRUE(DEBUG_BUS_POP_EVENT(DebugEventBus, TestEvent3).myarr == TestEvent3().myarr);
    DEBUG_BUS_POP_EVENT(DebugEventBus, Update);
    TEST_ASSERT_TRUE(DebugEventBus::empty());
}

void process() {
    UNITY_BEGIN();
    RUN_TEST(TestEventQueueInitial);
    RUN_TEST(TestEventQueueReset);
    RUN_TEST(TestEventQueuePanic);
    RUN_TEST(TestEventQueueDispatch);
    RUN_TEST(TestEventQueueDispatchOrder);
    RUN_TEST(TestEventQueueCircular);
    UNITY_END();
}

#include "../test_common/TestMain.h"

#endif // UNIT_TEST
