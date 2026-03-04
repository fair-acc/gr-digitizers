#ifndef GR_DIGITIZERS_OPENCMWSOURCE_HPP
#define GR_DIGITIZERS_OPENCMWSOURCE_HPP

#include <Client.hpp>
#include <ClientContext.hpp>
#include <RestClient.hpp>
#include <URI.hpp>
#include <fair/opencmw/cmwlight/CmwLightClient.hpp>
#include <gnuradio-4.0/Block.hpp>
#include <gnuradio-4.0/BlockRegistry.hpp>

namespace fair::opencmw {
static ::opencmw::client::ClientContext initGlobalClientContext(::opencmw::zmq::Context& zctx) {
    using namespace ::opencmw::client;
    std::string nameserver = "http://localhost:7500";
    if (const auto ptr = std::getenv("CMW_NAMESERVER"); ptr != nullptr) {
        nameserver = std::string{ptr};
    }
    std::vector<std::unique_ptr<ClientBase>> clients{};
    clients.emplace_back(std::make_unique<RestClient>());
    clients.emplace_back(std::make_unique<MDClientCtx>(zctx, 100ms, "OpenCmwMajordomoClient"));
    clients.emplace_back(std::make_unique<cmwlight::CmwLightClientCtx>(zctx, nameserver, 100ms, "OpenCmwRda3Client"));
    return ClientContext{std::move(clients)};
}

static ::opencmw::client::ClientContext& getGloablClientContext() {
    static ::opencmw::zmq::Context          zctx{};
    static ::opencmw::client::ClientContext clientContext = initGlobalClientContext(zctx);
    return clientContext;
}

struct OpenCmwSource : gr::Block<OpenCmwSource> {
    using FloatLimits = gr::Limits<std::numeric_limits<float>::lowest(), std::numeric_limits<float>::max()>;
    gr::PortOut<gr::pmt::Value> out;

    gr::Annotated<std::string, "url", gr::Visible, gr::Doc<"URL of the resource to subscribe to">>                     url;
    gr::Annotated<std::string, "signal name", gr::Visible, gr::Doc<"Identifier for the signal">>                       signal_name;
    gr::Annotated<std::string, "signal quantity", gr::Visible, gr::Doc<"Physical quantity represented by the signal">> signal_quantity;
    gr::Annotated<std::string, "signal unit", gr::Visible, gr::Doc<"Unit of measurement for the signal values">>       signal_unit;
    gr::Annotated<float, "signal min", gr::Doc<"Minimum expected value for the signal">, FloatLimits>                  signal_min        = std::numeric_limits<float>::lowest();
    gr::Annotated<float, "signal max", gr::Doc<"Maximum expected value for the signal">, FloatLimits>                  signal_max        = std::numeric_limits<float>::max();
    gr::Annotated<bool, "verbose console", gr::Doc<"For debugging">>                                                   verbose_console   = false;
    gr::Annotated<float, "reconnect timeout", gr::Doc<"reconnect timeout in sec">>                                     reconnect_timeout = 5.f;
    gr::Annotated<std::string, "nameserver", gr::Doc<"Nameserver to use for device lookup">>                           nameserver        = "http://localhost:7500";

    GR_MAKE_REFLECTABLE(OpenCmwSource, out, url, signal_name, signal_unit, signal_quantity, signal_min, signal_max, verbose_console, reconnect_timeout);

    std::mutex                 mutex;
    std::queue<gr::pmt::Value> updates;

    void start() {
        if (verbose_console) {
            std::println("OpenCmwSource: subscribe to {}", url);
        }
        auto& clientContext = getGloablClientContext();
        clientContext.subscribe(::opencmw::URI(url), [this](const ::opencmw::mdp::Message& response) {
            if (verbose_console) {
                std::println("OpenCmwSource: received update");
            }
            auto res    = gr::property_map{};
            auto header = gr::property_map{};
            header.try_emplace("url", std::pmr::string{url});
            if (response.error.empty()) {
                using AcquisitionMap = std::map<std::string, std::variant<bool, float, double, int8_t, int16_t, int32_t, int64_t, std::string, std::vector<bool>, std::vector<float>, std::vector<double>, std::vector<int8_t>, std::vector<int16_t>, std::vector<int32_t>, std::vector<int64_t>, std::vector<std::string>>>;
                ::opencmw::IoBuffer             buffer(response.data);
                AcquisitionMap                  acqMap{};
                ::opencmw::DeserialiserInfo     info;
                ::opencmw::FieldDescriptionLong field;
                ::opencmw::FieldHeaderReader<::opencmw::CmwLight>::get<::opencmw::ProtocolCheck::LENIENT>(buffer, info, field);
                ::opencmw::IoSerialiser<::opencmw::CmwLight, AcquisitionMap>::deserialise(buffer, field, acqMap);
                if (verbose_console) {
                    std::println("Deserialised subscription reply:\n  fields: {}\n  fieldTypes: {}\n  errors: {}\n  mapFields: {}", info.additionalFields | std::views::keys, info.additionalFields | std::views::values, info.exceptions | std::views::transform([](auto& e) { return e.what(); }), acqMap | std::views::keys);
                }
                auto val = gr::property_map{};
                for (auto& [key, value] : acqMap) {
                    std::visit([&](auto&& v) { val.insert(std::make_pair(key, gr::pmt::Value(v))); }, value);
                }
                res.try_emplace("data", val);
                auto errors = gr::pmt::Value{info.exceptions | std::views::transform([](auto& e) { return e.what(); }) | std::ranges::to<std::vector<std::string>>()};
                res.try_emplace("errors", errors);
            } else {
                if (verbose_console) {
                    std::println("error in subscription: {}", response.error);
                }
                auto errors = gr::pmt::Value{response.error};
                res.try_emplace("errors", errors);
            }
            res.try_emplace("header", header);
            std::lock_guard guard{mutex};
            updates.push(std::move(res));
            this->progress->incrementAndGet();
        });
    }

