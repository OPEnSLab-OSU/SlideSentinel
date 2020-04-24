#pragma once

#undef min
#undef max
#include <array>
#include "tinyfsm.h"
#include "GlobalEvents.h"
#include "SatCommDriverEvents.h"
#include "CircularBuffer.h"

namespace SatComm {
    // Settings
    // TODO: Somewhere else?
    constexpr size_t OutgoingQueueMax = 25;
    constexpr size_t IncomingQueueMax = 5;

    // ----------------------------------------------------------------------------
    // Event declarations
    //

    // Inbound
    struct QueueMessage : tinyfsm::Event {
        QueueMessage(const char* const msg_in, const size_t len_in)
            : msg(msg_in)
            , len(len_in) {}

        const char* const msg;
        const size_t len;
    };

    struct SendImmediate : tinyfsm::Event { };

    // Outbound
    // None for the moment, all events are exposed as static functions for th

    // ----------------------------------------------------------------------------
    // SatCommController (FSM base class) declaration
    // Implements a Queue on top of the Iridium driver.
    //

    template<class EventBus>
    class Controller
        : public tinyfsm::Fsm<Controller<EventBus>>
    {
    public:

        // SatCommController Inbound
        void react(QueueMessage const&);
        void react(SignalLost const&);
        virtual void react(SendImmediate const&) { }
        // SatCommController Driver Inbound
        virtual void react(RingAlert const&) { }
        virtual void react(DriverReady const&) { }
        virtual void react(SendSuccess const&) { }
        virtual void react(SendReceiveSuccess const&) { }
        virtual void react(Success const&) { }
        // misc.
        void react(tinyfsm::Event const&) { }

        virtual void entry() { };  /* entry actions in some states */
        virtual void exit() { };  /* exit actions in some states */

        static void  reset();
        static void  start();

        // User access functions, used in place of outbound events
        static size_t available() { return m_incoming.size(); }
        static void receive(Packet& out) { out = m_incoming.front(); m_incoming.destroy_front(); }
        static bool connected();
        // NOTE: this will be ignored if connected returns false
        static void queue(const char* bytes, size_t count) { Controller::dispatch(QueueMessage{ bytes, count }); }
        static void send_now() { Controller::dispatch(SendImmediate{}); }
        static void recv_now() { Controller::dispatch(RingAlert{}); }
        static size_t send_pending() { return m_outgoing.size(); }

    protected:
        // shared data between states
        // Circular Buffer to store outgoing packets
        static CircularBuffer<Packet, OutgoingQueueMax> m_outgoing;
        static CircularBuffer<Packet, IncomingQueueMax> m_incoming;

        // forward declarations
        class Wait;
        class Idle;
        class TXRX;
        using Parent = Controller<EventBus>;

        // ----------------------------------------------------------------------------
        // State: Wait For Signal (Initial State)
        // Desc: Waits for the satellite modem to send the Ready event.
        //
        class Wait
            : public Controller<EventBus>
        {
            void react(DriverReady const& e) override {
                // ready to receive!
                Wait::template transit<TXRX>();
            }
        };

        // ----------------------------------------------------------------------------
        // State: Idle
        // Desc: Waits for either a periodic timer to elapse, or a Ring alert/Send Immediate event
        //
        class Idle
            : public Parent
        {
            // TODO: timer?
            
            void react(SendImmediate const& e) override {
                if (!Parent::m_outgoing.empty() && !Parent::m_incoming.full())
                    Idle::template transit<TXRX>();
            }

            void react(RingAlert const& e) override {
                Idle::template transit<TXRX>();
            }
        };

        // ----------------------------------------------------------------------------
        // State: TXRX
        // Desc: Queues a transmission with then satellite driver, retransmitting until the internal
        // buffer is empty or signal is lost
        //
        class TXRX
            : public Parent
        {
            void transmit_success(bool more_packets = false) {
                // deque the packet, since it sent successfully
                Parent::m_outgoing.destroy_front();
                // next, continue to TX only if we have a packet
                // or the device has indicated that there are more packets to receive
                if ((!Parent::m_outgoing.empty() && !Parent::m_incoming.full()) || more_packets)
                    TXRX::template transit<TXRX>();
                else
                    TXRX::template transit<Idle>();
            }
            
            void entry() override {
                // check if the buffer has any items, if not then just receive
                if (Parent::m_outgoing.empty())
                    EventBus::dispatch(StartReceive{});
                // queue the top of the buffer to transmit
                else
                    EventBus::dispatch(StartSendReceive{
                        Parent::m_outgoing.front()
                    });
            }

            void react(SendReceiveSuccess const& e) override {
                // store the packet in our incoming queue
                m_incoming.emplace_back(e.received);
                // transmit worked!
                transmit_success(e.pending_packets > 0);
            }

            void react(SendSuccess const& e) override {
                transmit_success();
            }

            void react(Success const&) override {
                // succeed, but did not transmit or receive
                // in this case we assume that we started a receive cycle
                TXRX::template transit<Idle>();
            }
        };

    public:
        using InitialState = Wait;
    };

    template<class EventBus>
    bool Controller<EventBus>::connected() {
        return Controller::template is_in_state<Idle>() || Controller::template is_in_state<TXRX>();
    }

    template<class EventBus>
    void Controller<EventBus>::reset() {
        tinyfsm::StateList<Wait, Idle, TXRX>::reset();
    }

    template<class EventBus>
    void Controller<EventBus>::start() {
        tinyfsm::Fsm<Controller>::start();
    }

    template<class EventBus>
    void Controller<EventBus>::react(QueueMessage const& e) {
        // TODO: what happens when the queue is full?
        // check that the entire message will fit in the queue
        // by counting the number of packets we will create
        if (e.len / PacketSizeBytes + 1 > Parent::m_outgoing.allocated() - Parent::m_outgoing.size())
            return;
        // packetize the message and add it to the queue
        size_t cur_idx = 0;
        while (cur_idx < e.len) {
            // fill a packet with no more than PacketSizeBytes bytes
            if (Parent::m_outgoing.add_back({ {}, std::min(e.len - cur_idx, PacketSizeBytes) })) {
                for (size_t i = 0; i < Parent::m_outgoing.back().length; i++) {
                    Parent::m_outgoing.back().bytes[i] = e.msg[cur_idx + i];
                }
                cur_idx += Parent::m_outgoing.back().length;
            }
            else
                break;
        }
    }

    template<class EventBus>
    void Controller<EventBus>::react(SignalLost const&) {
        // transit to wait from any state
        Controller<EventBus>::template transit<Wait>();
    }

    template<class EventBus>
    CircularBuffer<Packet, OutgoingQueueMax> Controller<EventBus>::m_outgoing{};

    template<class EventBus>
    CircularBuffer<Packet, IncomingQueueMax> Controller<EventBus>::m_incoming{};
}
