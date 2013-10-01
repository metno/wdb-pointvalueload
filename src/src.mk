#-----------------------------------------------------------------------------
# WDB pointLoad Component
#-----------------------------------------------------------------------------

SOURCE = src/CmdLine.cpp \
	                 src/CfgFileReader.cpp \
		             src/FeltLoader.cpp \
			         src/Loader.cpp \
				     src/GribLoader.cpp \
					 src/GribField.cpp \
					 src/GribGridDefinition.cpp \
					 src/GribFile.cpp \
					 src/GribHandleReader.cpp \
                     src/FileLoader.cpp \
					 src/NetCDFLoader.cpp \
					 src/CmdLine.hpp \
					 src/CfgFileReader.hpp \
				     src/FeltLoader.hpp \
					 src/Loader.hpp \
					 src/GribLoader.hpp \
				     src/GribField.hpp \
					 src/GribGridDefinition.hpp \
					 src/GribFile.hpp \
					 src/GribHandleReader.hpp \
                     src/FileLoader.hpp \
					 src/NetCDFLoader.hpp

libpointvalueload_a_SOURCES += $(SOURCE)

pointValueLoad_SOURCES = \
                  src/main.cpp

EXTRA_DIST +=		src/src.mk
