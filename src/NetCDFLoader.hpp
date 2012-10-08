#ifndef POINTNETCDFLOADER_HPP
#define POINTNETCDFLOADER_HPP

// project
#include "FileLoader.hpp"

// wdb
#include <wdb/WdbLevel.h>
#include <WdbConfigFile.h>
#include <wdb/LoaderConfiguration.h>
#include <wdb/LoaderDatabaseConnection.h>

using namespace std;

namespace MetNoFimex {
    class CDMReader;
}

namespace wdb { namespace load { namespace point {

    class Loader;

    class NetCDFLoader : public FileLoader
    {
    public:
        NetCDFLoader(Loader& controller);
        ~NetCDFLoader();

    private:

        // open configuration files specific to NetCDF
        void setup();

        // read - create CDMReader - for input file
        bool openCDM(const string& fileName);

        // iterate input file and gather parameters to be loaded
        void loadInterpolated(const string& fileName);

        // get the metadata values from config files - for each parameter
        ////////////////////////////////////////////////////////////////////////////////
        string dataProviderName(const string& varname);
        string valueParameterName(const string& varname);
        string valueParameterUnit(const string& varname);
        void levelValues( std::vector<wdb::load::Level>& levels, const string& varname);
        ////////////////////////////////////////////////////////////////////////////////
    };

} } }  // end namespaces

#endif // NETCDFLOADER_HPP
