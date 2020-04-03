#pragma once

template<typename MockInterface>
struct MockOutput : public MockInterface {
	static MockInterface* mock_ptr;

	static void set_mock(MockInterface& ptr) {
		mock_ptr = &ptr;
	}

	// as an output
	template<typename E>
	static void dispatch(const E& event) {
		mock_ptr->dispatch(event);
	}

	// as an input
	template<typename E>
	static void react(const E& event) {
		mock_ptr->dispatch(event);
	}

	template<typename T>
	using Alias = MockOutput<MockInterface>;
};

template<typename Mock>
Mock* MockOutput<Mock>::mock_ptr = nullptr;