    void stop() {
        auto& clientContext = getGloablClientContext();
        if (verbose_console) {
            std::println("OpenCmwSource: unsubscribe from {}", url);
        }
        clientContext.unsubscribe(::opencmw::URI(url));
    }

    auto processBulk(gr::OutputSpanLike auto& output) noexcept {
        size_t          written = 0;
        std::lock_guard guard{mutex};
        while (written < output.size() && !updates.empty()) {
            if (verbose_console) {
                std::println("OpenCmwSource: publish update(s): {}", updates.front());
            }
            *(output.begin() + written) = updates.front();
            ++written;
            updates.pop();
        }
        output.publish(written);
        // todo: propagate metadata
        return written == 0UZ ? gr::work::Status::INSUFFICIENT_INPUT_ITEMS : gr::work::Status::OK;
    }
};

template<typename T>
struct ExtractFromMap : gr::Block<ExtractFromMap<T>> {
    gr::Annotated<std::string, "fieldname", gr::Visible, gr::Doc<"Field to extract and publish from the map">> fieldname;

    gr::PortIn<gr::pmt::Value> in;
    gr::PortOut<T>             out;

    GR_MAKE_REFLECTABLE(ExtractFromMap, in, out, fieldname);

    [[nodiscard]] constexpr gr::work::Status processBulk(gr::InputSpanLike auto& input, gr::OutputSpanLike auto& output) noexcept {
        std::size_t nIn  = 0;
        std::size_t nOut = 0;
        for (const auto& map : input) {
            if (const gr::pmt::Value::Map* dataPtr = map.template get_if<gr::pmt::Value::Map>(); dataPtr != nullptr && dataPtr->contains("data")) {
                if (const auto* mapPtr = dataPtr->at("data").get_if<gr::pmt::Value::Map>(); mapPtr != nullptr && mapPtr->contains(fieldname)) {
                    if (const T* valuePtr = mapPtr->at(std::pmr::string{fieldname.value}).get_if<T>(); valuePtr != nullptr) {
                        if (nOut >= output.size()) {
                            break;
                        }
                        output[nOut] = *valuePtr;
                        ++nOut;
                    } else if (const gr::Tensor<T>* tensorPtr = mapPtr->at(std::pmr::string{fieldname.value}).get_if<gr::Tensor<T>>(); tensorPtr != nullptr) {
                        if (nOut + tensorPtr->size() >= output.size()) {
                            break;
                        }
                        std::copy(tensorPtr->begin(), tensorPtr->end(), output.begin() + nOut);
                        nOut += tensorPtr->size();
                    } else {
                        std::println("field {1} exists, but has unsupported field type: {2}, supported: {0} or Tensor<{0}>", gr::meta::type_name<T>(), fieldname.value, magic_enum::enum_name<>(magic_enum::enum_cast<gr::pmt::Value::ValueType>(mapPtr->at(std::pmr::string{fieldname.value})._value_type).value_or(gr::pmt::Value::ValueType::Monostate)));
                    }
                }
            }
            ++nIn;
        }
        output.publish(nOut);
        input.consume(nIn);
        return nOut > 0 ? gr::work::Status::OK : (nIn > 0 ? gr::work::Status::INSUFFICIENT_OUTPUT_ITEMS : gr::work::Status::INSUFFICIENT_INPUT_ITEMS);
    }
};

} // namespace fair::opencmw

inline auto registerRemoteStreamSource = ::gr::registerBlock<fair::opencmw::OpenCmwSource>(::gr::globalBlockRegistry());

#endif // GR_DIGITIZERS_OPENCMWSOURCE_HPP
