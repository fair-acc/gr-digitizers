#ifndef _QA_EDGE_TRIGGER_H_
#define _QA_EDGE_TRIGGER_H_

#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/TestCase.h>

namespace gr::digitizers {

class qa_edge_trigger : public CppUnit::TestCase {
public:
    CPPUNIT_TEST_SUITE(qa_edge_trigger);
    CPPUNIT_TEST(decode);
    CPPUNIT_TEST(encode_decode);
    CPPUNIT_TEST_SUITE_END();

private:
    void decode();
    void encode_decode();
};

} /* namespace gr::digitizers */

#endif /* _QA_EDGE_TRIGGER_H_ */
