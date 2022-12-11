#include "tinyfsm.h"
#include "FlashStorage.h"
#include "RTClib.h""
#include "Plog.h"

struct sync_t { bool is_written; DateTime synctime; };

FlashStorage(PLOGSyncStorage, sync_t);

/**
 * PLOG Time Synchronization service
 * Memorizes the last SyncTime event to flashstorage
 * and uses it to synchronize PLOG on reboot. This
 * ensures that log files don't overlap even after the
 * device is restarted.
 */

template<class EventBus>
class PLOGSynchronizer
    : public tinyfsm::Fsm<PLOGSynchronizer<EventBus>>
{
public:
    static void reset() { }
    static void start();
    static void react(const SyncTime&);
    static void react(tinyfsm::Event const&) { }

    static void entry() { }
    static void exit() { }

    class EmptyState : public PLOGSynchronizer<EventBus> {};

    using InitialState = EmptyState;
};

template <class T>
void PLOGSynchronizer<T>::start() {
    const sync_t time = PLOGSyncStorage.read();
    if (time.is_written) {
      plog::util::Time t;
      plog::util::ftime(&t);
      if (t.time.unixtime() < time.synctime.unixtime())
        plog::TimeSync(time.synctime, 0);
    }
    else
      plog::TimeSync(DateTime(__DATE__, __TIME__), -7);
}

template <class T>
void PLOGSynchronizer<T>::react(const SyncTime& e) {
  const DateTime time(e.time.tm_year, e.time.tm_mon, e.time.tm_yday, e.time.tm_hour, e.time.tm_min, e.time.tm_sec);
  plog::TimeSync(time, 0);
  LOGD << "Synchronized PLOG time to " << time.unixtime();
  PLOGSyncStorage.write({ true, time });
}