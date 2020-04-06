#pragma once

#ifdef UNIT_TEST

#include "unity.h"
#include "tinyfsm.h"
#include "SatCommController.h"
#include "EventBus.h"
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

	// should attempt to recieve once on startup, then nothing else
	TEST_ASSERT_SAME_TYPE(*DebugEventBus::front(), SatComm::StartRecieve);
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
	// should recieve a StartSendRecieve event here w/ the first message
	// we reply with success, but no recieved message
	Controller::dispatch(SatComm::SendSuccess{});
	// should recieve another StartSendRecieve event here w/ the second message
	Controller::dispatch(SatComm::SendSuccess{});
	// we should not recieve a third
	Controller::dispatch(SatComm::SendImmediate{});

	auto event = DebugEventBus::front();
	TEST_ASSERT_SAME_TYPE(*event, SatComm::StartSendRecieve);
	TEST_ASSERT_EQUAL_STRING(event.as<SatComm::StartSendRecieve>().send.bytes.data(), message);
	event = DebugEventBus::front();
	TEST_ASSERT_SAME_TYPE(*event, SatComm::StartSendRecieve);
	TEST_ASSERT_EQUAL_STRING(event.as<SatComm::StartSendRecieve>().send.bytes.data(), message2);
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
	// should recieve a StartSendRecieve event here w/ the first message
	// we reply with success, but no recieved message
	Controller::dispatch(SatComm::SendSuccess{});
	// should recieve another StartSendRecieve event here w/ the second message
	// now there's bad signal! uh oh
	Controller::dispatch(SatComm::SignalLost{});
	// nevermind! we're back
	Controller::dispatch(SatComm::DriverReady{});
	// should recieve another StartSendRecieve event here w/ the second message again
	Controller::dispatch(SatComm::SendSuccess{});
	// we should not recieve a third
	Controller::dispatch(SatComm::SendImmediate{});

	auto event = DebugEventBus::front();
	TEST_ASSERT_SAME_TYPE(*event, SatComm::StartSendRecieve);
	TEST_ASSERT_EQUAL_STRING(event.as<SatComm::StartSendRecieve>().send.bytes.data(), message);
	event = DebugEventBus::front();
	TEST_ASSERT_SAME_TYPE(*event, SatComm::StartSendRecieve);
	TEST_ASSERT_EQUAL_STRING(event.as<SatComm::StartSendRecieve>().send.bytes.data(), message2);
	event = DebugEventBus::front();
	TEST_ASSERT_SAME_TYPE(*event, SatComm::StartSendRecieve);
	TEST_ASSERT_EQUAL_STRING(event.as<SatComm::StartSendRecieve>().send.bytes.data(), message2);
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
	// should recieve a StartSendRecieve event here w/ the first message
	// we reply with success, and a recieved message
	Controller::dispatch(SatComm::SendRecieveSuccess{ recv1, 1 });
	// should recieve a StartRecieve event here
	Controller::dispatch(SatComm::SendRecieveSuccess{ recv2, 0 });
	Controller::dispatch(SatComm::SendImmediate{});

	// first send
	auto event = DebugEventBus::front();
	TEST_ASSERT_SAME_TYPE(*event, SatComm::StartSendRecieve);
	TEST_ASSERT_EQUAL_STRING(event.as<SatComm::StartSendRecieve>().send.bytes.data(), message);
	// got a packet
	event = DebugEventBus::front();
	TEST_ASSERT_SAME_TYPE(*event, SatComm::MessageRecieved);
	TEST_ASSERT_TRUE(event.as<SatComm::MessageRecieved>().recieved.bytes == recv1.bytes);
	// second recieve
	TEST_ASSERT_SAME_TYPE(*DebugEventBus::front(), SatComm::StartRecieve);
	// second packet
	event = DebugEventBus::front();
	TEST_ASSERT_SAME_TYPE(*event, SatComm::MessageRecieved);
	TEST_ASSERT_TRUE(event.as<SatComm::MessageRecieved>().recieved.bytes == recv2.bytes);
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
	// should recieve a StartSendRecieve event here w/ the first message
	// we reply with success, and a recieved message
	Controller::dispatch(SatComm::SendRecieveSuccess{ recv1, 1 });
	// should recieve a StartRecieve event here
	// this time the signal dies!
	Controller::dispatch(SatComm::SignalLost{});
	// but it comes back eventually
	Controller::dispatch(SatComm::DriverReady{});
	// should recieve another StartRecieve event here w/ the second message again
	Controller::dispatch(SatComm::SendRecieveSuccess{ recv2, 0 });
	// we should not recieve a fourth
	Controller::dispatch(SatComm::SendImmediate{});

	// first send
	auto event = DebugEventBus::front();
	TEST_ASSERT_SAME_TYPE(*event, SatComm::StartSendRecieve);
	TEST_ASSERT_EQUAL_STRING(event.as<SatComm::StartSendRecieve>().send.bytes.data(), message);
	// got a packet
	event = DebugEventBus::front();
	TEST_ASSERT_SAME_TYPE(*event, SatComm::MessageRecieved);
	TEST_ASSERT_TRUE(event.as<SatComm::MessageRecieved>().recieved.bytes == recv1.bytes);
	// second recieve
	TEST_ASSERT_SAME_TYPE(*DebugEventBus::front(), SatComm::StartRecieve);
	// third recieve
	TEST_ASSERT_SAME_TYPE(*DebugEventBus::front(), SatComm::StartRecieve);
	// second packet
	event = DebugEventBus::front();
	TEST_ASSERT_SAME_TYPE(*event, SatComm::MessageRecieved);
	TEST_ASSERT_TRUE(event.as<SatComm::MessageRecieved>().recieved.bytes == recv2.bytes);
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
	// the controller will send a StartRecieve event here, which we assume there are no packets for
	Controller::dispatch(SatComm::Success{});
	// send a ring alert
	Controller::dispatch(SatComm::RingAlert{});
	// should recieve a StartRecieve event here
	// we reply with success, and a recieved message
	Controller::dispatch(SatComm::SendRecieveSuccess{ recv1, 1 });
	// should recieve another StartRecieve event here
	Controller::dispatch(SatComm::SendRecieveSuccess{ recv2, 0 });
	// we should not recieve a third
	Controller::dispatch(SatComm::SendImmediate{});

	// first recieve
	TEST_ASSERT_SAME_TYPE(*DebugEventBus::front(), SatComm::StartRecieve);
	// second recieve
	TEST_ASSERT_SAME_TYPE(*DebugEventBus::front(), SatComm::StartRecieve);
	// got a packet
	auto event = DebugEventBus::front();
	TEST_ASSERT_SAME_TYPE(*event, SatComm::MessageRecieved);
	TEST_ASSERT_TRUE(event.as<SatComm::MessageRecieved>().recieved.bytes == recv1.bytes);
	// third recieve
	TEST_ASSERT_SAME_TYPE(*DebugEventBus::front(), SatComm::StartRecieve);
	// second packet
	event = DebugEventBus::front();
	TEST_ASSERT_SAME_TYPE(*event, SatComm::MessageRecieved);
	TEST_ASSERT_TRUE(event.as<SatComm::MessageRecieved>().recieved.bytes == recv2.bytes);
	TEST_ASSERT_TRUE(DebugEventBus::empty());
}

#endif