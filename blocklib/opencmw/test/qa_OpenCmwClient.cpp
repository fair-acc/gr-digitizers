#include <Client.hpp>
#include <majordomo/Worker.hpp>

#include <fair/opencmw/cmwlight/CmwLightClient.hpp>

#include <boost/ut.hpp>

#include <expected>
#include <print>

#include "ProcessHelper.hpp"

/**
 * Most tests in this suite require a running Test FESA Server which is registered to a Nameserver.
 * Additionally, the following Environment Variables have to be set:
 *  - CMW_NAMESERVER=http://host:port - URL to the Nameserver to query
 *  - CMW_DEVICE_ADDRESS=tcp://host:port - URL where an instance of the GSITemplateDevice FESA class is running
 *    This can be obtained via nameserver lookup, but has to be set if the device needs ssh tunnelling with ssh -L local_port:device:port
 *    Use the output of the nameserver lookup test to determine the hostname and port of the device to set up the ssh tunnel.
 *
 * FESA Test Device:
 * - vmla017
 * - DEV nameserver
 * - Device: GSITemplateDevice
 * - servername: GSITemplate_DU.vmla017
 * - Properties:
 *   - Acquisition:
 *     - Version
 *     - ModuleStatus
 *     - Status
 *     - Acquisition (mux)
 *       - Voltage: generates random values for beam processes 1 and 2 every 2 seconds
 *   - Setting:
 *     - Power
 *     - Init
 *     - Reset
 *     - Setting (mux)
 *       - Voltage: determines the acquisition values
 */

struct TestSettings {
    double voltage = 42.23;
};
ENABLE_REFLECTION_FOR(TestSettings, voltage);

static bool waitFor(const std::atomic<int>& counter, const int expectedValue, const auto timeout) {
    const auto start = std::chrono::system_clock::now();
    while (counter.load() < expectedValue && std::chrono::system_clock::now() - start < timeout) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    return counter.load() == expectedValue;
}

static bool waitFor(const std::atomic<int>& counter, const int expectedValue) {
    using namespace std::literals;
    return waitFor(counter, expectedValue, 1s);
}

