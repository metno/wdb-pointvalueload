#-----------------------------------------------------------------------------
# WDB pointLoad Component
#-----------------------------------------------------------------------------

pointLoad_SOURCES =  src/main.cpp \
                     src/CmdLine.cpp \
	                 src/CfgFileReader.cpp \
		             src/FeltLoader.cpp \
			         src/Loader.cpp \
				     src/GribLoader.cpp \
					 src/GribField.cpp \
					 src/GribGridDefinition.cpp \
					 src/GribFile.cpp \
					 src/GribHandleReader.cpp \
					 src/WdbConnection.cpp \
					 src/CmdLine.h \
					 src/CfgFileReader.h \
				     src/FeltLoader.h \
					 src/Loader.hpp \
					 src/GribLoader.hpp \
				     src/GribField.hpp \
					 src/GribGridDefinition.hpp \
					 src/GribFile.hpp \
					 src/GribHandleReader.hpp \
					 src/WciTransactors.hpp \
					 src/WdbConnection.hpp

EXTRA_DIST +=		src/src.mk
