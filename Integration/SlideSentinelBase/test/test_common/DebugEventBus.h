#pragma once

#ifdef UNIT_TEST

#include <memory>
#include <deque>
#include <typeinfo>
#include <typeindex>
#include <unordered_map>
#include "tinyfsm.h"
#include "unity.h"
#include "UnitySameType.h"

/**
 * This macro pops an event off of the internal stack in DebugEventBus.
 * If the type of the event is not expected_type, this macro calls TEST_FAIL, 
 * otherwise it returns the popped event.
 * @returns The event with type expected_type
 */
#define DEBUG_BUS_POP_EVENT(bus, expected_type) \
    ({ TEST_ASSERT_FALSE(bus::empty()); \
    TEST_ASSERT_SAME_TYPE(expected_type, bus::peek_front_type()); \
    bus::pop<expected_type>(); })


/**
 * @brief A mock for EventQueue, designed to capture output events from a state machine for testing
 * 
 * DebugEventBus is a testing tool that allows a developer to capture
 * events emitted from a state machine, and verify both the order and
 * type of events during the test. 
 * 
 * Events are stored in a FILO order internally, and can be accessed using
 * the DEBUG_BUS_POP_EVENT macro. This macro both verifies that the type of
 * the event is correct, and gets a copy of the event for further verification.
 * A usage example is as follows:
 * ```C++
 * // declare the state machine with DebugEventBus as the output
 * using TestStateMachine = StateMachine<DebugEventBus>;
 * 
 * // reset the DebugEventBus and StateMachine
 * DebugEventBus::reset();
 * TestStateMachine::reset();
 * TestStateMachine::start();
 * 
 * // send some input events to the state machine
 * TestStateMachine::dispatch(...);
 * ...
 * 
 * // Verify that the last event dispatched was of type Event1
 * DEBUG_BUS_POP_EVENT(DebugEventBus, Event1);
 * // Verify that the second to last event dispatched was of type Event2
 * // and that the contents of that event are == 3;
 * auto event = DEBUG_BUS_POP_EVENT(DebugEventBus, Event2);
 * TEST_ASSERT_EQUAL(3, event.value);
 * // Verify that these two events are the only two events dispatched
 * TEST_ASSERT_TRUE(DebugEventBus::empty());
 * ```
 * @note RTTI must be enabled to use this tool.
 */
class DebugEventBus {
private:
    using EventPtr = std::unique_ptr<tinyfsm::Event>;
    static std::deque<std::pair<EventPtr, std::type_index>> m_event_list;

public:
    template<typename E>
    static void dispatch(E const& event) {
        char buf[256];
        snprintf(buf, sizeof(buf), "DebugEventBus Dispatch Event: %s", typeid(E).name());
        UnityMessage(buf, 0);
        // copy the event and store it into a vector so we can check it later
        EventPtr temp = EventPtr(new E(event));
        m_event_list.emplace_back(std::move(temp), typeid(E));
    }

    template<typename E>
    static void react(E const& event) {
        dispatch<E>(event);
    }

    static const std::type_index& peek_front_type() {
        if (m_event_list.empty())
            TEST_FAIL_MESSAGE("Peeked when there is no types available!");
        return m_event_list.front().second;
    }

    template<class E>
    static E pop() {
        if (m_event_list.empty())
            TEST_FAIL_MESSAGE("No more events!");
        TEST_ASSERT_SAME_TYPE(E, peek_front_type());
        EventPtr temp = std::move(m_event_list.front().first);
        m_event_list.pop_front();
        return E(*static_cast<const E*>(temp.get()));
    }

    static size_t count() {
        return m_event_list.size();
    }

    static bool empty() {
        return m_event_list.empty();
    }

    static void reset() {
        m_event_list.clear();
    }
};

std::deque<std::pair<std::unique_ptr<tinyfsm::Event>, std::type_index>>  DebugEventBus::m_event_list{};

template<typename T>
using DebugEventBusProxy = DebugEventBus;

#endif