#pragma once

#ifdef UNIT_TEST

#include "unity.h"
#include "tinyfsm.h"
#include "SatCommController.h"
#include "EventQueue.h"
#include "Mockable.h"
#include "DebugEventBus.h"
#include "UnitySameType.h"

void TestNoSignal() {
	using Controller = SatComm::Controller <DebugEventBus>;

	// test that if there is no signal, there is no attempt to transmit

	DebugEventBus::reset();
	Controller::reset();
	Controller::start();
	Controller::dispatch(SatComm::SignalLost{});
	Controller::dispatch(SatComm::SendImmediate{});
	Controller::dispatch(SatComm::RingAlert{});

	TEST_ASSERT_TRUE(DebugEventBus::empty());
}

void TestNoPackets() {
	using Controller = SatComm::Controller <DebugEventBus>;

	// test that if there is no packets, there is no attempt to transmit
	DebugEventBus::reset();
	Controller::reset();
	Controller::start();
	Controller::dispatch(SatComm::SignalLost{});
	Controller::dispatch(SatComm::DriverReady{});
	Controller::dispatch(SatComm::Success{});
	Controller::dispatch(SatComm::SendImmediate{});

	// should attempt to receive once on startup, then nothing else
	TEST_ASSERT_SAME_TYPE(*DebugEventBus::front(), SatComm::StartReceive);
	TEST_ASSERT_TRUE(DebugEventBus::empty());
}

void TestTransmitNoRx() {
	using Controller = SatComm::Controller <DebugEventBus>;

	// test that if there is no signal, there is no attempt to transmit

	DebugEventBus::reset();
	Controller::reset();
	Controller::start();

	// queue the message
	const char message[] = "Hello message!";
	Controller::dispatch(SatComm::QueueMessage{
		message,
		sizeof(message)
	});

	// and another one
	const char message2[] = "Another message!";
	Controller::dispatch(SatComm::QueueMessage{
		message2,
		sizeof(message2)
	});

	// start the controller
	Controller::dispatch(SatComm::SignalLost{});
	Controller::dispatch(SatComm::DriverReady{});
	// should receive a StartSendReceive event here w/ the first message
	// we reply with success, but no received message
	Controller::dispatch(SatComm::SendSuccess{});
	// should receive another StartSendReceive event here w/ the second message
	Controller::dispatch(SatComm::SendSuccess{});
	// we should not receive a third
	Controller::dispatch(SatComm::SendImmediate{});

	auto event = DebugEventBus::front();
	TEST_ASSERT_SAME_TYPE(*event, SatComm::StartSendReceive);
	TEST_ASSERT_EQUAL_STRING(event.as<SatComm::StartSendReceive>().send.bytes.data(), message);
	event = DebugEventBus::front();
	TEST_ASSERT_SAME_TYPE(*event, SatComm::StartSendReceive);
	TEST_ASSERT_EQUAL_STRING(event.as<SatComm::StartSendReceive>().send.bytes.data(), message2);
	TEST_ASSERT_TRUE(DebugEventBus::empty());
}

void TestTransmitNoRxBadSignal() {
	using Controller = SatComm::Controller <DebugEventBus>;

	// test that if there is no signal, there is no attempt to transmit

	DebugEventBus::reset();
	Controller::reset();
	Controller::start();

	// queue the message
	const char message[] = "Hello message!";
	Controller::dispatch(SatComm::QueueMessage{
		message,
		sizeof(message)
		});

	// and another one
	const char message2[] = "Another message!";
	Controller::dispatch(SatComm::QueueMessage{
		message2,
		sizeof(message2)
		});

	// start the controller
	Controller::dispatch(SatComm::SignalLost{});
	Controller::dispatch(SatComm::DriverReady{});
	// should receive a StartSendReceive event here w/ the first message
	// we reply with success, but no received message
	Controller::dispatch(SatComm::SendSuccess{});
	// should receive another StartSendReceive event here w/ the second message
	// now there's bad signal! uh oh
	Controller::dispatch(SatComm::SignalLost{});
	// nevermind! we're back
	Controller::dispatch(SatComm::DriverReady{});
	// should receive another StartSendReceive event here w/ the second message again
	Controller::dispatch(SatComm::SendSuccess{});
	// we should not receive a third
	Controller::dispatch(SatComm::SendImmediate{});

	auto event = DebugEventBus::front();
	TEST_ASSERT_SAME_TYPE(*event, SatComm::StartSendReceive);
	TEST_ASSERT_EQUAL_STRING(event.as<SatComm::StartSendReceive>().send.bytes.data(), message);
	event = DebugEventBus::front();
	TEST_ASSERT_SAME_TYPE(*event, SatComm::StartSendReceive);
	TEST_ASSERT_EQUAL_STRING(event.as<SatComm::StartSendReceive>().send.bytes.data(), message2);
	event = DebugEventBus::front();
	TEST_ASSERT_SAME_TYPE(*event, SatComm::StartSendReceive);
	TEST_ASSERT_EQUAL_STRING(event.as<SatComm::StartSendReceive>().send.bytes.data(), message2);
	TEST_ASSERT_TRUE(DebugEventBus::empty());
}

