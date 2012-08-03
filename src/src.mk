#-----------------------------------------------------------------------------
# WDB pointLoad Component
#-----------------------------------------------------------------------------

pointLoad_SOURCES =  src/main.cpp \
                     src/CmdLine.cpp \
	                 src/CfgFileReader.cpp \
					 src/CfgXmlFileReader.cpp \
					 src/CfgXmlElements.cpp \
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
					 src/CfgXmlFileReader.hpp \
					 src/CfgXmlElements.hpp \
				     src/FeltLoader.hpp \
					 src/Loader.hpp \
					 src/GribLoader.hpp \
				     src/GribField.hpp \
					 src/GribGridDefinition.hpp \
					 src/GribFile.hpp \
					 src/GribHandleReader.hpp \
                     src/FileLoader.hpp \
					 src/NetCDFLoader.hpp

EXTRA_DIST +=		src/src.mk
