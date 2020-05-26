#pragma once

#include "tinyfsm.h"
#include "CircularBuffer.h"
#include "CircularHeap.h"
#include "GlobalEvents.h"
#include "LogMacros.h"
#include <typeinfo>
#include <cstdio>

template<template<class> class... Sources>
class EventQueue {
private:
    using List = tinyfsm::FsmList<Sources<EventQueue>...>;

    template<class E>
    static void dispatch_base(const void* event) {
        List::dispatch(*static_cast<const E*>(event));
    }

    template<typename E>
    static void dispatch_base_empty(const void *) {
        List::dispatch(E{});
    }

    using DispatchPtr = void(*)(const void*);

    struct DispatchStruct {
        DispatchStruct(DispatchPtr i_ptr, bool i_is_empty)
            : ptr(i_ptr)
            , is_empty(i_is_empty) {}

        DispatchPtr ptr;
        bool is_empty;
    };

    static CircularHeap<2048> m_ev_buffer;
    static CircularBuffer<DispatchStruct, 32> m_dis_buffer;
    static bool m_did_panic;

public:
    template<typename E>
    static void dispatch(E const& event) {
        if (std::is_same<E, Panic>::value) {
            m_did_panic = true;
            return;
        }
        if (m_did_panic)
            return;
        if (m_dis_buffer.full()) {
          char buf[128];
          snprintf(buf, sizeof buf, "Discarding event %s due to full buffer", typeid(E).name());
          MSG_WARN(buf);
        }
        else
          MSG_INFO(typeid(E).name());
        // if the event is empty, just copy the dispatch pointer
        if (std::is_empty<E>::value)
            m_dis_buffer.emplace_back(&(EventQueue::template dispatch_base_empty<E>), true);
        else {
            // copy the event into our "heap"
            if (!m_ev_buffer.allocate_push_back<E>(event))
                return;
            // and store both the size of the event and the pointer to dispatch it
            m_dis_buffer.emplace_back(&(EventQueue::template dispatch_base<E>), false);
        }
    }

    static bool next() {
        if (!m_did_panic) {
            if (!m_dis_buffer.empty()) {
                DispatchStruct disptr = m_dis_buffer.front();
                // get the function pointer, which contains the event type to dispatch for
                // and call it
                // note that for empty types this pointer may be invalid, which is fine b/c
                // it will be ignored.
                if (disptr.is_empty)
                    disptr.ptr(nullptr);
                else {
                    disptr.ptr(m_ev_buffer.get_front());
                    // destroy the pointer, deallocating the size of the event stored with the dispatch pointer
                    m_ev_buffer.deallocate_pop_front();
                }
                m_dis_buffer.destroy_front();
            }
            else {
                const Update update{};
                dispatch_base<Update>(&update);
            }
        }
        return !m_did_panic;
    }

    static size_t queued() {
        return m_dis_buffer.size();
    }

    static void reset() {
        List::reset();
        m_dis_buffer.reset();
        m_ev_buffer.clear();
        m_did_panic = false;
    }

    static void start() {
        List::start();
    }
};

template<template<class> class... S>
CircularHeap<2048> EventQueue<S...>::m_ev_buffer{};

template<template<class> class... S>
CircularBuffer<typename EventQueue<S...>::DispatchStruct, 32> EventQueue<S...>::m_dis_buffer{};

template<template<class> class... S>
bool EventQueue<S...>::m_did_panic = false;
