#ifndef OPENCMW_CPP_DIRECTORYLIGHTCLIENT_HPP
#define OPENCMW_CPP_DIRECTORYLIGHTCLIENT_HPP

#include <latch>
#include <map>
#include <ranges>
#include <string_view>

#include <ThreadPool.hpp>

#include <cpr/cpr.h>

#include <IoSerialiserJson.hpp>

namespace opencmw::client::cmwlight {
struct NameserverReplyLocation {
    std::string domain;
    std::string endpoint;
};
struct NameserverReplyServer {
    std::string             name;
    NameserverReplyLocation location;
};

struct NameserverReplyResource {
    std::string           name;
    NameserverReplyServer server;
};

struct NameserverReply {
    std::vector<NameserverReplyResource> resources;
};
} // namespace opencmw::client::cmwlight
ENABLE_REFLECTION_FOR(opencmw::client::cmwlight::NameserverReplyLocation, domain, endpoint)
ENABLE_REFLECTION_FOR(opencmw::client::cmwlight::NameserverReplyServer, name, location)
ENABLE_REFLECTION_FOR(opencmw::client::cmwlight::NameserverReplyResource, name, server)
ENABLE_REFLECTION_FOR(opencmw::client::cmwlight::NameserverReply, resources)

/*
 * Implements the CMW Directory Server lookup
 *
 * minimal API usage example:
 * curl ${CMW_NAMESERVER}/api/v1/devices/search --json '{ "proxyPreferred" : true, "domains" : [ ], "directServers" : [ ], "redirects" : { }, "names" : [ "GSITemplateDevice" ]}'
 * {"resources":[{"name":"GSITemplateDevice","server":{"name":"GSITemplate_DU.vmla017","location":{"domain":"RDA3","endpoint":"9#Address:#string#19#tcp:%2F%2Fvmla017:36725#ApplicationId:#string#124#app=GSITemplate_DU;uid=root;host=vmla017;pid=398;os=Linux%2D6%2E6%2E111%2Drt31%2Dyocto%2Dpreempt%2Drt;osArch=64bit;appArch=64bit;lang=C%2B%2B;#Language:#string#3#C%2B%2B#Name:#string#22#GSITemplate_DU%2Evmla017#Pid:#int#398#ProcessName:#string#14#GSITemplate_DU#StartTime:#long#1771235547903#UserName:#string#4#root#Version:#string#5#5%2E1%2E1"}}}]}
 */

namespace opencmw::client::cmwlight {
class DirectoryLightClient {
    using EndpointMap = std::map<std::string, std::variant<std::expected<std::string, std::string>, std::expected<int, std::string>, std::expected<long, std::string>>, std::less<>>;
    std::map<std::string, std::string, std::less<>> cache;
    std::mutex                                      mutex;
    std::deque<std::string>                         pendingLookups;
    std::string                                     nameserver;

public:
    explicit DirectoryLightClient(std::string _nameserver) : nameserver(std::move(_nameserver)) {}

    ~DirectoryLightClient() { cpr::GlobalThreadPool::GetInstance()->Stop(); };

    std::optional<std::string> lookup(const std::string_view name, const bool useCache = true) {
        {
            std::lock_guard lock{mutex};
            if (useCache) {
                if (const auto& res = cache.find(name); res != cache.end()) {
                    if (!res->second.empty()) {
                        return res->second;
                    } else {
                        return {}; // request was already sent
                    }
                }
            }
            cache[std::string{name}] = "";
        }
        triggerRequest(name);
        return {};
    }

    void addStaticLookup(const std::string& deviceName, const std::string_view address) {
        std::lock_guard lock{mutex};
        cache[deviceName] = address;
    };

