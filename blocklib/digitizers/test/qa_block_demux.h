#ifndef _QA_BLOCK_DEMUX_H_
#define _QA_BLOCK_DEMUX_H_

#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/TestCase.h>

namespace gr::digitizers {

class qa_block_demux : public CppUnit::TestCase {
public:
    CPPUNIT_TEST_SUITE(qa_block_demux);
    // TODO(PORT): this test was disabled with the comment "FIXME: Need to fix this block before usage"
    //  it passes though, so check what's wrong with the block
    CPPUNIT_TEST(passes_only_desired);
    CPPUNIT_TEST_SUITE_END();

private:
    void passes_only_desired();
};

} // namespace gr::digitizers

#endif /* _QA_BLOCK_DEMUX_H_ */