const boost::ut::suite OpenCmwTests = [] {
    using namespace boost::ut;
    using namespace std::literals;

    "ParseNameserverReply"_test = [] {
        std::println("# test parsing nameserver reply - starting");
        const std::string nameserverReply = R"""({"resources":[{"name":"GGGG001","server":{"name":"TestDU2.srv001","location":{"domain":"RDA3","endpoint":"9#Address:#string#17#tcp:%2F%2Fsrv001:1234#ApplicationId:#string#108#app=TestDU2;uid=root;host=srv001;pid=1977;os=Linux%2D3%2E10%2E101%2Drt111%2Dpc002;osArch=64bit;appArch=64bit;lang=C%2B%2B;#Language:#string#3#C%2B%2B#Name:#string#14#TestDU2%2Esrv001#Pid:#int#1977#ProcessName:#string#7#TestDU2#StartTime:#long#1769161872191#UserName:#string#4#root#Version:#string#5#3%2E1%2E0"}}}]})""";
        const auto        result          = opencmw::client::cmwlight::DirectoryLightClient::parseNameserverReply(nameserverReply);
        expect(std::get<std::expected<std::string, std::string>>(result.at("GGGG001").at("Address")).value() == "tcp://srv001:1234");
        std::println("# test parsing nameserver reply - finished");
    };

    "Query rda3 directory server/nameserver"_test = [] {
        std::println("# query rda3 nameserver - starting");
        const auto env_nameserver = std::getenv("CMW_NAMESERVER");
        if (env_nameserver == nullptr) {
            std::println("skipping BasicCmwLight example test as it relies on the availability of network infrastructure.");
            return; // skip test
        }
        const std::string                               nameserver{env_nameserver};
        const opencmw::zmq::Context                     zctx{};
        opencmw::client::cmwlight::DirectoryLightClient nameserverClient{nameserver};
        std::optional<std::string>                      result = nameserverClient.lookup("GSITemplateDevice");
        expect(!result.has_value()); // check that the device is originally not in the nameserver's cache
        std::chrono::system_clock::time_point start = std::chrono::system_clock::now();
        while (!(result = nameserverClient.lookup("GSITemplateDevice")).has_value() && std::chrono::system_clock::now() - start < 1s) {
            std::this_thread::sleep_for(1ms);
        }
        expect(result.has_value() && result.value().starts_with("tcp://vmla"));
        // check that later lookups immediately return the cached result
        result = nameserverClient.lookup("GSITemplateDevice");
        expect(result.has_value() && result.value().starts_with("tcp://vmla"));
        std::println("Lookup result: GSITemplateDevice -> {} ", result.value_or("nameserver lookup failed"));
        std::println("# query rda3 nameserver - finished");
    };

    "CmwLightClientGetLocal"_test = [] {
        std::println("# Test rda3 get/set - starting");
        Process rda3Server({"java", "-jar", "./DemoRda3Server.jar", "-cfg", "--cmw.rda3.transport.server.port=12345", "-sleep", "1500"});
        using namespace opencmw;
        using namespace std::literals;
        const std::string DEVICE_NAME          = "GSITemplateDevice";
        const std::string STATUS_PROPERTY      = "/GSITemplateDevice/Status";
        const std::string SETTING_PROPERTY     = "/GSITemplateDevice/Setting";
        const std::string ACQUISITION_PROPERTY = "/GSITemplateDevice/Acquisition";
        const std::string SELECTOR             = "FAIR.SELECTOR.C=3:S=1:P=1:T=300";

        std::string                                      nameserver = "http://localhost:7500"; // this is actually never queried since all requests in this test provide the url authority
        const zmq::Context                               zctx{};
        std::vector<std::unique_ptr<client::ClientBase>> clients;
        auto                                             cmwlightClient = std::make_unique<client::cmwlight::CmwLightClientCtx>(zctx, nameserver, 100ms, "testclient");
        std::string                                      deviceHost     = "rda3://localhost:12345";
        clients.emplace_back(std::move(cmwlightClient));
        client::ClientContext clientContext{std::move(clients)};
        // send some requests
        { // non-multiplexed get request
            auto        endpoint = URI<>::factory(URI(deviceHost)).scheme("rda3tcp").path(STATUS_PROPERTY).build();
            std::atomic getReceived{0};
            clientContext.get(endpoint, [&getReceived](const mdp::Message& message) {
                if (!message.error.empty()) {
                    expect(false) << "get should have succeeded";
                } else {
                    IoBuffer         buffer(message.data);
                    majordomo::Empty empty{};
                    const auto       deserialiserInfo = deserialise<CmwLight, ProtocolCheck::LENIENT>(buffer, empty); // deserialising into an empty struct to get field information
                    std::println("Deserialised subscription reply:\n  fields: {}\n  fieldTypes: {}", deserialiserInfo.additionalFields | std::views::keys, deserialiserInfo.additionalFields | std::views::values);
                    expect(deserialiserInfo.additionalFields.size() == 13_i) << "wrong number of field in subscription update";
                    ++getReceived;
                }
            });
            expect(waitFor(getReceived, 1)) << "timed-out waiting for get response";
        }
        { // multiplexed get request
            auto        endpoint = URI<>::factory(URI(deviceHost)).scheme("rda3tcp").path(ACQUISITION_PROPERTY).addQueryParameter("ctx", SELECTOR).build();
            std::atomic getReceived{0};
            clientContext.get(endpoint, [&getReceived](const mdp::Message& message) {
                if (!message.error.empty()) {
                    expect(false) << "get should have succeeded";
                } else {
                    IoBuffer         buffer(message.data);
                    majordomo::Empty empty{};
                    const auto       deserialiserInfo = deserialise<CmwLight, ProtocolCheck::LENIENT>(buffer, empty); // deserialising into an empty struct to get field information
                    std::println("Deserialised subscription reply:\n  fields: {}\n  fieldTypes: {}", deserialiserInfo.additionalFields | std::views::keys, deserialiserInfo.additionalFields | std::views::values);
                    expect(deserialiserInfo.additionalFields.size() == 12_i) << "wrong number of field in subscription update";
                    ++getReceived;
                }
            });
            expect(waitFor(getReceived, 1)) << "timed-out waiting for get response";
        }
        { // multiplexed set request
            auto         endpoint = URI<>::factory(URI(deviceHost)).scheme("rda3tcp").path(SETTING_PROPERTY).addQueryParameter("ctx", SELECTOR).build();
            std::atomic  setReceived{0};
            TestSettings testSettings{};
            IoBuffer     data{};
            opencmw::serialise<CmwLight, false>(data, testSettings);
            clientContext.set(
                endpoint,
                [&setReceived](const mdp::Message& message) {
                    if (!message.error.empty()) {
                        std::println("Error: {}", message.error);
                        expect(false) << "set should have succeeded";
                    } else {
                        ++setReceived;
                    }
                },
                std::move(data));
            expect(waitFor(setReceived, 1)) << "timed-out waiting for set response";
        }
        { // error-case: multiplexed get request with missing selector
            auto        endpoint = URI<>::factory(URI(deviceHost)).scheme("rda3tcp").path(SETTING_PROPERTY).build();
            std::atomic getError{0};
            clientContext.get(endpoint, [&getError](const mdp::Message& message) {
                if (!message.error.empty()) {
                    expect(message.error.contains("Access point 'GSITemplateDevice/Setting' needs a selector"));
                    ++getError;
                } else {
                    expect(false) << "get should have failed";
                }
            });
            expect(waitFor(getError, 1)) << "timed-out waiting for get error";
        }
        { // error-case: non-existent device property
            auto        endpoint = URI<>::factory(URI(deviceHost)).scheme("rda3tcp").path("GSITemplateDevice/NON_EXISTENT_PROPERTY").build();
            std::atomic getError{0};
            clientContext.get(endpoint, [&getError](const mdp::Message& message) {
                if (!message.error.empty()) {
                    std::println("Error: {}", message.error);
                    expect(message.error.contains("Error in request: UNKNOWN_PROPERTY: Device 'GSITemplateDevice' has no property named 'NON_EXISTENT_PROPERTY'"));
                    ++getError;
                } else {
                    expect(false) << "get should have failed";
                }
            });
            expect(waitFor(getError, 1)) << "timed-out waiting for get error";
        }
        std::println("# Test rda3 get/set - finished");
    };

    "CmwLightClientSubscribeLocal"_test = [] {
        std::println("# test rda3 subscribe - starting");
        Process rda3Server({"java", "-jar", "./DemoRda3Server.jar", "-cfg", "--cmw.rda3.transport.server.port=12345", "-sleep", "1500"});
        using namespace opencmw;
        using namespace std::literals;
        const std::string DEVICE_NAME          = "GSITemplateDevice";
        const std::string STATUS_PROPERTY      = "/GSITemplateDevice/Status";
        const std::string ACQUISITION_PROPERTY = "/GSITemplateDevice/Acquisition";
        const std::string SELECTOR             = "FAIR.SELECTOR.P=1";

        std::string                                      nameserver = "http://localhost:7500";
        const zmq::Context                               zctx{};
        std::vector<std::unique_ptr<client::ClientBase>> clients;
        auto                                             cmwlightClient = std::make_unique<client::cmwlight::CmwLightClientCtx>(zctx, nameserver, 100ms, "testclient");
        std::string                                      deviceHost     = "rda3://localhost:12345";
        clients.emplace_back(std::move(cmwlightClient));
        client::ClientContext clientContext{std::move(clients)};
        { // Subscribe non-multiplexed
            std::atomic subscriptionUpdatesReceived{0};
            auto        subscriptionEndpoint = URI<>::factory(URI(deviceHost)).scheme("rda3tcp").path(STATUS_PROPERTY).build();
            clientContext.subscribe(subscriptionEndpoint, [&subscriptionUpdatesReceived](const mdp::Message& message) {
                if (!message.error.empty()) {
                    expect(false) << "subscription should not notify exceptions";
                } else {
                    IoBuffer         buffer(message.data);
                    majordomo::Empty empty{};
                    const auto       deserialiserInfo = deserialise<CmwLight, ProtocolCheck::LENIENT>(buffer, empty); // deserialising into an empty struct to get field information
                    std::println("Deserialised subscription reply:\n  fields: {}\n  fieldTypes: {}", deserialiserInfo.additionalFields | std::views::keys, deserialiserInfo.additionalFields | std::views::values);
                    expect(deserialiserInfo.additionalFields.size() == 13_i) << "wrong number of field in subscription update";
                    ++subscriptionUpdatesReceived;
                }
            });
            expect(waitFor(subscriptionUpdatesReceived, 2, 5s)) << "timed-out waiting for subscription updates"; // property gets notified every 2 seconds
            // Check that subscription updates stop after unsubscribing
            clientContext.unsubscribe(subscriptionEndpoint);
            std::this_thread::sleep_for(200ms); // get a few subscription updates
            int subscriptionUpdatesAfterUnsubscribe = subscriptionUpdatesReceived;
            std::this_thread::sleep_for(5s); // get a few subscription updates
            expect(eq(subscriptionUpdatesReceived.load(), subscriptionUpdatesAfterUnsubscribe));
        }
        { // error case: subscribe to multiplexed without selector
            std::atomic subscriptionUpdateError{0};
            auto        subscriptionEndpoint = URI<>::factory(URI(deviceHost)).scheme("rda3tcp").path(ACQUISITION_PROPERTY).build();
            clientContext.subscribe(subscriptionEndpoint, [&subscriptionUpdateError](const mdp::Message& message) {
                if (!message.error.empty()) {
                    expect(message.error.contains("Access point 'GSITemplateDevice/Acquisition' needs a selector"));
                    ++subscriptionUpdateError;
                } else {
                    expect(false) << "invalid subscription should not notify updates";
                }
            });
            expect(waitFor(subscriptionUpdateError, 1, 1s)) << "timed-out waiting for subscription error";
        }
        { // subscribe to multiplexed property
            std::atomic subscriptionUpdatesReceived{0};
            auto        subscriptionEndpoint = URI<>::factory(URI(deviceHost)).scheme("rda3tcp").path(ACQUISITION_PROPERTY).addQueryParameter("ctx", SELECTOR).build();
            std::println("Subscribing to acquisition property with url {}", subscriptionEndpoint);
            clientContext.subscribe(subscriptionEndpoint, [&subscriptionUpdatesReceived](const mdp::Message& message) {
                if (!message.error.empty()) {
                    expect(false) << "subscription should not notify exceptions";
                } else {
                    IoBuffer         buffer(message.data);
                    majordomo::Empty empty{};
                    const auto       deserialiserInfo = deserialise<CmwLight, ProtocolCheck::LENIENT>(buffer, empty); // deserialising into an empty struct to get field information
                    std::println("Deserialised subscription reply:\n  fields: {}\n  fieldTypes: {}", deserialiserInfo.additionalFields | std::views::keys, deserialiserInfo.additionalFields | std::views::values);
                    expect(deserialiserInfo.additionalFields.size() == 12_i) << "wrong number of fields in subscription update";
                    ++subscriptionUpdatesReceived;
                }
            });
            expect(waitFor(subscriptionUpdatesReceived, 2, 5s)) << "timed-out waiting for subscription updates"; // property gets notified every 2 seconds
            // Check that subscription updates stop after unsubscribing
            clientContext.unsubscribe(subscriptionEndpoint);
            std::this_thread::sleep_for(200ms); // get a few subscription updates
            int subscriptionUpdatesAfterUnsubscribe = subscriptionUpdatesReceived;
            std::this_thread::sleep_for(5s); // get a few subscription updates
            expect(eq(subscriptionUpdatesReceived.load(), subscriptionUpdatesAfterUnsubscribe));
        }
        std::println("# test rda3 subscribe - finished");
    };

    "CmwLightClientGetDev"_test = [] {
        std::println("# Test rda3 get/set - starting");
        using namespace opencmw;
        using namespace std::literals;
        const std::string DEVICE_NAME          = "GSITemplateDevice";
        const std::string STATUS_PROPERTY      = "/GSITemplateDevice/Status";
        const std::string SETTING_PROPERTY     = "/GSITemplateDevice/Setting";
        const std::string ACQUISITION_PROPERTY = "/GSITemplateDevice/Acquisition";
        const std::string SELECTOR             = "FAIR.SELECTOR.C=3:S=1:P=1:T=300";

        const char* env_nameserver = std::getenv("CMW_NAMESERVER");
        if (env_nameserver == nullptr) {
            std::println("skipping BasicCmwLight example test as it relies on the availability of network infrastructure. Define CMW_NAMESERVER environment variable to run this test.");
            return; // skip test
        }
        std::string                                      nameserver{env_nameserver};
        const zmq::Context                               zctx{};
        std::vector<std::unique_ptr<client::ClientBase>> clients;
        auto                                             cmwlightClient = std::make_unique<client::cmwlight::CmwLightClientCtx>(zctx, nameserver, 100ms, "testclient");
        std::string                                      deviceHost{}; // "tcp://vmla017:36725" };
        if (const char* env_cmw_host = std::getenv("CMW_DEVICE_HOST"); env_cmw_host != nullptr) {
            deviceHost = std::string{env_cmw_host};
            cmwlightClient->nameserverClient().addStaticLookup(DEVICE_NAME, std::string{env_cmw_host});
        } else {
            while (!deviceHost.empty()) {
                deviceHost = cmwlightClient->nameserverClient().lookup(DEVICE_NAME).value_or("");
            }
        }
        clients.emplace_back(std::move(cmwlightClient));
        client::ClientContext clientContext{std::move(clients)};
        // send some requests
        { // non-multiplexed get request
            auto        endpoint = URI<>::factory(URI(deviceHost)).scheme("rda3tcp").path(STATUS_PROPERTY).build();
            std::atomic getReceived{0};
            clientContext.get(endpoint, [&getReceived](const mdp::Message& message) {
                if (!message.error.empty()) {
                    expect(false) << "get should have succeeded";
                } else {
                    IoBuffer         buffer(message.data);
                    majordomo::Empty empty{};
                    const auto       deserialiserInfo = deserialise<CmwLight, ProtocolCheck::LENIENT>(buffer, empty); // deserialising into an empty struct to get field information
                    std::println("Deserialised subscription reply:\n  fields: {}\n  fieldTypes: {}", deserialiserInfo.additionalFields | std::views::keys, deserialiserInfo.additionalFields | std::views::values);
                    expect(deserialiserInfo.additionalFields.size() == 13_i) << "wrong number of field in subscription update";
                    ++getReceived;
                }
            });
            expect(waitFor(getReceived, 1)) << "timed-out waiting for get response";
        }
        { // multiplexed get request
            auto        endpoint = URI<>::factory(URI(deviceHost)).scheme("rda3tcp").path(ACQUISITION_PROPERTY).addQueryParameter("ctx", SELECTOR).build();
            std::atomic getReceived{0};
            clientContext.get(endpoint, [&getReceived](const mdp::Message& message) {
                if (!message.error.empty()) {
                    expect(false) << "get should have succeeded";
                } else {
                    IoBuffer         buffer(message.data);
                    majordomo::Empty empty{};
                    const auto       deserialiserInfo = deserialise<CmwLight, ProtocolCheck::LENIENT>(buffer, empty); // deserialising into an empty struct to get field information
                    std::println("Deserialised subscription reply:\n  fields: {}\n  fieldTypes: {}", deserialiserInfo.additionalFields | std::views::keys, deserialiserInfo.additionalFields | std::views::values);
                    expect(deserialiserInfo.additionalFields.size() == 12_i) << "wrong number of field in subscription update";
                    ++getReceived;
                }
            });
            expect(waitFor(getReceived, 1)) << "timed-out waiting for get response";
        }
        { // multiplexed set request
            auto         endpoint = URI<>::factory(URI(deviceHost)).scheme("rda3tcp").path(SETTING_PROPERTY).addQueryParameter("ctx", SELECTOR).build();
            std::atomic  setReceived{0};
            TestSettings testSettings{};
            IoBuffer     data{};
            opencmw::serialise<CmwLight, false>(data, testSettings);
            clientContext.set(
                endpoint,
                [&setReceived](const mdp::Message& message) {
                    if (!message.error.empty()) {
                        std::println("Error: {}", message.error);
                        expect(false) << "set should have succeeded";
                    } else {
                        ++setReceived;
                    }
                },
                std::move(data));
            expect(waitFor(setReceived, 1)) << "timed-out waiting for set response";
        }
        { // error-case: multiplexed get request with missing selector
            auto        endpoint = URI<>::factory(URI(deviceHost)).scheme("rda3tcp").path(SETTING_PROPERTY).build();
            std::atomic getError{0};
            clientContext.get(endpoint, [&getError](const mdp::Message& message) {
                if (!message.error.empty()) {
                    expect(message.error.contains("Access point 'GSITemplateDevice/Setting' needs a selector"));
                    ++getError;
                } else {
                    expect(false) << "get should have failed";
                }
            });
            expect(waitFor(getError, 1)) << "timed-out waiting for get error";
        }
        { // error-case: non-existent device property
            auto        endpoint = URI<>::factory(URI(deviceHost)).scheme("rda3tcp").path("GSITemplateDevice/NON_EXISTENT_PROPERTY").build();
            std::atomic getError{0};
            clientContext.get(endpoint, [&getError](const mdp::Message& message) {
                if (!message.error.empty()) {
                    std::println("Error: {}", message.error);
                    expect(message.error.contains("Error in request: UNKNOWN_PROPERTY: Device 'GSITemplateDevice' has no property named 'NON_EXISTENT_PROPERTY'"));
                    ++getError;
                } else {
                    expect(false) << "get should have failed";
                }
            });
            expect(waitFor(getError, 1)) << "timed-out waiting for get error";
        }
        std::println("# Test rda3 get/set - finished");
    };

    "CmwLightClientSubscribeDev"_test = [] {
        std::println("# test rda3 subscribe - starting");
        using namespace opencmw;
        using namespace std::literals;
        const std::string DEVICE_NAME          = "GSITemplateDevice";
        const std::string STATUS_PROPERTY      = "/GSITemplateDevice/Status";
        const std::string ACQUISITION_PROPERTY = "/GSITemplateDevice/Acquisition";
        const std::string SELECTOR             = "FAIR.SELECTOR.P=1";

        const char* env_nameserver = std::getenv("CMW_NAMESERVER");
        if (env_nameserver == nullptr) {
            std::println("skipping BasicCmwLight example test as it relies on the availability of network infrastructure. Define CMW_NAMESERVER environment variable to run this test.");
            return; // skip test
        }
        std::string                                      nameserver{env_nameserver};
        const zmq::Context                               zctx{};
        std::vector<std::unique_ptr<client::ClientBase>> clients;
        auto                                             cmwlightClient = std::make_unique<client::cmwlight::CmwLightClientCtx>(zctx, nameserver, 100ms, "testclient");
        std::string                                      deviceHost{}; // "tcp://vmla017:36725" };
        if (const char* env_cmw_host = std::getenv("CMW_DEVICE_HOST"); env_cmw_host != nullptr) {
            deviceHost = std::string{env_cmw_host};
            cmwlightClient->nameserverClient().addStaticLookup(DEVICE_NAME, std::string{env_cmw_host});
        } else {
            while (!deviceHost.empty()) {
                deviceHost = cmwlightClient->nameserverClient().lookup(DEVICE_NAME).value_or("");
            }
        }
        clients.emplace_back(std::move(cmwlightClient));
        client::ClientContext clientContext{std::move(clients)};
        { // Subscribe non-multiplexed
            std::atomic subscriptionUpdatesReceived{0};
            auto        subscriptionEndpoint = URI<>::factory(URI(deviceHost)).scheme("rda3tcp").path(STATUS_PROPERTY).build();
            clientContext.subscribe(subscriptionEndpoint, [&subscriptionUpdatesReceived](const mdp::Message& message) {
                if (!message.error.empty()) {
                    expect(false) << "subscription should not notify exceptions";
                } else {
                    IoBuffer         buffer(message.data);
                    majordomo::Empty empty{};
                    const auto       deserialiserInfo = deserialise<CmwLight, ProtocolCheck::LENIENT>(buffer, empty); // deserialising into an empty struct to get field information
                    std::println("Deserialised subscription reply:\n  fields: {}\n  fieldTypes: {}", deserialiserInfo.additionalFields | std::views::keys, deserialiserInfo.additionalFields | std::views::values);
                    expect(deserialiserInfo.additionalFields.size() == 13_i) << "wrong number of field in subscription update";
                    ++subscriptionUpdatesReceived;
                }
            });
            expect(waitFor(subscriptionUpdatesReceived, 2, 5s)) << "timed-out waiting for subscription updates"; // property gets notified every 2 seconds
            // Check that subscription updates stop after unsubscribing
            clientContext.unsubscribe(subscriptionEndpoint);
            std::this_thread::sleep_for(200ms); // get a few subscription updates
            int subscriptionUpdatesAfterUnsubscribe = subscriptionUpdatesReceived;
            std::this_thread::sleep_for(5s); // get a few subscription updates
            expect(eq(subscriptionUpdatesReceived.load(), subscriptionUpdatesAfterUnsubscribe));
        }
        { // error case: subscribe to multiplexed without selector
            std::atomic subscriptionUpdateError{0};
            auto        subscriptionEndpoint = URI<>::factory(URI(deviceHost)).scheme("rda3tcp").path(ACQUISITION_PROPERTY).build();
            clientContext.subscribe(subscriptionEndpoint, [&subscriptionUpdateError](const mdp::Message& message) {
                if (!message.error.empty()) {
                    expect(message.error.contains("Access point 'GSITemplateDevice/Acquisition' needs a selector"));
                    ++subscriptionUpdateError;
                } else {
                    expect(false) << "invalid subscription should not notify updates";
                }
            });
            expect(waitFor(subscriptionUpdateError, 1, 1s)) << "timed-out waiting for subscription error";
        }
        { // subscribe to multiplexed property
            std::atomic subscriptionUpdatesReceived{0};
            auto        subscriptionEndpoint = URI<>::factory(URI(deviceHost)).scheme("rda3tcp").path(ACQUISITION_PROPERTY).addQueryParameter("ctx", SELECTOR).build();
            std::println("Subscribing to acquisition property with url {}", subscriptionEndpoint);
            clientContext.subscribe(subscriptionEndpoint, [&subscriptionUpdatesReceived](const mdp::Message& message) {
                if (!message.error.empty()) {
                    expect(false) << "subscription should not notify exceptions";
                } else {
                    IoBuffer         buffer(message.data);
                    majordomo::Empty empty{};
                    const auto       deserialiserInfo = deserialise<CmwLight, ProtocolCheck::LENIENT>(buffer, empty); // deserialising into an empty struct to get field information
                    std::println("Deserialised subscription reply:\n  fields: {}\n  fieldTypes: {}", deserialiserInfo.additionalFields | std::views::keys, deserialiserInfo.additionalFields | std::views::values);
                    expect(deserialiserInfo.additionalFields.size() == 12_i) << "wrong number of fields in subscription update";
                    ++subscriptionUpdatesReceived;
                }
            });
            expect(waitFor(subscriptionUpdatesReceived, 2, 5s)) << "timed-out waiting for subscription updates"; // property gets notified every 2 seconds
            // Check that subscription updates stop after unsubscribing
            clientContext.unsubscribe(subscriptionEndpoint);
            std::this_thread::sleep_for(200ms); // get a few subscription updates
            int subscriptionUpdatesAfterUnsubscribe = subscriptionUpdatesReceived;
            std::this_thread::sleep_for(5s); // get a few subscription updates
            expect(eq(subscriptionUpdatesReceived.load(), subscriptionUpdatesAfterUnsubscribe));
        }
        std::println("# test rda3 subscribe - finished");
    };
};

int main() { /* tests are statically executed */ }
