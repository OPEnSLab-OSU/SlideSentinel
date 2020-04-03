#pragma once

#include "tinyfsm.h"

template<template<class> class... Sources>
class EventBus {
private:
    using List = tinyfsm::FsmList<Sources<EventBus>...>;


public:
    template<typename E>
    static void dispatch(E const& event) {
        List::dispatch(event);
    }
};

template<class EventBus, template<class> class... Sources>
class FsmList : public tinyfsm::FsmList<Sources<EventBus>...> {};
