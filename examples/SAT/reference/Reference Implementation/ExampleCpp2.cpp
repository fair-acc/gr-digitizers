#include <top_block.h>
#include <blocks/null_source.h>
#include <blocks/null_sink.h>

int main()
{
    gr_top_block_sptr tb = gr_make_top_block("topblock");
    tb->connect(gr_make_null_source(sizeof(gr_complex)),
gr_make_null_sink(sizeof(gr_complex)));
    tb->run();
}
