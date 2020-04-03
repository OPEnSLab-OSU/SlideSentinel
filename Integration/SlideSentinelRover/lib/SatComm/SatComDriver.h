/**
 * State machine representing a simple GNSS upload
 * Not designed to be safe, instead we use this
 * as a representation of the actual hardware state
 * 
 * Eventually I should learn how to draw UML diagrams,
 * so I can draw the state machine here.
 */

#pragma once
#include "tinyfsm.h"

namespace SatCom {
	// ----------------------------------------------------------------------------
	// 1. Event Declarations
	//

	struct Transmit : tinyfsm::Event {
		char* data;
		uint32_t len;
	};

	struct TogglePower : tinyfsm::Event {};

	struct TransmitResult : tinyfsm::Event {
		bool did_succeed;
	}


	// ----------------------------------------------------------------------------
	// 2. State Machine Base Class Declaration
	//
	template<typename ParentType>
	struct SatComDriver : tinyfsm::Fsm<SatComDriver>
	{
		virtual void react(Transmit const &) { /* Panic */ };
		virtual void react(TogglePower const &) = 0;

		virtual void entry(void) { };  /* entry actions in some states */
		virtual void exit(void)  { };  /* exit actions in some states */

		static void reset(void);   /* implemented below */
	};

	// ----------------------------------------------------------------------------
	// 3. State Declarations
	//
	struct Off;

	template<typename ParentType>
	struct Idle : Switch<ParentType>
	{
		void react(Transmit const &) override { 
			// transmit the data
			// TODO: transmission
			// dispatch the result as an event
			ParentType::dispatch(TransmitResult{ true });
		}

		void react(TogglePower const &) override {
			transit<Off>();
		}
	};

	template<typename ParentType>
	struct Off {
		void react(Transmit const &) override { 
			// Panic
		}

		void react(TogglePower const &) override {
			transit<Idle>();
		}
	}

	template<typename ParentType>
	void Switch<ParentType>::reset() {
		// Reset all states (calls constructor on all states in list)
		tinyfsm::StateList<Off, On>::reset();
		start();

		// Alternatively, make counter public above and reset the values
		// here instead of using a copy-constructor with StateList<>:
		//state<On>().counter = 0;
		//state<Off>().counter = 0;
	}
};