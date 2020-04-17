//
// Created by Noah on 4/8/2020.
//

#ifndef SLIDESENTINELROVER_SATCOMMDRIVER_H
#define SLIDESENTINELROVER_SATCOMMDRIVER_H

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
        // buffer packet used to store send/recv data
        static Packet m_buffer;

        // forward decelerations
        using Parent = Driver<EventBus>;
        class Off;
        class Wait;
        class Ready;
        class SendReceive;
        class Receive;

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
        // Desc: Waits for the controller to send a packet
        //
        class Ready : public Parent
        {
            void entry() override {
                EventBus::dispatch(DriverReady{});
            }

            // trigger a transmission!
            void react(const StartSendReceive& e) override {
                // check signal
                if (!digitalRead(NET_AV_PIN))
                    Parent::template transit<Wait>();
                // check packet validity
                else if (e.send.length > 0) {
                    // write the packet to an internal buffer
                    Parent::m_buffer = e.send;
                    // transit to SendReceive
                    Parent::template transit<SendReceive>();
                }
                // else just receive
                else
                    Parent::template transit<Receive>();

            }

            void react(const StartReceive& e) override {
                // check signal
                if (!digitalRead(NET_AV_PIN))
                    Parent::template transit<Wait>();
                else
                    Parent::template transit<Receive>();
            }
        };

        // ----------------------------------------------------------------------------
        // State: StartSendReceive
        // Desc: Writes a packet to the modem, then reads a packet from the modem if one
        // is available
        //
        class StartSendReceive : public Parent
        {
            void react(const Update&) override {
                size_t bufmax = Parent::m_buffer.bytes.max_size();
                int err = Parent::m_modem.sendReceiveSBDBinary(
                        reinterpret_cast<uint8_t*>(Parent::m_buffer.bytes.data()),
                        Parent::m_buffer.length,
                        reinterpret_cast<uint8_t*>(Parent::m_buffer.bytes.data()),
                        bufmax);
                // check error
                if (err != ISBD_SUCCESS) {
                    // TODO: Panic differently for different errors?
                    Parent::template transit<Wait>();
                    return;
                }
                // dispatch a
            }
        };
    };

    template <class T>
    IridiumSBD Driver<T>::m_modem(IridiumSerial, -1, RING_INDICATOR_PIN);

    template <class T>
    Packet Driver<T>::m_buffer = {{}, 0};
}

#endif //SLIDESENTINELROVER_SATCOMMDRIVER_H
