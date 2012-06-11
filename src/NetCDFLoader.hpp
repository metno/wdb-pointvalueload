#ifndef POINTNETCDFLOADER_HPP
#define POINTNETCDFLOADER_HPP

// project
#include "Loader.hpp"
#include "FileLoader.hpp"
#include "CmdLine.hpp"
#include "CfgFileReader.hpp"
#include "WdbConnection.hpp"

// wdb
#include <wdb/WdbLevel.h>
#include <WdbConfigFile.h>
#include <wdb/LoaderConfiguration.h>
#include <wdb/LoaderDatabaseConnection.h>

// libfelt
#include <felt/FeltFile.h>
#include <felt/FeltField.h>
#include <felt/FeltConstants.h>

// libfimex
#include <fimex/CDMReader.h>

// boost
#include <boost/shared_array.hpp>
#include <boost/date_time/posix_time/posix_time_types.hpp>

// std
#include <vector>
#include <tr1/unordered_map>

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

        void loadInterpolated(const string& fileName);

        void setup();
        string dataProviderName(const string& varname);
        string valueParameterName(const string& varname);
        string valueParameterUnit(const string& varname);
        void levelValues( std::vector<wdb::load::Level>& levels, const string& varname);
    };

} } }  // end namespaces

#endif // NETCDFLOADER_HPP
