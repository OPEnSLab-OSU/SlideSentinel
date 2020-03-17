#include "tinyfsm.h"
#include "unity.h"

#ifdef UNIT_TEST

struct Toggle : tinyfsm::Event { };

class Switch : public tinyfsm::Fsm<Switch> {
    friend class tinyfsm::Fsm<Switch>;

    /* default reaction for unhandled events */
    void react(tinyfsm::Event const &) { };

    virtual void react(Toggle const &) { };
    virtual void entry(void) { };  /* entry actions in some states */
    void         exit(void)  { };  /* no exit actions */

    public:
    static void reset(void);   /* implemented below */

};

class Off; // forward declaration

class On
: public Switch
{
  void entry() override { counter++; };
  void react(Toggle const &) override { transit<Off>(); };
  int counter;

public:
  On() : counter(0) { }
  int getCounter() const { return counter; }
};

class Off
: public Switch
{
  void entry() override { counter++; };
  void react(Toggle const &) override { transit<On>(); };
  int counter;

public:
  Off() : counter(0) { }
  int getCounter() const { return counter; }
};

void Switch::reset() {
  // Reset all states (calls constructor on all states in list)
  tinyfsm::StateList<Off, On>::reset();
  start();

  // Alternatively, make counter public above and reset the values
  // here instead of using a copy-constructor with StateList<>:
  //state<On>().counter = 0;
  //state<Off>().counter = 0;
}

FSM_INITIAL_STATE(Switch, Off)

void TestInitial() {
    Switch::reset();
    TEST_ASSERT_TRUE(Switch::is_in_state<Off>());
    TEST_ASSERT_EQUAL_INT(1, Switch::state<Off>().getCounter());
}

void TestReset() {
    Switch::start();
    Switch::dispatch(Toggle());
    Switch::reset();
    TEST_ASSERT_TRUE(Switch::is_in_state<Off>());
    TEST_ASSERT_FALSE(Switch::is_in_state<On>());
    TEST_ASSERT_EQUAL_INT(1, Switch::state<Off>().getCounter());
    TEST_ASSERT_EQUAL_INT(0, Switch::state<On>().getCounter());
}

void TestToggle() {
    Switch::reset();
    Switch::dispatch(Toggle());
    TEST_ASSERT_FALSE(Switch::is_in_state<Off>());
    TEST_ASSERT_TRUE(Switch::is_in_state<On>());
    TEST_ASSERT_EQUAL_INT(1, Switch::state<Off>().getCounter());
    TEST_ASSERT_EQUAL_INT(1, Switch::state<On>().getCounter());
    Switch::dispatch(Toggle());
    TEST_ASSERT_TRUE(Switch::is_in_state<Off>());
    TEST_ASSERT_FALSE(Switch::is_in_state<On>());
    TEST_ASSERT_EQUAL_INT(2, Switch::state<Off>().getCounter());
    TEST_ASSERT_EQUAL_INT(1, Switch::state<On>().getCounter());
}

void TestInvalid() {
    Switch::reset();
    Switch::dispatch(tinyfsm::Event());
    TEST_ASSERT_TRUE(Switch::is_in_state<Off>());
    TEST_ASSERT_FALSE(Switch::is_in_state<On>());
    TEST_ASSERT_EQUAL_INT(1, Switch::state<Off>().getCounter());
    TEST_ASSERT_EQUAL_INT(0, Switch::state<On>().getCounter());
}

#endif