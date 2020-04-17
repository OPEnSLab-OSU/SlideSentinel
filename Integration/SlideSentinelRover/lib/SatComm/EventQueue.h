#pragma once

#include "tinyfsm.h"

template<template<class> class... Sources>
class EventQueue {
private:
    using List = tinyfsm::FsmList<Sources<EventQueue>...>;

    template<class E>
    static void dispatch_base(const void* event) {
        List::dispatch(*static_cast<const E*>(event));
    }

    using DispatchPtr = void(*)(const void*);

    static CircularHeap<2048> m_ev_buffer;
    static CircularBuffer<std::pair<DispatchPtr, size_t>, 32> m_dis_buffer;

public:
    template<typename E>
    static void dispatch(E const& event) {
        if (m_dis_buffer.full())
            return;
        // copy the event into our "heap"
        if (!m_ev_buffer.allocate_push_back<E>(event))
            return;
        // and store both the size of the event and the pointer to dispatch it
        m_dis_buffer.emplace_back(&(EventQueue::template dispatch_base<E>), sizeof(E));
    }

    static void next() {
        if (!m_ev_buffer.empty()) {
            const std::pair<DispatchPtr, size_t> disptr = m_dis_buffer.front();
            // get the function pointer, which contains the event type to dispatch for
            // and call it
            disptr.first(m_ev_buffer.get_front());
            // destroy the pointer, deallocating the size of the event stored with the dispatch pointer
            m_ev_buffer.deallocate_pop_front(disptr.second);
            m_dis_buffer.destroy_front();
        }
    }

    static void reset() {
        m_dis_buffer.reset();
        m_ev_buffer.clear();
    }
};

template<template<class> class... S>
CircularHeap<2048> EventQueue<S...>::m_ev_buffer{};

template<template<class> class... S>
CircularBuffer<std::pair<void(*)(const void*), size_t>, 32> EventQueue<S...>::m_dis_buffer{};
