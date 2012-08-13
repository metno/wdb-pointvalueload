TESTS = fastloadTest

check_PROGRAMS = fastloadTest

fastloadTest_SOURCES = \
        test/InputDataTest.cpp \
		test/timetypetest.cpp
				 

fastloadTest_CPPFLAGS = \
		         $(AM_CPPFLAGS) \
		         $(wdb_fastload_CPPFLAGS) \
	             -I$(top_srcdir)/src \
                 $(BOOST_CPPFLAGS)

fastloadTest_LDADD = \
					$(wdb_fastload_LDADD) \
					$(BOOST_UNIT_TEST_FRAMEWORK_LIB)
