
#pragma once
#include "tinyfsm.h"
#include "SatComDriver.h"

namespace SatCom {

    // ----------------------------------------------------------------------------
    // 1. Event Declarations
    //

    struct AddPacket {
        bool is_packet;
    }

	// ----------------------------------------------------------------------------
	// 2. State Machine Base Class Declaration
	//

    template<typename Driver>
    struct SatCom : tinyfsm::Fsm<SatCom> {
        virtual void react(TransmitResult const &) { /* Panic */ };
        virtual void react(AddPacket const &) = 0;
    };

    // ----------------------------------------------------------------------------
	// 3. State Declarations
	//
    
    struct Off;

    template<typename Driver>
    struct SatComIdle : SatCom<Driver> {
        virtual void react(TransmitResult const &) { 
            transit<SatComOff<Driver>>();
        }

        virtual void react(AddPacket const &) {
            Driver::dispatch(Transmit{ nullptr, 0 });
        }
    }

    template<typename ParentType>
	struct Off {
		void react(AddPacket const &) override { 
			// Panic
		}
	}

}