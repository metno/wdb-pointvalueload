/*
    wdb - weather and water data storage

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


#ifndef POINTCFGXMLFILEREADER_H_
#define POINTCFGXMLFILEREADER_H_

// project
#include "CfgXmlElements.hpp"

// wdb
#include <wdb/WdbLevel.h>

// boost
#include <boost/filesystem.hpp>

// std
#include <list>
#include <string>
#include <iosfwd>

using namespace std;

extern "C"
{
    typedef struct _xmlXPathContext xmlXPathContext;
    typedef xmlXPathContext *xmlXPathContextPtr;
}

namespace wdb { namespace load { namespace point {
  class CfgXmlFileReader
    {
    public:
        CfgXmlFileReader(const boost::filesystem::path& configFile);
        ~CfgXmlFileReader();

        string dataProviderName4Netcdf(const string& variablename) const;
        string valueParameterName4Netcdf(const string& variablename) const;
        void levelValues4Netcdf(std::vector<wdb::load::Level>& levels, const string& variablename) const;

        string dataProviderName4Felt(const string& producer, const string& gridarea) const;
        string valueParameterName4Felt(const string& parameter, const string& verticalcoordinate, const string& level1) const;
        string valueParameterUnits4Felt(const string& parameter, const string& verticalcoordinate, const string& level1) const;
        void levelValues4Felt(vector<wdb::load::Level>& levels, const string& parameter, const string& verticalcoordinate, const string& level1, const string& level2) const;

        string dataProviderName4Grib(const string& center, const string& process) const;
        string valueParameterName4Grib1(const string& center,
                                        const string& codetable2version,
                                        const string& parameterid,
                                        const string& timerange,
                                        const string& thresholdindicator,
                                        const string& thresholdlower,
                                        const string& thresholdupper,
                                        const string& thresholdscale) const;
        string valueParameterUnits4Grib1(const string& center,
                                         const string& codetable2version,
                                         const string& parameterid,
                                         const string& timerange,
                                         const string& thresholdindicator,
                                         const string& thresholdlower,
                                         const string& thresholdupper,
                                         const string& thresholdscale) const;
        void levelValues4Grib1(vector<wdb::load::Level>& levels,
                               const string& center,
                               const string& codetable2version,
                               const string& parameterid,
                               const string& timerange,
                               const string& thresholdindicator,
                               const string& thresholdlower,
                               const string& thresholdupper,
                               const string& thresholdscale,
                               const string& typeoflevel,
                               const string& level) const;

        string valueParameterName4Grib2(const string& parameterid) const;
        string valueParameterUnits4Grib2(const string& parameterid) const;
        void levelValues4Grib2(vector<wdb::load::Level>& levels, const string& parameterid, const string& typeoflevel, const string& level) const;
    private:
        string fileName_;
        dataprovider_multimap dataproviders_;
        elements_multimap netcdfloads_;
        felt_multimap feltfloads_;
        grib1_multimap grib1loads_;
        grib2_multimap grib2loads_;

        void init_(xmlXPathContextPtr context);
    };

} } }// end namespaces

#endif /* POINTCFGXMLFILEREADER_H_ */
