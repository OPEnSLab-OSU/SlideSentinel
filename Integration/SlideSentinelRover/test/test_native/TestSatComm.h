#pragma once

#if !defined(ARDUINO) && defined(UNIT_TEST)

#include "unity.h"
#include "tinyfsm.h"
#include "SatCommController.h"
#include "EventBus.h"
#include "Mockable.h"
#include <ArduinoFake.h>

// Events MUST be passed by value, as they are copied in FakeIt
class MockInterface {
public:
	virtual void dispatch(const SatComm::MessageRecieved) = 0;
	virtual void dispatch(const SatComm::StartSendRecieve) = 0;
	virtual void dispatch(const SatComm::StartRecieve) = 0;
	virtual void dispatch(const tinyfsm::Event) = 0;
};

using MockType = MockOutput<MockInterface>;

static void stub_mock(fakeit::Mock<MockInterface>& mymock) {
	using namespace fakeit;

	Fake(OverloadedMethod(mymock, dispatch, void(const SatComm::StartRecieve)));
	Fake(OverloadedMethod(mymock, dispatch, void(const SatComm::StartSendRecieve)));
	Fake(OverloadedMethod(mymock, dispatch, void(const SatComm::MessageRecieved)));
	Fake(OverloadedMethod(mymock, dispatch, void(const tinyfsm::Event)));
}

void TestNoSignal() {
	fakeit::Mock<MockInterface> mymock;
	stub_mock(mymock);
	MockType::set_mock(mymock.get());

	using Controller = SatComm::Controller <MockType>;

	// test that if there is no signal, there is no attempt to transmit

	Controller::reset();
	Controller::start();
	Controller::dispatch(SatComm::SignalLost{});
	Controller::dispatch(SatComm::SendImmediate{});
	Controller::dispatch(SatComm::RingAlert{});

	using namespace fakeit;
	VerifyNoOtherInvocations(mymock);
}

void TestNoPackets() {
	fakeit::Mock<MockInterface> mymock;
	stub_mock(mymock);
	MockType::set_mock(mymock.get());

	using Controller = SatComm::Controller <MockType>;

	// test that if there is no packets, there is no attempt to transmit
	Controller::reset();
	Controller::start();
	Controller::dispatch(SatComm::SignalLost{});
	Controller::dispatch(SatComm::DriverReady{});
	Controller::dispatch(SatComm::Success{});
	Controller::dispatch(SatComm::SendImmediate{});

	using namespace fakeit;

	// should attempt to recieve once on startup, then nothing else
	Verify(OverloadedMethod(mymock, dispatch, void(const SatComm::StartRecieve))).Once();

	VerifyNoOtherInvocations(mymock);
}

void TestTransmitNoRx() {
	fakeit::Mock<MockInterface> mymock;
	stub_mock(mymock);
	MockType::set_mock(mymock.get());

	using Controller = SatComm::Controller <MockType>;

	// test that if there is no signal, there is no attempt to transmit

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

	using namespace fakeit;

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

	
	Verify(	OverloadedMethod(mymock, dispatch, void(const SatComm::StartSendRecieve))
				.Matching([message](const SatComm::StartSendRecieve& e) {
					return strcmp(e.send.bytes.data(), message) == 0;
				})
			+ OverloadedMethod(mymock, dispatch, void(const SatComm::StartSendRecieve))
				.Matching([message2](const SatComm::StartSendRecieve& e) {
					return strcmp(e.send.bytes.data(), message2) == 0;
				}))
		.Once();

	VerifyNoOtherInvocations(mymock);
}

void TestTransmitNoRxBadSignal() {
	fakeit::Mock<MockInterface> mymock;
	stub_mock(mymock);
	MockType::set_mock(mymock.get());

	using Controller = SatComm::Controller <MockType>;

	// test that if there is no signal, there is no attempt to transmit

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

	using namespace fakeit;

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

	Verify(
		OverloadedMethod(mymock, dispatch, void(const SatComm::StartSendRecieve))
			.Matching([message](const SatComm::StartSendRecieve& e) {
				return strcmp(e.send.bytes.data(), message) == 0;
			})
		+ OverloadedMethod(mymock, dispatch, void(const SatComm::StartSendRecieve))
			.Matching([message2](const SatComm::StartSendRecieve& e) {
				return strcmp(e.send.bytes.data(), message2) == 0;
			}) * 2
	).Once();

	VerifyNoOtherInvocations(mymock);
}

