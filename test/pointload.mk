TESTS = pointloadTest

check_PROGRAMS = pointloadTest

pointloadTest_SOURCES = \
         test/testLoading.cpp

pointloadTest_CPPFLAGS = \
        $(AM_CPPFLAGS) \
        $(CPPFLAGS) \
        -I$(top_srcdir)/src \
        $(BOOST_CPPFLAGS)

pointloadTest_LDADD = \
          $(pointLoad_LDADD) \
          $(BOOST_UNIT_TEST_FRAMEWORK_LIB)
