#include "edge_trigger_receiver_cpu.h"
#include "edge_trigger_receiver_cpu_gen.h"

namespace gr::digitizers {

edge_trigger_receiver_cpu::edge_trigger_receiver_cpu(const block_args &args)
    : INHERITED_CONSTRUCTORS {
    udp::resolver           resolver(d_io_service);
    udp::resolver::query    query(udp::v4(), args.host, std::to_string(args.port));
    udp::resolver::iterator iter     = resolver.resolve(query);
    udp::endpoint           endpoint = *iter;
    d_udp_receive                    = std::make_unique<udp_receiver>(d_io_service, endpoint);
}

work_return_t edge_trigger_receiver_cpu::work(work_io &wio) {
    const auto  noutput_items = wio.outputs()[0].n_items;
    auto        out           = wio.outputs()[0].items<float>();

    std::string message       = d_udp_receive->get_msg();
    if (message != "") {
        edge_detect_t edge;
        if (decode_edge_detect(message, edge)) {
            tag_t edge_tag = make_edge_detect_tag(edge);
            edge_tag.set_offset(wio.outputs()[0].nitems_written());
            wio.outputs()[0].add_tag(edge_tag);
        } else {
            d_logger->error("Decoding UDP datagram failed: {}", message);
        }
    }

    memset(out, 0, noutput_items * sizeof(float));

    wio.produce_each(noutput_items);
    return work_return_t::OK;
}

} // namespace gr::digitizers
