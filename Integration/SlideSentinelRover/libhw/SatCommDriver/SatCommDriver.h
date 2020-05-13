//
// Created by Noah on 4/8/2020.
//

#ifndef SLIDESENTINELROVER_SATCOMMDRIVER_H
#define SLIDESENTINELROVER_SATCOMMDRIVER_H

#include <atomic>
#include <IridiumSBD.h>
#include "Arduino.h"
#include "wiring_private.h" // pinPeripheral() function
#include "tinyfsm.h"
#include "GlobalEvents.h"
#include "SatCommDriverEvents.h"
#include "PanicHandler.h"

namespace SatComm {
    constexpr auto RING_INDICATOR_PIN = A3;
    constexpr auto NET_AV_PIN = A5;
    constexpr auto IRIDIUM_TX = 11;
    constexpr auto IRIDIUM_RX = 13;
    constexpr auto IridiumBaud = 19200;
    Uart IridiumSerial(&sercom1, IRIDIUM_RX, IRIDIUM_TX, SERCOM_RX_PAD_1, UART_TX_PAD_0);
    
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

        // SatCommDriver Events Inbound
        // Default dispatch an error
        virtual void react(const StartSendReceive&) { EventBus::dispatch(SignalLost{}); }
        virtual void react(const StartReceive&) { EventBus::dispatch(SignalLost{}); }

        // misc.
        void react(tinyfsm::Event const&) { }

        virtual void entry() { };  /* entry actions in some states */
        virtual void exit() { };  /* exit actions in some states */

        static void  reset();
        static void  start();

    protected:
        // Shared global data
        // IridumSBD driver
        // TODO: a more elegent solution to this problem
        // maybe unique_ptr?
        static IridiumSBD* m_modem;

        // forward decelerations
        using Parent = Driver<EventBus>;
        class Off;
        class Wait;
        class Ready;
        using StateList = tinyfsm::StateList<Off, Wait, Ready>;

    public:
        // Initial state
        using InitialState = Off;

    protected:
        // ----------------------------------------------------------------------------
        // State: Off (Initial State)
        // Desc: Waits for the PowerUp signal to be sent
        //
        class Off : public Parent
        {
            // Note: this can take up to 2 minutes
            void react(const PowerUp&) override {
                // power up actions
                // set the RING pin to input
                pinMode(RING_INDICATOR_PIN, INPUT);
                pinMode(NET_AV_PIN, INPUT); //Network availability
                // start serial bus
                IridiumSerial.begin(IridiumBaud);
                pinPeripheral(IRIDIUM_TX, PIO_SERCOM); //Private functions for serial communication
                pinPeripheral(IRIDIUM_RX, PIO_SERCOM);
                // start the driver
                int result;
                result = Parent::m_modem->begin();
                if (result != ISBD_SUCCESS && result != ISBD_ALREADY_AWAKE) {
                    char message[64];
                    snprintf(message, sizeof(message), "Modem failed to begin with error: %d", result);
                    PANIC(message);
                    EventBus::dispatch(Panic{});
                    return;
                }
                // log the firmware version
                char version[12] = {};
                size_t try_count = 0;
                do {
                    result = Parent::m_modem->getFirmwareVersion(version, sizeof(version));
                } while(result != ISBD_SUCCESS && try_count++ < 5);
                if (result != ISBD_SUCCESS) {
                    char message[64];
                    snprintf(message, sizeof(message), "Modem version failed with error: %d", result);
                    PANIC(message);
                    EventBus::dispatch(Panic{});
                }
                // Serial.print("Firmware version: ");
                // Serial.println(version);
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
                int sigqual = 0;
                if (digitalRead(NET_AV_PIN)) {
                    int err = m_modem->getSignalQuality(sigqual);
                    if (err != ISBD_SUCCESS) {
                        char message[128];
                        snprintf(message, sizeof(message), "Signal quality failed with error: %d", err);
                        PANIC(message);
                        EventBus::dispatch(Panic{});
                    }
                    if (sigqual > 0)
                        Parent::template transit<Ready>();
                }
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
                detachInterrupt(RING_INDICATOR_PIN);
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
                    EventBus::dispatch(RingAlert{});
                    m_did_ring.store(false);
                    // make sure that we don't trigger ourselved again from the same interrupt
                    attachInterrupt(RING_INDICATOR_PIN, handleRing, FALLING);
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
                int err = Parent::m_modem->sendReceiveSBDBinary(
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
                    buf.length = bufmax;
                    // get the number of messages pending reception
                    err = Parent::m_modem->getWaitingMessageCount();
                    if (err < 0) {
                        // I guess we lost the signal
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
    void Driver<T>::reset() {
        if (m_modem != nullptr)
            delete m_modem;
        m_modem = new IridiumSBD(IridiumSerial, -1, RING_INDICATOR_PIN);
        IridiumSerial.end();
        StateList::reset();
    }

    template <class T>
    void Driver<T>::start() {
        if (m_modem == nullptr)
            m_modem = new IridiumSBD(IridiumSerial, -1, RING_INDICATOR_PIN);
        tinyfsm::Fsm<Driver>::start();
    }

    template <class T>
    IridiumSBD* Driver<T>::m_modem = nullptr;

    template <class T>
    std::atomic_bool Driver<T>::Ready::m_did_ring{ false };
};

void SERCOM1_Handler()
{
    SatComm::IridiumSerial.IrqHandler();
}

#endif //SLIDESENTINELROVER_SATCOMMDRIVER_H