void TestTransmitRx() {

	using Controller = SatComm::Controller <DebugEventBus>;

	// initialize the packets to be sent back
	SatComm::Packet recv1{ { 'I', '\'', 'm', ' ', 'h', 'e', 'r', 'e', '!', '\0' }, 10 };
	SatComm::Packet recv2{ { 'A', 'l', 's', 'o', ' ', 'h', 'e', 'r', 'e', '!', '\0' }, 11 };

	DebugEventBus::reset();
	Controller::reset();
	Controller::start();

	// queue the message
	const char message[] = "Hello message!";
	Controller::dispatch(SatComm::QueueMessage{
		message,
		sizeof(message)
		});

	// start the controller
	Controller::dispatch(SatComm::SignalLost{});
	Controller::dispatch(SatComm::DriverReady{});
	// should receive a StartSendReceive event here w/ the first message
	// we reply with success, and a received message
	Controller::dispatch(SatComm::SendReceiveSuccess{ recv1, 1 });
	// should receive a StartReceive event here
	Controller::dispatch(SatComm::SendReceiveSuccess{ recv2, 0 });
	Controller::dispatch(SatComm::SendImmediate{});

	// first send
	auto event = DebugEventBus::front();
	TEST_ASSERT_SAME_TYPE(*event, SatComm::StartSendReceive);
	TEST_ASSERT_EQUAL_STRING(event.as<SatComm::StartSendReceive>().send.bytes.data(), message);
	// got a packet
	event = DebugEventBus::front();
	TEST_ASSERT_SAME_TYPE(*event, SatComm::MessageReceived);
	TEST_ASSERT_TRUE(event.as<SatComm::MessageReceived>().received.bytes == recv1.bytes);
	// second receive
	TEST_ASSERT_SAME_TYPE(*DebugEventBus::front(), SatComm::StartReceive);
	// second packet
	event = DebugEventBus::front();
	TEST_ASSERT_SAME_TYPE(*event, SatComm::MessageReceived);
	TEST_ASSERT_TRUE(event.as<SatComm::MessageReceived>().received.bytes == recv2.bytes);
	TEST_ASSERT_TRUE(DebugEventBus::empty());
}

void TestTransmitRxBadSignal() {
	using Controller = SatComm::Controller <DebugEventBus>;

	// initialize the packets to be sent back
	SatComm::Packet recv1{ { 'I', '\'', 'm', ' ', 'h', 'e', 'r', 'e', '!', '\0' }, 10 };
	SatComm::Packet recv2{ { 'A', 'l', 's', 'o', ' ', 'h', 'e', 'r', 'e', '!', '\0' }, 11 };

	DebugEventBus::reset();
	Controller::reset();
	Controller::start();

	// queue the message
	const char message[] = "Hello message!";
	Controller::dispatch(SatComm::QueueMessage{
		message,
		sizeof(message)
		});

	// start the controller
	Controller::dispatch(SatComm::SignalLost{});
	Controller::dispatch(SatComm::DriverReady{});
	Controller::dispatch(SatComm::SendImmediate{});
	// should receive a StartSendReceive event here w/ the first message
	// we reply with success, and a received message
	Controller::dispatch(SatComm::SendReceiveSuccess{ recv1, 1 });
	// should receive a StartReceive event here
	// this time the signal dies!
	Controller::dispatch(SatComm::SignalLost{});
	// but it comes back eventually
	Controller::dispatch(SatComm::DriverReady{});
	// should receive another StartReceive event here w/ the second message again
	Controller::dispatch(SatComm::SendReceiveSuccess{ recv2, 0 });
	// we should not receive a fourth
	Controller::dispatch(SatComm::SendImmediate{});

	// first send
	auto event = DebugEventBus::front();
	TEST_ASSERT_SAME_TYPE(*event, SatComm::StartSendReceive);
	TEST_ASSERT_EQUAL_STRING(event.as<SatComm::StartSendReceive>().send.bytes.data(), message);
	// got a packet
	event = DebugEventBus::front();
	TEST_ASSERT_SAME_TYPE(*event, SatComm::MessageReceived);
	TEST_ASSERT_TRUE(event.as<SatComm::MessageReceived>().received.bytes == recv1.bytes);
	// second receive
	TEST_ASSERT_SAME_TYPE(*DebugEventBus::front(), SatComm::StartReceive);
	// third receive
	TEST_ASSERT_SAME_TYPE(*DebugEventBus::front(), SatComm::StartReceive);
	// second packet
	event = DebugEventBus::front();
	TEST_ASSERT_SAME_TYPE(*event, SatComm::MessageReceived);
	TEST_ASSERT_TRUE(event.as<SatComm::MessageReceived>().received.bytes == recv2.bytes);
	TEST_ASSERT_TRUE(DebugEventBus::empty());
}

