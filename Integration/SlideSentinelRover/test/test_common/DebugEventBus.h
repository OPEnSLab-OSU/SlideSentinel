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

#define DEBUG_BUS_POP_EVENT(bus, expected_type) \
    ({ TEST_ASSERT_FALSE(bus::empty()); \
    TEST_ASSERT_SAME_TYPE(expected_type, bus::peek_front_type()); \
    bus::pop<expected_type>(); })

class DebugEventBus {
private:
    using EventPtr = std::unique_ptr<tinyfsm::Event>;
    static std::deque<std::pair<EventPtr, std::type_index>> m_event_list;

public:
    template<typename E>
    static void dispatch(E const& event) {
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