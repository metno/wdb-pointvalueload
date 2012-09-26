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

#ifndef FILELOADER_HPP
#define FILELOADER_HPP

// project
#include "Loader.hpp"
#include "CmdLine.hpp"
#include "CfgFileReader.hpp"

#include <boost/shared_ptr.hpp>
#include <boost/shared_array.hpp>

using namespace std;

namespace MetNoFimex {
    class CDMReader;
}

namespace wdb { namespace load { namespace point {

    /*
     * Struct that holds various metadata for parameters that are to be extracted from field data files
     * i,e. how should parameter be named in WDB, what units to be used, which levels to be loaded ...
     * The metada is given through various config files
     **/
    struct EntryToLoad {
        string cdmName_;
        string standardName_;
        string wdbName_;
        string wdbUnit_;
        string wdbDataProvider_;
        string wdbLevelName_;
        set<double> wdbLevels_;
        size_t cdmXDimLength_;
        size_t cdmYDimLength_;
        string cdmLevelName_;
        vector<string> cdmShape_;
        boost::shared_array<double> cdmData_;
    };

    /*
     * The base class for point loaders
     * Describes the main steps how to load points
     * and implements common infrastructre
     *
     * ATM there are 3 spercialized classes:
     * FeltLoader, GribLoader and NetCDFLoader
     **/
    class FileLoader
    {
    public:
        FileLoader(Loader& controller);
        virtual ~FileLoader( );


        // Extract data for wdb-fastload consumption
        void load(const string& fileName);

    protected:
        /*
         * Opens/reads configuration files with the
         * metadata about parameters to be loaded
         * metadata: wdb name, wdb unit, wdb levels ...
         **/
        virtual void setup();

        /*
         * Iterate through each record in the data files
         * and make a list of EntryToLoad items that will
         * represent parameters to be extracetd
         *
         * Each file type konws how to to this
         **/
        virtual void loadInterpolated(const string& fileName) = 0;

        /* Uses fimex to create CDMReader object
         *
         * Some file types require xml
         * configuration file (felt, grib)
         * and some not (netcdf)
         *
         * Overload method as needed
         **/
        virtual bool openCDM(const std::string& file) = 0;

        /*
         * iterates through each EntryToLoad item (see loadIntepolated )
         * reads the data from a fimex CDMReader object (see openCDM )
         * and assembles the data line to be sent to wdb-fastload
         *
         * The lines are written for each lat/lon, each time step,
         * each ensemble member and only for selected levels
         **/
        virtual void loadEntries();

        /*
         * If requested it will extract the u and v wind components
         * to calculate wind_speed and wind_direction as prescribed
         * by met.no scientis
         * To make this happen, data must have u and w components
         * and fimex.process.rotateVectorToLatLonX, fimex.process.rotateVectorToLatLonY
         * command options have to hold fimex points for the u and w components
         **/
        virtual void loadWindEntries();

        // Extract time axis and unique forecast time from the data file
        virtual bool timeFromCDM();

        // Recalculate wind_speed and wind_direction
        // based on u and v wind components
        virtual bool processCDM();

        /*
         * apply fimex template interpolation to the data
         * by using the created CDMReader (look openCDM )
         **/
         virtual bool interpolateCDM();

        /*
         * Read the units.conf file to find what units
         * should be used when inserting the data into wdb
         **/
        void readUnit(const string& unitname, float& coeff, float& term);

        // access to the command line options
        const CmdLine& options() { return controller_.options(); }

        boost::shared_ptr<MetNoFimex::CDMReader> cdmTemplate() { return controller_.cdmTemplate(); }

        // the list of parameters (+ metadata) to be extracted
        std::map<std::string, EntryToLoad>& entries2load()  {return entries2Load_; }

        // fimex can hel to obtain rotated wind componenets
        // the parameter names are placed as strings in
        // u and v component vector
        vector<string>& uwinds() { return uWinds_; }
        vector<string>& vwinds() { return vWinds_; }

        Loader& controller_;

        // CDMReader for data that will be interpolated
        boost::shared_ptr<MetNoFimex::CDMReader> cdmData_;

        // the values from time axis as strings
        vector<string> times_;
        const vector<string>& times() { return times_;}

        // basic conf files - assist in parameter to wdb mapping
        /// Conversion Hash Map - Dataprovider Name
        CfgFileReader point2DataProviderName_;
        /// Conversion Hash Map - Value Parameter
        CfgFileReader point2ValueParameter_;
        /// Conversion Hash Map - Level Parameter
        CfgFileReader point2LevelParameter_;
        /// Conversion Hash Map - Level Additions
        CfgFileReader point2LevelAdditions_;
        CfgFileReader point2Units_;

        vector<string> uWinds_;
        vector<string> vWinds_;

        // the list of parameters (and metadata) to be extracted
        map<string, EntryToLoad> entries2Load_;
    };


    // helper factory class - should make easier to add new specialized classes
    // TODO: move it to own class
    class FileLoaderFactory
    {
    public:
        static FileLoader *createFileLoader(const std::string &type, class Loader& controller);
    };


} } } // end namespaces

#endif // FILELOADER_HPP
