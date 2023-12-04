#include <boost/ut.hpp>
#include <timing.hpp>

using namespace std::string_literals;

namespace fair::timing::test {

const boost::ut::suite TimingTests = [] {
    using namespace boost::ut;
    using namespace std::chrono_literals;

    tag("timing-hardware") /
    "snoopAndPublish"_test = [] {
        Timing timing;
        auto reader = timing.snooped.new_reader();
        timing.initialize();
        timing.injectEvent(Timing::Event{2000, 0x10, 0x20}, timing.getTAI());
        timing.injectEvent(Timing::Event{5000, 0x1234, 0x12}, timing.getTAI());
        std::this_thread::sleep_for(20ms);
        timing.process();
        auto data = reader.get();
        expect(data.size() == 2_ul);
        expect(data[0].id() == 0x10);
        expect(data[1].param() == 0x12);
    };

    "snoopAndPublishSimulated"_test = [] {
        Timing timing;
        timing.simulate = true;
        auto reader = timing.snooped.new_reader();
        timing.initialize();
        timing.injectEvent(Timing::Event{2000, 0x10, 0x20}, timing.getTAI());
        timing.injectEvent(Timing::Event{5000, 0x1234, 0x12}, timing.getTAI());
        timing.process();
        auto data = reader.get();
        expect(data.size() == 2_ul);
        expect(data[0].id() == 0x10);
        expect(data[1].param() == 0x12);
    };

    "TimingEvent"_test = [] {
        Timing::Event defaultInitialized{};
        expect(defaultInitialized.fid == 1_ul);
        expect(defaultInitialized.time == 0_ul);
    };

    "TimingEventFromString"_test = [] {
        auto noEvent = Timing::Event::fromString("");
        expect(!noEvent.has_value());
        auto event = Timing::Event::fromString("0x1 0x123456789 123456789");
        expect(event.has_value());
        expect(event->time == 123456789_ul);
        expect(event->id() == 0x1);
        expect(event->param() == 0x123456789);
        auto realEvent = Timing::Event::fromString("0x112c100c00300085 0x5c00075bcd15 100000000");
        expect(realEvent.has_value());
        expect(realEvent->time == 100000000_ul);
        expect(realEvent->id() == 0x112c100c00300085);
        expect(realEvent->param() == 0x5c00075bcd15);
        expect(realEvent->bpcid == 23);
        expect(realEvent->sid == 3);
        expect(realEvent->bpid == 2);
        expect(realEvent->gid == 300);
        expect(realEvent->eventNo == 256);
        expect(realEvent->flagBeamin);
        expect(realEvent->flagBpcStart);
        expect(!realEvent->reqNoBeam);
        expect(realEvent->virtAcc == 5);
        expect(realEvent->fid == 1);
        expect(realEvent->bpcts == 123456789);
    };
};

}

int
main() { /* tests are statically executed */
}
