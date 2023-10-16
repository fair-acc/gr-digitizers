#include <format>
#include <thread>

#include <CLI/CLI.hpp>

#include <SAFTd.h>
#include <TimingReceiver.h>
#include <SoftwareActionSink.h>
#include <SoftwareCondition.h>
#include <CommonFunctions.h>

using saftlib::SAFTd_Proxy;
using saftlib::TimingReceiver_Proxy;
using saftlib::SoftwareActionSink_Proxy;
using saftlib::SoftwareCondition_Proxy;

int main(int argc, char** argv) {
    CLI::App app{"timing receiver saftbus example"};
    bool inject = false;
    app.add_flag("-i", inject, "inject");
    uint64_t eventID     = 0x0; // full 64 bit EventID contained in the timing message
    app.add_option("-e", eventID, "event id to send");
    uint64_t eventParam  = 0x0; // full 64 bit parameter contained in the timing message
    app.add_option("-p", eventParam, "event parameters to send");
    uint64_t eventTNext  = 0x0; // time for next event (this value is added to the current time or the next PPS, see option -p
    app.add_option("-t", eventTNext, "time to send the event");
    bool ppsAlign= false;
    app.add_flag("--pps", ppsAlign, "add time to next pps pulse");
    bool absoluteTime = false;
    app.add_flag("--abs", absoluteTime, "time is an absolute time instead of an offset");
    bool UTC = false;
    app.add_flag("--utc", UTC, "absolute time is in utc, default is tai");
    bool UTCleap = false;
    app.add_flag("--leap", UTCleap, "utc calculation leap second flag");

    bool snoop = false;
    app.add_flag("-s", snoop, "snoop");
    uint64_t snoopID     = 0x0;
    app.add_option("-f", snoopID, "id filter");
    uint64_t snoopMask   = 0x0;
    app.add_option("-m", snoopMask, "snoop mask");
    int64_t  snoopOffset = 0x0;
    app.add_option("-o", snoopOffset, "snoop offset");
    int64_t snoopSeconds = -1;
    app.add_option("-d", snoopSeconds, "snoop duration in seconds, -1 for infinite");

    uint32_t pmode = PMODE_HEX;
    app.add_option("--print-mode", pmode, "print mode PMODE_NONE(0x0), PMODE_DEC(0x1), PMODE_HEX(0x2), PMODE_VERBOSE(0x4), PMODE_UTC(0x8)");

    CLI11_PARSE(app, argc, argv);

    try {
        // initialize required stuff
        std::shared_ptr<SAFTd_Proxy> saftd = SAFTd_Proxy::create();

        // get a specific device
        std::map<std::string, std::string> devices = saftd->getDevices();
        if (devices.empty()) {
            std::cerr << "No devices attached to saftd" << std::endl;
            return -1;
        }
        std::shared_ptr<TimingReceiver_Proxy> receiver = TimingReceiver_Proxy::create(devices.begin()->second);

        if (inject) {
            const saftlib::Time wrTime = receiver->CurrentTime(); // current WR time
            saftlib::Time eventTime; // time for next event in PTP time
            if (ppsAlign) {
                const saftlib::Time ppsNext   = (wrTime - (wrTime.getTAI() % 1000000000)) + 1000000000; // time for next PPS
                eventTime = (ppsNext + eventTNext);
            } else if (absoluteTime) {
                if (UTC) {
                    eventTime = saftlib::makeTimeUTC(eventTNext, UTCleap);
                } else {
                    eventTime = saftlib::makeTimeTAI(eventTNext);
                }
            } else {
                eventTime = wrTime + eventTNext;
            }
            receiver->InjectEvent(eventID, eventParam, eventTime);
            if (pmode & PMODE_HEX) {
              std::cout << "Injected event (eventID/parameter/time): 0x" << std::hex << std::setw(16) << std::setfill('0') << eventID
                                                                            << " 0x" << std::setw(16) << std::setfill('0') << eventParam
                                                                            << " 0x" << std::setw(16) << std::setfill('0') << (UTC?eventTime.getUTC():eventTime.getTAI()) << std::dec << std::endl;
            }
        }

        if (snoop) {
          std::shared_ptr<SoftwareActionSink_Proxy> sink = SoftwareActionSink_Proxy::create(receiver->NewSoftwareActionSink("gr_timing_example"));
          std::shared_ptr<SoftwareCondition_Proxy> condition = SoftwareCondition_Proxy::create(sink->NewCondition(false, snoopID, snoopMask, snoopOffset));
          // Accept all errors
          condition->setAcceptLate(true);
          condition->setAcceptEarly(true);
          condition->setAcceptConflict(true);
          condition->setAcceptDelayed(true);
          condition->SigAction.connect([&pmode](uint64_t id, uint64_t param, saftlib::Time deadline, saftlib::Time executed, uint16_t flags) {
              std::cout << "tDeadline: " << tr_formatDate(deadline, pmode, false);
              // print event id
              auto fid   = ((id >> 60) & 0xf);   // 1
              auto gid   = ((id >> 48) & 0xfff); // 4
              auto evtno = ((id >> 36) & 0xfff); // 4
              if (fid == 0) { //full << " FLAGS: N/A";
                  auto flgs = 0;
                  auto sid   = ((id >> 24) & 0xfff);  // 4
                  auto bpid  = ((id >> 10) & 0x3fff); // 5
                  auto res   = (id & 0x3ff);          // 4
                  std::cout << std::format("FID={:#01X}, GID={:#04X}, EVTNO={:#04X}, FLAGS={:#01X}, SID={:#04X}, BPID={:#05X}, RES={:#04X}, id={:#016X}",
                                           fid, gid, evtno, flgs, sid, bpid, res, id);
              } else if (fid == 1) {
                  auto flgs = ((id >> 32) & 0xf);   // 1
                  auto bpc   = ((id >> 34) & 0x1);   // 1
                  auto sid   = ((id >> 20) & 0xfff); // 4
                  auto bpid  = ((id >> 6) & 0x3fff); // 5
                  auto res   = (id & 0x3f);          // 4
                  std::cout << std::format("FID={:#01X}, GID={:#04X}, EVTNO={:#04X}, FLAGS={:#01X}, BPC={:#01X}, SID={:#04X}, BPID={:#05X}, RES={:#04X}, id={:#016X}",
                                           fid, gid, evtno, flgs, bpc, sid, bpid, res, id);
              } else {
                  auto other = (id & 0xfffffffff);   // 9
                  std::cout << std::format("FID={:#01X}, GID={:#04X}, EVTNO={:#04X}, OTHER={:#09X}, id={:#016X}", fid, gid, evtno, other, id);
              }
              // print parameter
              std::cout << std::format(", Param={:#016X}", param);
              // print flags
              auto delay = executed - deadline;
              if (flags & 1) {
                  std::cout << std::format(" !late (by {} ns)", delay);
              }
              if (flags & 2) {
                  std::cout << std::format(" !early (by {} ns)", delay);
              }
              if (flags & 4) {
                  std::cout << std::format(" !conflict (delayed by {} ns)", delay);
              }
              if (flags & 8) {
                  std::cout << std::format(" !delayed (by {} ns)", delay);
              }
          });
          condition->setActive(true);
          const auto startTime = std::chrono::system_clock::now();
          while(true) {
              if (snoopSeconds < 0) { // wait infinitely
                  saftlib::wait_for_signal(-1);
              } else {
                  auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(startTime - std::chrono::system_clock::now() + std::chrono::seconds(snoopSeconds)).count();
                  if (duration > 0) {
                      saftlib::wait_for_signal( duration >= std::numeric_limits<int>::max() ? std::numeric_limits<int>::max() : duration);
                  } else {
                      break;
                  }
              }
          }
        }
    } catch (const saftbus::Error& error) {
      std::cerr << "Failed to invoke method: \'" << error.what() << "\'" << std::endl;
    }

    return 0;
}
