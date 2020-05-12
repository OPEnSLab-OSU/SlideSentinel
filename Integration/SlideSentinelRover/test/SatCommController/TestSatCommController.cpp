#ifdef UNIT_TEST

#include "unity.h"
#include "tinyfsm.h"
#include "SatCommController.h"
#include "../test_common/DebugEventBus.h"

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
	TEST_ASSERT_FALSE(Controller::connected());
	TEST_ASSERT_EQUAL(0, Controller::available());
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


}

void TestSignalCycle() {
    using Controller = SatComm::Controller <DebugEventBus>;

    // test that if there is no packets, there is only one attempt to recieve
    DebugEventBus::reset();
    Controller::reset();
    Controller::start();
    Controller::dispatch(SatComm::SignalLost{});
    Controller::dispatch(SatComm::DriverReady{});
    Controller::dispatch(SatComm::Success{});
    Controller::dispatch(SatComm::SignalLost{});
    TEST_ASSERT_FALSE(Controller::connected());
    Controller::dispatch(SatComm::DriverReady{});

    // should attempt to receive once on startup, then nothing else
    DEBUG_BUS_POP_EVENT(DebugEventBus, SatComm::StartReceive);
    TEST_ASSERT_TRUE(DebugEventBus::empty());
    TEST_ASSERT_TRUE(Controller::connected());
    TEST_ASSERT_EQUAL(0, Controller::available());
}

void TestSignalCycleRingAlert() {
    using Controller = SatComm::Controller <DebugEventBus>;

    // test that if a ring alert is received during signallost the device
    // will attempt to receive when signal is found
    DebugEventBus::reset();
    Controller::reset();
    Controller::start();
    Controller::dispatch(SatComm::SignalLost{});
    Controller::dispatch(SatComm::DriverReady{});
    Controller::dispatch(SatComm::Success{});
    Controller::dispatch(SatComm::SignalLost{});
    TEST_ASSERT_FALSE(Controller::connected());

    Controller::dispatch(SatComm::RingAlert{});
    Controller::dispatch(SatComm::DriverReady{});
    Controller::dispatch(SatComm::Success{});

    // should attempt to receive twice: once on startup, then once again when signal is found
    DEBUG_BUS_POP_EVENT(DebugEventBus, SatComm::StartReceive);
    DEBUG_BUS_POP_EVENT(DebugEventBus, SatComm::StartReceive);
    TEST_ASSERT_TRUE(DebugEventBus::empty());
    TEST_ASSERT_TRUE(Controller::connected());
    TEST_ASSERT_EQUAL(0, Controller::available());
}

