TESTS = pointValueLoadTest

check_PROGRAMS = pointValueLoadTest

pointValueLoadTest_SOURCES = \
         test/testLoading.cpp

pointValueLoadTest_CPPFLAGS = \
        $(AM_CPPFLAGS) \
        $(CPPFLAGS) \
        -I$(top_srcdir)/src \
        $(BOOST_CPPFLAGS) \
        -DSRCDIR=\"@top_srcdir@\"

pointValueLoadTest_LDADD = \
          $(pointValueLoad_LDADD) \
          $(BOOST_UNIT_TEST_FRAMEWORK_LIB)
