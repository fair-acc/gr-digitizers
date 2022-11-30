#ifndef _QA_STREAM_TO_VECTOR_OVERLAY_FF_H_
#define _QA_STREAM_TO_VECTOR_OVERLAY_FF_H_

#include <cppunit/TestCase.h>
#include <cppunit/extensions/HelperMacros.h>

namespace gr::digitizers {

class qa_stream_to_vector_overlay : public CppUnit::TestCase
{
public:
    CPPUNIT_TEST_SUITE(qa_stream_to_vector_overlay);
    CPPUNIT_TEST(t1);
    CPPUNIT_TEST_SUITE_END();

private:
    void t1();
};

} /* namespace gr::digitizers */

#endif /* _QA_STREAM_TO_VECTOR_OVERLAY_FF_H_ */