    static std::expected<std::string, std::string> urlDecode(std::string_view str) {
        std::string ret;
        ret.reserve(str.length());
        const std::size_t len = str.length();
        for (std::size_t i = 0; i < len; i++) {
            if (str[i] != '%') {
                if (str[i] == '+') {
                    ret += ' ';
                } else {
                    ret += str[i];
                }
            } else if (i + 2 < len) {
                auto isHex = [](char c) { return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F'); };
                auto toHex = [](char c) {
                    if (c >= '0' && c <= '9') {
                        return c - '0';
                    }
                    if (c >= 'a' && c <= 'f') {
                        return c - 'a' + 10;
                    }
                    if (c >= 'A' && c <= 'F') {
                        return c - 'A' + 10;
                    }
                    return 0;
                };
                if (!isHex(str.at(i + 1)) || !isHex(str.at(i + 2))) {
                    return std::unexpected(std::format("invalid hexadecimal encoding at position {} in '{}'", i, str));
                }
                const auto ch = static_cast<char>('\x10' * toHex(str.at(i + 1)) + toHex(str.at(i + 2)));
                ret += ch;
                i = i + 2;
            } else {
                return std::unexpected(std::format("incomplete hexadecimal encoding at position {} in '{}'", i, str));
            }
        }
        return ret;
    }

    static std::expected<std::string, std::string> parseEndpointString(std::string_view sizeStr, std::string_view valueStr) {
        std::string str2{sizeStr};
        char*       parsed;
        std::size_t size = std::strtoull(str2.data(), &parsed, 10);
        if (parsed != std::to_address(str2.end())) {
            return std::unexpected(std::format("Error parsing string size: {}", sizeStr));
        }
        if (const auto result = urlDecode(valueStr); result.has_value()) {
            if (result.value().size() == size) {
                return result;
            } else {
                return std::unexpected(std::format("invalid size {} for string value {}", size, valueStr));
            }
        } else {
            return result;
        }
    }

    static std::expected<int, std::string> parseEndpointInt(std::string_view str) { return std::atoi(str.data()); }

    static std::expected<long, std::string> parseEndpointLong(std::string_view str) {
        std::string str2{str};
        auto        parsed = std::to_address(str2.end());
        long        number = std::strtol(str2.data(), &parsed, 10);
        if (parsed != std::to_address(str2.end())) {
            return std::unexpected(std::format("Error parsing long: {}", str));
        }
        return number;
    }

    static EndpointMap parseEndpoint(std::string_view endpoint) {
        using namespace std::literals;
        auto tokens = std::views::split(endpoint, "#"sv);
        if (tokens.empty()) {
            return {};
        }
        auto        iterator         = tokens.begin();
        std::string fieldCountString = {(*iterator).data(), (*iterator).size()};
        char*       end              = to_address(fieldCountString.end());
        std::size_t fieldCount       = std::strtoull(fieldCountString.data(), &end, 10);
        ++iterator;
        EndpointMap result;
        std::size_t n = 0;
        while (n < fieldCount && iterator != tokens.end()) {
            std::string_view fieldNameView{&(*iterator).front(), (*iterator).size()};
            std::string      fieldname{fieldNameView.substr(0, fieldNameView.size() - 1)};
            if (++iterator == tokens.end()) {
                result.try_emplace("parserError", std::expected<std::string, std::string>(std::unexpected(std::format("incomplete field {} in endpoint {}", fieldname, endpoint))));
                break;
            }
            std::string type{std::string_view{&(*iterator).front(), (*iterator).size()}};
            if (++iterator == tokens.end()) {
                result.try_emplace("parserError", std::expected<std::string, std::string>(std::unexpected(std::format("incomplete field {} in endpoint {}", fieldname, endpoint))));
                break;
            }
            std::string_view valueView{&(*iterator).front(), (*iterator).size()};
            if (type == "string") {
                if (++iterator == tokens.end()) {
                    result.try_emplace("parserError", std::expected<std::string, std::string>(std::unexpected(std::format("incomplete field {} in endpoint {}", fieldname, endpoint))));
                    break;
                }
                auto value = parseEndpointString(valueView, std::string_view{&(*iterator).front(), (*iterator).size()});
                result.try_emplace(fieldname, value);
            } else if (type == "int") {
                auto value = parseEndpointInt(valueView);
                result.try_emplace(fieldname, value);
            } else if (type == "long") {
                auto value = parseEndpointLong(valueView);
                result.try_emplace(fieldname, value);
            } else {
                result.try_emplace(fieldname, std::expected<std::string, std::string>(std::unexpected(std::format("invalid field type {} in '{}'", type, endpoint))));
            }
            ++iterator;
            ++n;
        }
        if (iterator != tokens.end()) {
            result.try_emplace("parserError", std::expected<std::string, std::string>(std::unexpected(std::format("expected {} fields in '{}', but there are more fields", fieldCount, endpoint))));
        }
        return result;
    }

    static std::map<std::string, EndpointMap, std::less<>> parseNameserverReply(const std::string& reply) {
        IoBuffer        buffer{reply.data(), reply.size()};
        NameserverReply replyObj;
        auto            res = opencmw::deserialise<Json, ProtocolCheck::LENIENT>(buffer, replyObj);
        for (const auto& [name, server] : replyObj.resources) {
            if (server.location.domain != "RDA3") {
                continue;
            }
            return {{name, parseEndpoint(server.location.endpoint)}};
        }
        return {};
    }

    void triggerRequest(std::string_view name) {
        using namespace std::literals;
        const std::string requestData = std::format(R"""({{ "proxyPreferred" : true, "domains" : [ ], "directServers" : [ ], "redirects" : {{ }}, "names" : [ "{}" ]}})""", name);
        const auto        uri         = URI<>::factory(URI(std::string(nameserver))).path("/api/v1/devices/search").build();
        std::string       deviceName{name};
        cpr::PostCallback(
            [this, deviceName](const cpr::Response& response) {
                if (response.status_code == 200) {
                    for (auto& [nameKey, data] : parseNameserverReply(response.text)) {
                        if (nameKey != deviceName) {
                            continue;
                        }
                        std::lock_guard lock{mutex};
                        cache[std::string{nameKey}] = std::get<std::expected<std::string, std::string>>(data.at("Address")).value();
                        return;
                    }
                }
                std::lock_guard lock{mutex};
                cache.erase(deviceName); // lookup failed, do not cache failure
            },
            cpr::Url{uri.str()}, cpr::Body{requestData}, cpr::Header{{"Content-Type", "application/json"}});
    }
};
} // namespace opencmw::client::cmwlight
#endif // OPENCMW_CPP_DIRECTORYLIGHTCLIENT_HPP