void TestTransmitNoRx() {
	using Controller = SatComm::Controller <DebugEventBus>;

	// test that if there is no signal, there is no attempt to transmit

	DebugEventBus::reset();
	Controller::reset();
	Controller::start();

	// queue the message
	const char message[] = "Hello message!";
	Controller::queue(message, sizeof(message));

	// and another one
	const char message2[] = "Another message!";
	Controller::dispatch(SatComm::QueueMessage{
		message2,
		sizeof(message2)
	});

	TEST_ASSERT_EQUAL(2, Controller::send_pending());

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

    auto event = DEBUG_BUS_POP_EVENT(DebugEventBus, SatComm::StartSendReceive);
	TEST_ASSERT_EQUAL_STRING(message, event.send.bytes.data());

    auto event2 = DEBUG_BUS_POP_EVENT(DebugEventBus, SatComm::StartSendReceive);
	TEST_ASSERT_EQUAL_STRING(message2, event2.send.bytes.data());
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

    TEST_ASSERT_EQUAL(2, Controller::send_pending());

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

    auto event = DEBUG_BUS_POP_EVENT(DebugEventBus, SatComm::StartSendReceive);
	TEST_ASSERT_EQUAL_STRING(message, event.send.bytes.data());

    auto event2 = DEBUG_BUS_POP_EVENT(DebugEventBus, SatComm::StartSendReceive);
	TEST_ASSERT_EQUAL_STRING(message2, event2.send.bytes.data());

    auto event3 = DEBUG_BUS_POP_EVENT(DebugEventBus, SatComm::StartSendReceive);
	TEST_ASSERT_EQUAL_STRING(message2, event3.send.bytes.data());
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

    TEST_ASSERT_EQUAL(1, Controller::send_pending());

	// start the controller
	Controller::dispatch(SatComm::SignalLost{});
	Controller::dispatch(SatComm::DriverReady{});
	// should receive a StartSendReceive event here w/ the first message
	// we reply with success, and a received message
	Controller::dispatch(SatComm::SendReceiveSuccess{ recv1, 1 });
	TEST_ASSERT_EQUAL(1, Controller::available());
	// should receive a StartReceive event here
	Controller::dispatch(SatComm::SendReceiveSuccess{ recv2, 0 });
	TEST_ASSERT_EQUAL(2, Controller::available());
	Controller::dispatch(SatComm::SendImmediate{});

	// first send
    auto event = DEBUG_BUS_POP_EVENT(DebugEventBus, SatComm::StartSendReceive);
	TEST_ASSERT_EQUAL_STRING(message, event.send.bytes.data());
	// check the packet
	SatComm::Packet pkt;
	Controller::receive(pkt);
	TEST_ASSERT_TRUE(pkt.bytes == recv1.bytes);
	TEST_ASSERT_EQUAL(recv1.length, pkt.length);
	// second receive
    DEBUG_BUS_POP_EVENT(DebugEventBus, SatComm::StartReceive);
	// second packet
    Controller::receive(pkt);
    TEST_ASSERT_TRUE(pkt.bytes == recv2.bytes);
    TEST_ASSERT_EQUAL(recv2.length, pkt.length);
    TEST_ASSERT_EQUAL(0, Controller::available());
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

    TEST_ASSERT_EQUAL(1, Controller::send_pending());

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
    auto event = DEBUG_BUS_POP_EVENT(DebugEventBus, SatComm::StartSendReceive);
	TEST_ASSERT_EQUAL_STRING(message, event.send.bytes.data());
	// got a packet
    SatComm::Packet pkt;
    Controller::receive(pkt);
    TEST_ASSERT_TRUE(pkt.bytes == recv1.bytes);
    TEST_ASSERT_EQUAL(recv1.length, pkt.length);
    // second receive
    DEBUG_BUS_POP_EVENT(DebugEventBus, SatComm::StartReceive);
	// third receive
    DEBUG_BUS_POP_EVENT(DebugEventBus, SatComm::StartReceive);
	// second packet
    Controller::receive(pkt);
    TEST_ASSERT_TRUE(pkt.bytes == recv2.bytes);
    TEST_ASSERT_EQUAL(recv2.length, pkt.length);
    TEST_ASSERT_EQUAL(0, Controller::available());
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
    DEBUG_BUS_POP_EVENT(DebugEventBus, SatComm::StartReceive);
	// second receive
    DEBUG_BUS_POP_EVENT(DebugEventBus, SatComm::StartReceive);
	// got a packet
    SatComm::Packet pkt;
    Controller::receive(pkt);
    TEST_ASSERT_TRUE(pkt.bytes == recv1.bytes);
    TEST_ASSERT_EQUAL(recv1.length, pkt.length);
	// third receive
    DEBUG_BUS_POP_EVENT(DebugEventBus, SatComm::StartReceive);
	// second packet
    Controller::receive(pkt);
    TEST_ASSERT_TRUE(pkt.bytes == recv2.bytes);
    TEST_ASSERT_EQUAL(recv2.length, pkt.length);
    TEST_ASSERT_EQUAL(0, Controller::available());
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
        auto event = DEBUG_BUS_POP_EVENT(DebugEventBus, SatComm::StartSendReceive);
        // and check the contents
        const auto& packet = event.send;
        TEST_ASSERT_LESS_OR_EQUAL(sizeof(message), idx + packet.length);
        TEST_ASSERT_EQUAL_CHAR_ARRAY(&message[idx], packet.bytes.data(), packet.length);
        // move the index forward!
        idx += packet.length;
        // reply with success
        Controller::dispatch(SatComm::SendSuccess{});
    }
    // we should not receive any more
    Controller::dispatch(SatComm::SendImmediate{});
    TEST_ASSERT_TRUE(DebugEventBus::empty());
}

void process() {
	UNITY_BEGIN();
	RUN_TEST(TestNoSignal);
    RUN_TEST(TestNoPackets);
    RUN_TEST(TestSignalCycle);
    RUN_TEST(TestSignalCycleRingAlert);
    RUN_TEST(TestTransmitNoRx);
    RUN_TEST(TestTransmitNoRxBadSignal);
    RUN_TEST(TestTransmitRx);
    RUN_TEST(TestTransmitRxBadSignal);
    RUN_TEST(TestRingAlert);
    RUN_TEST(TestTransmitMultiPacket);
    UNITY_END();
}

#include "../test_common/TestMain.h"

#endif