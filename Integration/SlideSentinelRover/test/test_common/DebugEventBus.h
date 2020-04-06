#pragma once

#include <memory>
#include <deque>
#include "tinyfsm.h"

class DebugEventPtr : public std::unique_ptr<tinyfsm::Event> {
public:
    DebugEventPtr(tinyfsm::Event* e)
        : std::unique_ptr<tinyfsm::Event>(e) {}

    DebugEventPtr()
        : std::unique_ptr<tinyfsm::Event>() {}

    template<typename T>
    const T& as() const {
        return *static_cast<const T*>(get());
    }
};

class DebugEventBus {
private:
    using EventPtr = DebugEventPtr;
    static std::deque<EventPtr> m_event_list;

public:
    template<typename E>
    static void dispatch(E const& event) {
        // copy the event and store it into a vector so we can check it later
        m_event_list.emplace_back(new E(event));
    }

    static EventPtr front() {
        if (m_event_list.size() == 0)
            return EventPtr{};
        EventPtr temp = std::move(m_event_list.front());
        m_event_list.pop_front();
        return std::move(temp);
    }

    static bool empty() {
        return m_event_list.empty();
    }

    static void reset() {
        m_event_list.clear();
    }
};

std::deque<DebugEventPtr> DebugEventBus::m_event_list{};