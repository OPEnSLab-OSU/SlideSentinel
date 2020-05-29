#pragma once

#include "tinyfsm.h"
#include "CircularBuffer.h"
#include "CircularHeap.h"
#include "GlobalEvents.h"
#include "LogMacros.h"
#include <cstdio>
#include "ctti/nameof.hpp"

/**
 * @brief A run-to-completion event bus designed to be used with TinyFSM.
 *
 * EventQueue is an alternative to TinyFSM's regular event dispatching system that implements
 * run-to-completion. Previously, TinyFSM events were processed immediately during dispatch,
 * which resulted in strange behavior if events called other events.
 * ```C++
 * void some_func() {
 *      dispatch(Event1{});
 *      dispatch(Event2{});
 * }
 *
 * void react(const Event1&) {
 *      dispatch(Event3{});
 * }
 *
 * void react(const Event2&) {}
 * ```
 * Take the above code snippet for example. Intuitively, we would expect the following
 * order of events when calling `some_func`:
 * ```
 * Event1
 * Event2
 * Event3
 * ```
 * As we would expect `some_func` to finish executing before it's events are processed--
 * in other words, we would expect it to *run to completion*. TinyFSM, however, chooses
 * to process each event *at the place where it is dispatched*, generating the following
 * sequence of events for the same code snippet:
 * ```
 * Event1
 * Event3 <-- react(const Event1&) is called immediately instead of being delayed
 * Event2
 * ```
 * TinyFSM does this to reduce the need for copying events, and as a result it is very
 * memory efficient. This approach, however, assumes that the state machines have no
 * circular dependencies, as they will generate an infinite loop:
 * ```C++
 * void react(const Event2&) {
 *      dispatch(Event1{}); //<-- will immediately call react(const Event2&)
 * }
 *
 * void react(const Event2&) {
 *      dispatch(Event1{}); //<-- will immediately call react(const Event1&)
 * }
 * ```
 * Where TinyFSM recursions indefinitely, run-to-completion queues the event
 * and waits for each call to `react` to complete, preventing a stack overflow.
 * Our design (SatComm) required the use of circular dependencies, and as a result
 * we could not use TinyFSM's built in event system. EventQueue add the missing
 * components of a run-to-completion event system to TinyFSM.
 *
 * EventQueue is implemented in terms of two components: a FIFO CircularHeap to serve
 * as type-agnostic temporary event storage, and CircularBuffer to store type-agnostic
 * function pointers to the appropriate dispatch action. Combined, these elements
 * can queue and dispatch any event type without using RTTI.
 *
 * There are two events that are handled specially by EventQueue: Update and Panic.
 * If a Panic event is dispatched, EventQueue will enter an error state and refuse
 * to dispatch further events until it is reset. EventQueue will automatically dispatch
 * Update events if the internal queue is empty.
 *
 * @tparam Sources State machines that EventQueue can dispatch to.
 */
template<template<class> class... Sources>
class EventQueue {
private:
    using List = tinyfsm::FsmList<Sources<EventQueue>...>;
    using DispatchPtr = void(*)(const void*);

    struct DispatchStruct {
        DispatchStruct(DispatchPtr i_ptr, bool i_is_empty)
            : ptr(i_ptr)
            , is_empty(i_is_empty) {}

        DispatchPtr ptr;
        bool is_empty;
    };

        template<class E>
    static void dispatch_base(const void* event) {
        List::dispatch(*static_cast<const E*>(event));
    }

    template<typename E>
    static void dispatch_base_empty(const void *) {
        List::dispatch(E{});
    }

    static CircularHeap<2048> m_ev_buffer;
    static CircularBuffer<DispatchStruct, 32> m_dis_buffer;
    static bool m_did_panic;

public:
    /**
     * Queue an event to be dispatched later by EventQueue::next. Events are
     * queued in FIFO fashion. If the queue is full, or EventQueue has panicked
     * the event is discarded.
     *
     * This version is for non-empty types, and will copy the event into the queue.
     * @tparam E The event type.
     * @param event The event to queue.
     */
    template<typename E>
    static typename std::enable_if<!std::is_empty<E>::value && !std::is_same<E, Panic>::value>::type dispatch(E const& event) {
        if (m_did_panic)
            return;
        if (m_dis_buffer.full()) {
            char buf[128];
            snprintf(buf, sizeof buf, "Discarding event %s due to full buffer", ctti::nameof<E>().begin());
            MSG_WARN(buf);
            return;
        }
        MSG_INFO(ctti::nameof<E>().begin());
        // copy the event into our "heap"
        if (!m_ev_buffer.allocate_push_back<E>(event))
            return;
        // bit of cleverness here: we store a pointer to a template instantiation of a static function
        // which dispatches the correct event.
        // This gets around the fact that we don't have RTTI but still need to know how to dispatch
        // the event based on its type.
        // The boolean indicated that the event is non-empty: as an optimization,
        // we construct empty events inside the function so they don't take up
        // space in our heap.
        m_dis_buffer.emplace_back(&(EventQueue::template dispatch_base<E>), false);
    }

    /**
     * Queue an event to be dispatched later by EventQueue::next. Events are
     * queued in FIFO fashion. If the queue is full, or EventQueue has panicked
     * the event is discarded.
     *
     * This version is for empty types. The event parameter will be ignored,
     * and a new event will be default-constructed on dispatch.
     * @tparam E The event type.
     * @param event The event to queue. This value will be ignored.
     */
    template<typename E>
    static typename std::enable_if<std::is_empty<E>::value && !std::is_same<E, Panic>::value>::type dispatch(E const& event) {
      if (m_did_panic)
            return;
        if (m_dis_buffer.full()) {
            char buf[128];
            snprintf(buf, sizeof buf, "Discarding event %s due to full buffer", ctti::nameof<E>().begin());
            MSG_WARN(buf);
            return;
        }
        MSG_INFO(ctti::nameof<E>().begin());
        // bit of cleverness here: we store a pointer to a template instantiation of a static function
        // which dispatches the correct event.
        // This gets around the fact that we don't have RTTI but still need to know how to dispatch
        // the event based on its type.
        // The boolean indicated that the event is empty: as an optimization,
        // we construct empty events inside the function so they don't take up
        // space in our heap.
        m_dis_buffer.emplace_back(&(EventQueue::template dispatch_base_empty<E>), true);
    }

    /**
     * Queue an event to be dispatched later by EventQueue::next. Events are
     * queued in FIFO fashion. If the queue is full, or EventQueue has panicked
     * the event is discarded.
     *
     * This version is for Panic. The EventQueue will enter a panic state, and
     * no longer dispatch or queue any further events until it is reset.
     * @tparam E The event type.
     * @param event The event to queue. This value will be ignored.
     */
    template<typename E>
    static typename std::enable_if<std::is_same<E, Panic>::value>::type dispatch(const E& event) {
        m_did_panic = true;
        return;
    }

    /**
     * Dispatch the least-recently queued event, or Update if the queue is empty.
     * @return true if the dispatch was successful, false if EventQueue has panicked.
     */
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