void TestTransmitRx() {
	fakeit::Mock<MockInterface> mymock;
	stub_mock(mymock);
	MockType::set_mock(mymock.get());

	using Controller = SatComm::Controller <MockType>;

	// initialize the packets to be sent back
	SatComm::Packet recv1{ { 'I', '\'', 'm', ' ', 'h', 'e', 'r', 'e', '!', '\0' }, 10 };
	SatComm::Packet recv2{ { 'A', 'l', 's', 'o', ' ', 'h', 'e', 'r', 'e', '!', '\0' }, 11 };

	Controller::reset();
	Controller::start();

	// queue the message
	const char message[] = "Hello message!";
	Controller::dispatch(SatComm::QueueMessage{
		message,
		sizeof(message)
		});

	using namespace fakeit;

	// start the controller
	Controller::dispatch(SatComm::SignalLost{});
	Controller::dispatch(SatComm::DriverReady{});
	// should recieve a StartSendRecieve event here w/ the first message
	// we reply with success, and a recieved message
	Controller::dispatch(SatComm::SendRecieveSuccess{ recv1, 1 });
	// should recieve a StartRecieve event here
	Controller::dispatch(SatComm::SendRecieveSuccess{ recv2, 0 });
	Controller::dispatch(SatComm::SendImmediate{});

	// Driver
	Verify(
		// first send
		OverloadedMethod(mymock, dispatch, void(const SatComm::StartSendRecieve))
			.Matching([message](const SatComm::StartSendRecieve& e) {
				return strcmp(e.send.bytes.data(), message) == 0;
			}),
		// second recieve
		OverloadedMethod(mymock, dispatch, void(const SatComm::StartRecieve))
	).Once();

	// Controller
	Verify(
		// first recv
		OverloadedMethod(mymock, dispatch, void(const SatComm::MessageRecieved))
			.Matching([recv1](const SatComm::MessageRecieved& e) {
				return e.recieved.bytes == recv1.bytes;
			}),
		// second recv
		OverloadedMethod(mymock, dispatch, void(const SatComm::MessageRecieved))
			.Matching([recv2](const SatComm::MessageRecieved& e) {
				return e.recieved.bytes == recv2.bytes;
			})
	).Once();

	VerifyNoOtherInvocations(mymock);
}

void TestTransmitRxBadSignal() {
	fakeit::Mock<MockInterface> mymock;
	stub_mock(mymock);
	MockType::set_mock(mymock.get());

	using Controller = SatComm::Controller <MockType>;

	// initialize the packets to be sent back
	SatComm::Packet recv1{ { 'I', '\'', 'm', ' ', 'h', 'e', 'r', 'e', '!', '\0' }, 10 };
	SatComm::Packet recv2{ { 'A', 'l', 's', 'o', ' ', 'h', 'e', 'r', 'e', '!', '\0' }, 11 };

	Controller::reset();
	Controller::start();

	// queue the message
	const char message[] = "Hello message!";
	Controller::dispatch(SatComm::QueueMessage{
		message,
		sizeof(message)
		});

	using namespace fakeit;

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

	// Driver
	Verify(
		// first send
		OverloadedMethod(mymock, dispatch, void(const SatComm::StartSendRecieve))
			.Matching([message](const SatComm::StartSendRecieve& e) {
				return strcmp(e.send.bytes.data(), message) == 0;
			}),
		// first and second recv
		OverloadedMethod(mymock, dispatch, void(const SatComm::StartRecieve)),
		OverloadedMethod(mymock, dispatch, void(const SatComm::StartRecieve))
	).Once();

	// Controller
	Verify(
		// first recv data
		OverloadedMethod(mymock, dispatch, void(const SatComm::MessageRecieved))
			.Matching([recv1](const SatComm::MessageRecieved& e) {
				return e.recieved.bytes == recv1.bytes;
			}),
		// second recv data
		OverloadedMethod(mymock, dispatch, void(const SatComm::MessageRecieved))
			.Matching([recv2](const SatComm::MessageRecieved& e) {
				return e.recieved.bytes == recv2.bytes;
			})
	).Once();

	VerifyNoOtherInvocations(mymock);
}

void TestRingAlert() {
	fakeit::Mock<MockInterface> mymock;
	stub_mock(mymock);
	MockType::set_mock(mymock.get());

	using Controller = SatComm::Controller <MockType>;

	// initialize the packets to be sent back
	SatComm::Packet recv1{ { 'I', '\'', 'm', ' ', 'h', 'e', 'r', 'e', '!', '\0' }, 10 };
	SatComm::Packet recv2{ { 'A', 'l', 's', 'o', ' ', 'h', 'e', 'r', 'e', '!', '\0' }, 11 };

	Controller::reset();
	Controller::start();

	using namespace fakeit;

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

	// Driver
	Verify(
		// first and second recv
		OverloadedMethod(mymock, dispatch, void(const SatComm::StartRecieve)),
		OverloadedMethod(mymock, dispatch, void(const SatComm::StartRecieve)),
		OverloadedMethod(mymock, dispatch, void(const SatComm::StartRecieve))
	).Once();

	// Controller
	Verify(
		// first recv data
		OverloadedMethod(mymock, dispatch, void(const SatComm::MessageRecieved))
			.Matching([recv1](const SatComm::MessageRecieved& e) {
				return e.recieved.bytes == recv1.bytes;
			}),
		// second recv data
		OverloadedMethod(mymock, dispatch, void(const SatComm::MessageRecieved))
			.Matching([recv2](const SatComm::MessageRecieved& e) {
				return e.recieved.bytes == recv2.bytes;
			})
	).Once();
	
	VerifyNoOtherInvocations(mymock);
}

#endif