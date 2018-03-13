/*
 * TODO: Copyright
 */

#include <gnuradio/top_block.h>
#include <gnuradio/blocks/vector_source_f.h>
#include <gnuradio/blocks/tag_debug.h>

#include <iostream>
#include <vector>

int main(int argc, char **argv)
{

    auto top = gr::make_top_block("test");

    std::vector<float> data = {1.0, 3.0, 4.0};

    gr::tag_t tag;
    tag.offset = 0;
    tag.key =   pmt::intern("samp_rate");
    tag.value = pmt::from_uint64(200);
    tag.srcid = pmt::intern("vector_source");

    std::vector<gr::tag_t> tags = {tag};

    auto source = gr::blocks::vector_source_f::make(data, false, 1, tags);

    auto sink = gr::blocks::tag_debug::make(sizeof(float), "mysink");
    sink->set_display(true);

    // connect and run
    top->connect(source, 0, sink, 0);
    top->run();

    std::cout << "number of tags: " << sink->num_tags() << "\n";

    return 0;
}
