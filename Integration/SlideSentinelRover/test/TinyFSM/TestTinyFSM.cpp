#ifdef UNIT_TEST

#include "tinyfsm.h"
#include "unity.h"

struct ToggleSwitch : tinyfsm::Event { };

class Off; // forward declaration

class Switch : public tinyfsm::Fsm<Switch> {
    friend class tinyfsm::Fsm<Switch>;

    /* default reaction for unhandled events */
    void react(tinyfsm::Event const &) { };

    virtual void react(ToggleSwitch const &) { };
    virtual void entry(void) { };  /* entry actions in some states */
    void         exit(void)  { };  /* no exit actions */

    public:
    static void reset(void);   /* implemented below */

    using InitialState = Off;
};

class On
: public Switch
{
  void entry() override { counter++; };
  void react(ToggleSwitch const &) override { transit<Off>(); };
  int counter;

public:
  On() : counter(0) { }
  int getCounter() const { return counter; }
};

class Off
: public Switch
{
  void entry() override { counter++; };
  void react(ToggleSwitch const &) override { transit<On>(); };
  int counter;

public:
  Off() : counter(0) { }
  int getCounter() const { return counter; }
};

void Switch::reset() {
  // Reset all states (calls constructor on all states in list)
  tinyfsm::StateList<Off, On>::reset();

  // Alternatively, make counter public above and reset the values
  // here instead of using a copy-constructor with StateList<>:
  //state<On>().counter = 0;
  //state<Off>().counter = 0;
}

void TestInitial() {
    Switch::reset();
    Switch::start();
    TEST_ASSERT_TRUE(Switch::is_in_state<Off>());
    TEST_ASSERT_EQUAL_INT(Switch::state<Off>().getCounter(), 1);
}

void TestReset() {
    Switch::start();
    Switch::dispatch(ToggleSwitch());
    Switch::reset();
    Switch::start();
    TEST_ASSERT_TRUE(Switch::is_in_state<Off>());
    TEST_ASSERT_FALSE(Switch::is_in_state<On>());
    TEST_ASSERT_EQUAL_INT(Switch::state<Off>().getCounter(), 1);
    TEST_ASSERT_EQUAL_INT(Switch::state<On>().getCounter(), 0);
}

void TestToggle() {
    Switch::reset();
    Switch::start();
    Switch::dispatch(ToggleSwitch{});
    TEST_ASSERT_FALSE(Switch::is_in_state<Off>());
    TEST_ASSERT_TRUE(Switch::is_in_state<On>());
    TEST_ASSERT_EQUAL_INT(Switch::state<Off>().getCounter(),1);
    TEST_ASSERT_EQUAL_INT(Switch::state<On>().getCounter(), 1);
    Switch::dispatch(ToggleSwitch{});
    TEST_ASSERT_TRUE(Switch::is_in_state<Off>());
    TEST_ASSERT_FALSE(Switch::is_in_state<On>());
    TEST_ASSERT_EQUAL_INT(Switch::state<Off>().getCounter(), 2);
    TEST_ASSERT_EQUAL_INT(Switch::state<On>().getCounter(), 1);
}

void TestInvalid() {
    Switch::reset();
    Switch::start();
    Switch::dispatch(tinyfsm::Event());
    TEST_ASSERT_TRUE(Switch::is_in_state<Off>());
    TEST_ASSERT_FALSE(Switch::is_in_state<On>());
    TEST_ASSERT_EQUAL_INT(Switch::state<Off>().getCounter(), 1);
    TEST_ASSERT_EQUAL_INT(Switch::state<On>().getCounter(), 0);
}

void process() {
    UNITY_BEGIN();
    // Register your tests here
    // TinyFSM
    RUN_TEST(TestInitial);
    RUN_TEST(TestReset);
    RUN_TEST(TestToggle);
    RUN_TEST(TestInvalid);
    UNITY_END();
}

#include "../test_common/TestMain.h"

#endif