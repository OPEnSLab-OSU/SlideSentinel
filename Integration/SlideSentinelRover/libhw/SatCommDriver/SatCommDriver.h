//
// Created by Noah on 4/8/2020.
//

#ifndef SLIDESENTINELROVER_SATCOMMDRIVER_H
#define SLIDESENTINELROVER_SATCOMMDRIVER_H

#include <atomic>
#include <IridiumSBD.h>
#include "Arduino.h"
#include "tinyfsm.h"
#include "GlobalEvents.h"
#include "SatCommDriverEvents.h"

namespace SatComm {
    constexpr auto RING_INDICATOR_PIN = A3;
    constexpr auto NET_AV_PIN = 19;
    constexpr auto IridiumBaud = 19200;
    constexpr auto& IridiumSerial = Serial1;

    // ----------------------------------------------------------------------------
    // SatCommDriver (FSM base class) declaration
    //
    // TODO: PowerDown, invalid events, reset/start
    template<class EventBus>
    class Driver
            : public tinyfsm::Fsm<Driver<EventBus>>
    {
    public:
        // Global Events Inbound
        virtual void react(const Update&) { }
        virtual void react(const PowerUp&) { }
        virtual void react(const PowerDown&);

        // SatCommDriver Events Inbound
        virtual void react(const StartSendReceive&);
        virtual void react(const StartReceive&);

        // misc.
        void react(tinyfsm::Event const&) { }

        virtual void entry(void) { };  /* entry actions in some states */
        virtual void exit(void) { };  /* exit actions in some states */

        static void  reset();
        static void  start();

    protected:
        // Shared global data
        // IridumSBD driver
        static IridiumSBD m_modem;

        // forward decelerations
        using Parent = Driver<EventBus>;
        class Off;
        class Wait;
        class Ready;

        // ----------------------------------------------------------------------------
        // State: Off (Initial State)
        // Desc: Waits for the PowerUp signal to be sent
        //
        class Off : public Parent
        {
            void react(const PowerUp&) override {
                // power up actions
                // set the RING pin to input
                pinMode(RING_INDICATOR_PIN, INPUT);
                pinMode(NET_AV_PIN, INPUT); //Network availability
                // start serial bus
                IridiumSerial.begin(IridiumBaud);
                // start the driver
                int result = Parent::m_modem.begin();
                if (result != ISBD_SUCCESS) {
                    // TODO: Panic?
                }
                // log the firmware version
                char version[12] = {};
                result = Parent::m_modem.getFirmwareVersion(version, sizeof(version));
                if (result != ISBD_SUCCESS) {
                    // TODO: Panic?
                }
                Serial.print("Firmware version: ");
                Serial.println(version);
                // we've successfully powered on!
                Parent::template transit<Wait>();
            }
        };

        // ----------------------------------------------------------------------------
        // State: Wait
        // Desc: Waits for a satellite signal to be available by checking the NET_AV pin
        //
        class Wait : public Parent
        {
            void entry() override {
                EventBus::dispatch(SignalLost{});
            }

            void react(const Update&) override {
                if (digitalRead(NET_AV_PIN))
                    Parent::template transit<Ready>();
            }
        };

        // ----------------------------------------------------------------------------
        // State: Ready
        // Desc: Waits for the controller to send a packet, and attaches an interrupt
        // to the RING pin.
        //
        class Ready : public Parent
        {
        public:

            static void handleRing() {
                m_did_ring.store(true);
            }

            void entry() override {
                // set did_ring to false
                m_did_ring.store(false);
                // register ring alert interrupt
                attachInterrupt(RING_INDICATOR_PIN, handleRing, LOW);
                // ready!
                EventBus::dispatch(DriverReady{});
            }

            void exit() override {
                // de-register ring alert interrupt
                detachInterrupt(RING_INDICATOR_PIN);
            }

            // TODO: Do we need to check signal every time?
            void react(const Update&) override {
                // if we got a ring alert interrupt, follow it
                if (m_did_ring.load()) {
                    m_did_ring.store(false);
                    if (Parent::m_modem.hasRingAsserted() || Parent::m_modem.getWaitingMessageCount() > 0)
                        EventBus::dispatch(RingAlert{});
                }
                // if signal is lost, transit out of ready
                else if (!digitalRead(NET_AV_PIN))
                    Parent::template transit<Wait>();
            }

            // trigger a transmission!
            void react(const StartSendReceive& e) override {
                // check signal
                if (!digitalRead(NET_AV_PIN))
                    return Parent::template transit<Wait>();
                // good to go! send/receive the packet
                Packet buf{ {}, 0 };
                size_t bufmax = buf.bytes.max_size(); // NOTE: this is also an output parameter
                int err = Parent::m_modem.sendReceiveSBDBinary(
                        reinterpret_cast<const uint8_t*>(e.send.bytes.data()),
                        e.send.length,
                        reinterpret_cast<uint8_t*>(buf.bytes.data()),
                        bufmax);
                // check error
                if (err != ISBD_SUCCESS) {
                    // TODO: Panic differently for different errors?
                    return Parent::template transit<Wait>();
                }
                // if we received any data, copy it and indicate need for further data
                if (bufmax > 0) {
                    // get the number of messages pending reception
                    err = Parent::m_modem.getWaitingMessageCount();
                    if (err < 0) {
                        // TODO: Panic
                        return Parent::template transit<Wait>();
                    }
                    // send a success event!
                    EventBus::dispatch(SendReceiveSuccess{ buf, static_cast<size_t>(err) });
                }
                else if (e.send.length > 0)
                    EventBus::dispatch(SendSuccess{});
                else
                    EventBus::dispatch(Success{});
            }

            void react(const StartReceive& e) override {
                // redirect to StartSendReceive, but with an empty packet
                react(StartSendReceive{{ {}, 0 }});
            }

        private:
            // boolean to detect ring alerts
            static std::atomic_bool m_did_ring;
        };
    };

    template <class T>
    IridiumSBD Driver<T>::m_modem(IridiumSerial, -1, RING_INDICATOR_PIN);

    template <class T>
    std::atomic_bool Driver<T>::Ready::m_did_ring{ false };
};

#endif //SLIDESENTINELROVER_SATCOMMDRIVER_H
