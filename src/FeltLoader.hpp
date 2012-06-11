/*
 wdb

 Copyright (C) 2012 met.no

 Contact information:
 Norwegian Meteorological Institute
 Box 43 Blindern
 0313 OSLO
 NORWAY
 E-mail: wdb@met.no

 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 2 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 MA  02110-1301, USA
*/

#ifndef POINTFELTLOADER_H_
#define POINTFELTLOADER_H_

// project
#include "Loader.hpp"
#include "FileLoader.hpp"
#include "CmdLine.hpp"
#include "CfgFileReader.hpp"

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

    class FeltLoader : public FileLoader
    {
    public:
        FeltLoader(Loader& controller);
        ~FeltLoader();

    private:

        void loadInterpolated(const string& fileName);

        std::string dataProviderName(const felt::FeltField& field);
        std::string valueParameterName(const felt::FeltField & field);
        std::string valueParameterUnit(const felt::FeltField & field);
        void levelValues( std::vector<wdb::load::Level>& levels, const felt::FeltField& field);
    };

} } }  // end namespaces
#endif /*POINTFELTLOADER_H_*/
