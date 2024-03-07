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
        timing.injectEvent(Timing::Event{2000, 0x10, 0x20}, timing.currentTimeTAI());
        timing.injectEvent(Timing::Event{5000, 0x1234, 0x12}, timing.currentTimeTAI());
        std::this_thread::sleep_for(20ms);
        timing.process();
        auto data = reader.get(reader.available());
        expect(data.size() == 2_ul);
        expect(data[0].id() == 0x10);
        expect(data[1].param() == 0x12);
    };

    "snoopAndPublishSimulated"_test = [] {
        Timing timing;
        timing.simulate = true;
        auto reader = timing.snooped.new_reader();
        timing.initialize();
        timing.injectEvent(Timing::Event{2000, 0x10, 0x20}, timing.currentTimeTAI());
        timing.injectEvent(Timing::Event{5000, 0x1234, 0x12}, timing.currentTimeTAI());
        timing.process();
        auto data = reader.get(reader.available());
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
        expect(that % !noEvent.has_value());
        auto event = Timing::Event::fromString("0x1 0x123456789 123456789");
        expect(that % event.has_value());
        expect(that % event->time == 123456789);
        expect(that % event->id() == 0x1);
        expect(that % event->param() == 0x123456789);
        auto realEvent = Timing::Event::fromString("0x112c100c00300085 0x5c00075bcd15 100000000");
        expect(that % realEvent.has_value());
        expect(that % realEvent->time == 100000000);
        expect(that % realEvent->id() == 0x112c100c00300085);
        expect(that % realEvent->param() == 0x5c00075bcd15);
        expect(that % realEvent->bpcid == 23);
        expect(that % realEvent->sid == 3);
        expect(that % realEvent->bpid == 2);
        expect(that % realEvent->gid == 300);
        expect(that % realEvent->eventNo == 256);
        expect(that % realEvent->flagBeamin);
        expect(that % realEvent->flagBpcStart);
        expect(that % !realEvent->reqNoBeam);
        expect(that % realEvent->virtAcc == 5);
        expect(that % realEvent->fid == 1);
        expect(that % realEvent->bpcts == 123456789);
    };
};

}

int
main() { /* tests are statically executed */
}
