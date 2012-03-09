#-----------------------------------------------------------------------------
# WDB pointLoad Component
#-----------------------------------------------------------------------------

pointLoad_SOURCES =  src/main.cpp \
                     src/CfgFileReader.cpp \               
                     src/CmdLine.cpp \
                     src/DBConnection.cpp \  
                     src/FeltLoader.cpp \ 
                     src/GribField.cpp \  
                     src/GribFile.cpp \ 
                     src/GribGridDefinition.cpp \
                     src/GribHandleReader.cpp \ 
                     src/GribHandleReaderInterface.cpp \
                     src/GribLoader.cpp \
                     src/Loader.cpp \
                     src/WriteFloat.hpp \
                     src/CfgFileReader.h \  
                     src/CmdLine.h \
                     src/DBConnection.hpp \
                     src/FeltLoader.h \
                     src/GribField.hpp \
                     src/GribFile.hpp \
                     src/GribGridDefinition.hpp \
                     src/GribHandleReader.hpp \
                     src/GribHandleReaderInterface.hpp \
                     src/GribLoader.hpp \
                     src/Loader.hpp 
                     src/WciSession.hpp
	
EXTRA_DIST +=		src/src.mk
