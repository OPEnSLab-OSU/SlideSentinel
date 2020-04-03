#pragma once

#include <array>
#include "tinyfsm.h"

namespace SatComm {
	// Settings and types
	constexpr size_t PacketSizeBytes = 50;
	struct Packet {
		std::array<char, PacketSizeBytes> bytes;
		size_t length;
	};

	// ----------------------------------------------------------------------------
	// Event declarations
	//

	// Inbound
	struct StartSendRecieve : tinyfsm::Event {
		StartSendRecieve(const Packet& send_in)
			: send(send_in) {}

		const Packet& send;
	};
	struct StartRecieve : tinyfsm::Event { };

	// Outbound
	struct SignalLost : tinyfsm::Event { };
	struct SendSuccess : tinyfsm::Event { };
	struct SendRecieveSuccess : tinyfsm::Event {
		SendRecieveSuccess(const Packet& recv_in, const size_t pending_packets_in)
			: recieved(recv_in)
			, pending_packets(pending_packets_in) {}

		const Packet& recieved;
		const size_t pending_packets;
	};
	struct Success : tinyfsm::Event { };

	struct RingAlert : tinyfsm::Event { };
	struct DriverReady : tinyfsm::Event { };
}