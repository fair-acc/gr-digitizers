#ifndef _QA_BLOCK_SCALING_OFFSET_H_
#define _QA_BLOCK_SCALING_OFFSET_H_

#include <cppunit/TestCase.h>
#include <cppunit/extensions/HelperMacros.h>

namespace gr::digitizers {

class qa_block_scaling_offset : public CppUnit::TestCase
{
public:
    CPPUNIT_TEST_SUITE(qa_block_scaling_offset);
    CPPUNIT_TEST(scale_and_offset);
    CPPUNIT_TEST_SUITE_END();

private:
    void scale_and_offset();
};

} /* namespace gr::digitizers */

#endif /* _QA_BLOCK_SCALING_OFFSET_H_ */