void TestRingAlert() {
	using Controller = SatComm::Controller <DebugEventBus>;

	// initialize the packets to be sent back
	SatComm::Packet recv1{ { 'I', '\'', 'm', ' ', 'h', 'e', 'r', 'e', '!', '\0' }, 10 };
	SatComm::Packet recv2{ { 'A', 'l', 's', 'o', ' ', 'h', 'e', 'r', 'e', '!', '\0' }, 11 };

	DebugEventBus::reset();
	Controller::reset();
	Controller::start();

	// start the controller
	Controller::dispatch(SatComm::SignalLost{});
	Controller::dispatch(SatComm::DriverReady{});
	// the controller will send a StartReceive event here, which we assume there are no packets for
	Controller::dispatch(SatComm::Success{});
	// send a ring alert
	Controller::dispatch(SatComm::RingAlert{});
	// should receive a StartReceive event here
	// we reply with success, and a received message
	Controller::dispatch(SatComm::SendReceiveSuccess{ recv1, 1 });
	// should receive another StartReceive event here
	Controller::dispatch(SatComm::SendReceiveSuccess{ recv2, 0 });
	// we should not receive a third
	Controller::dispatch(SatComm::SendImmediate{});

	// first receive
	TEST_ASSERT_SAME_TYPE(*DebugEventBus::front(), SatComm::StartReceive);
	// second receive
	TEST_ASSERT_SAME_TYPE(*DebugEventBus::front(), SatComm::StartReceive);
	// got a packet
	auto event = DebugEventBus::front();
	TEST_ASSERT_SAME_TYPE(*event, SatComm::MessageReceived);
	TEST_ASSERT_TRUE(event.as<SatComm::MessageReceived>().received.bytes == recv1.bytes);
	// third receive
	TEST_ASSERT_SAME_TYPE(*DebugEventBus::front(), SatComm::StartReceive);
	// second packet
	event = DebugEventBus::front();
	TEST_ASSERT_SAME_TYPE(*event, SatComm::MessageReceived);
	TEST_ASSERT_TRUE(event.as<SatComm::MessageReceived>().received.bytes == recv2.bytes);
	TEST_ASSERT_TRUE(DebugEventBus::empty());
}

void TestTransmitMultiPacket() {
    using Controller = SatComm::Controller <DebugEventBus>;

    DebugEventBus::reset();
    Controller::reset();
    Controller::start();

    // queue the message
    const char message[] = "Hello message! This is a really long message, and will probably get split into several packets."
            " Given that I am a very wordy person, I guess this isn't too surprising, however in the future I would"
            " hope that messages would be compressed to prevent issues such as this one from occurring.";
    Controller::dispatch(SatComm::QueueMessage{
            message,
            sizeof(message)
    });

    // start the controller
    Controller::dispatch(SatComm::SignalLost{});
    Controller::dispatch(SatComm::DriverReady{});
    // should receive a StartSendReceive event here w/ the first message
    // should generate a number of packets, though how many is up to implementation
    // we just need to test that our string makes it
    size_t idx = 0;
    while (!DebugEventBus::empty()) {
        // check the message
        auto event = DebugEventBus::front();
        TEST_ASSERT_SAME_TYPE(*event, SatComm::StartSendReceive);
        // and check the contents
        const auto& packet = event.as<SatComm::StartSendReceive>().send;
        TEST_ASSERT_LESS_OR_EQUAL(sizeof(message), idx + packet.length);
        TEST_ASSERT_EQUAL_CHAR_ARRAY(packet.bytes.data(), &message[idx], packet.length);
        // move the index forward!
        idx += packet.length;
        // reply with success
        Controller::dispatch(SatComm::SendSuccess{});
    }
    // we should not receive any more
    Controller::dispatch(SatComm::SendImmediate{});
    TEST_ASSERT_TRUE(DebugEventBus::empty());
}

#endif