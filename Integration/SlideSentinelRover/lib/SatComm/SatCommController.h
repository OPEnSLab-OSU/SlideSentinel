#pragma once

#include <array>
#include "tinyfsm.h"
#include "GlobalEvents.h"
#include "SatCommDriver.h"
#include "CircularBuffer.h"

namespace SatComm {
    // Settings
    // TODO: Somewhere else?
    constexpr size_t OutgoingQueueMax = 25;

    // ----------------------------------------------------------------------------
    // Event declarations
    //

    // Inbound
    struct QueueMessage : tinyfsm::PrintingEvent<QueueMessage> {
        QueueMessage(const char* const msg_in, const size_t len_in)
            : msg(msg_in)
            , len(len_in) {}

        const char* const msg;
        const size_t len;
    };

    struct SendImmediate : tinyfsm::PrintingEvent<SendImmediate> { };

    // Outbound
    struct MessageRecieved : tinyfsm::PrintingEvent<MessageRecieved> {
        MessageRecieved(const Packet& recieved_in)
            : recieved(recieved_in) {}

        const Packet& recieved;
    };

    // ----------------------------------------------------------------------------
    // SatCommController (FSM base class) declaration
    //

    template<class EventBus>
    class Controller
        : public tinyfsm::Fsm<Controller<EventBus>>
    {
        /* NOTE: react(), entry() and exit() functions need to be accessible
         * from tinyfsm::Fsm class. You might as well declare friendship to
         * tinyfsm::Fsm, and make these functions private:
         *
         * friend class Fsm;
         */
    public:

        // SatComm Inbound
        void react(QueueMessage const&);
        void react(SignalLost const&);
        virtual void react(SendImmediate const&) { }
        virtual void react(RingAlert const&) { }
        virtual void react(DriverReady const&) { }
        virtual void react(SendSuccess const&) { }
        virtual void react(SendRecieveSuccess const&) { }
        virtual void react(Success const&) { }
        void react(tinyfsm::Event const&) { }

        // SatComm Driver Inbound

        virtual void entry(void) { };  /* entry actions in some states */
        virtual void exit(void) { };  /* exit actions in some states */

        static void  reset();
        static void  start();

    protected:
        // shared data between states
        // Circular Buffer to store outgoing packets
        static CircularBuffer<Packet, OutgoingQueueMax> m_outgoing;

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
            virtual void react(DriverReady const& e) override {
                // ready to recieve!
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
            
            virtual void react(SendImmediate const& e) override {
                if (!Parent::m_outgoing.empty())
                    Idle::template transit<TXRX>();
            }

            virtual void react(RingAlert const& e) override {
                Idle::template transit<TXRX>();
            }
        };

        // ----------------------------------------------------------------------------
        // State: TXRX
        // Desc: Queues a transmission with then satelitte driver, retransmitting until the internal
        // buffer is emptey or signal is lost
        //
        class TXRX
            : public Parent
        {
            void transmit_success(bool more_packets = false) {
                // deque the packet, since it sent successfully
                Parent::m_outgoing.destroy_front();
                // next, continue to TX only if we have a packet
                // or the device has indicated that there are more packets to recieve
                if (!Parent::m_outgoing.empty() || more_packets)
                    TXRX::template transit<TXRX>();
                else
                    TXRX::template transit<Idle>();
            }
            
            virtual void entry() override {
                // check if the buffer has any items, if not then just recieve
                if (Parent::m_outgoing.empty())
                    EventBus::dispatch(StartRecieve{});
                // queue the top of the buffer to transmit
                else
                    EventBus::dispatch(StartSendRecieve{
                        Parent::m_outgoing.front()
                    });
            }

            virtual void react(SendRecieveSuccess const& e) override {
                // dispatch the message recieved event to our higher ups
                EventBus::dispatch(MessageRecieved{
                    e.recieved
                });
                // transmit worked!
                transmit_success(e.pending_packets > 0);
            }

            virtual void react(SendSuccess const& e) override {
                transmit_success();
            }

            virtual void react(Success const&) override {
                // succeded, but did not transmit or recieve
                // in this case we assume that we started a recieve cycle
                TXRX::template transit<Idle>();
            }
        };

    public:
        using InitialState = Wait;
    };

    template<class EventBus>
    CircularBuffer<Packet, OutgoingQueueMax> Controller<EventBus>::m_outgoing{};

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
}
