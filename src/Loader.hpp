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

#ifndef POINTLOADER_H_
#define POINTLOADER_H_

// project
#include "CmdLine.hpp"

// libfimex
#include <fimex/CDMInterpolator.h>

// boost
#include <boost/shared_ptr.hpp>
#include <boost/shared_array.hpp>

// std
#include <set>
#include <vector>
#include <string>

using namespace std;

namespace wdb { namespace load { namespace point {

    class FeltLoader;
    class GribLoader;
    class NetCDFLoader;

    class Loader
    {
    public:
        Loader(const CmdLine& cmdLine);
        ~Loader();

        void load();

        const CmdLine& options() { return options_; }
        boost::shared_ptr<MetNoFimex::CDMReader> cdmTemplate() { return cdmTemplate_; }
        const vector<string>& placenames() { return placenames_; }
        const set<string>& stations2load() { return ids2load_; }
        const size_t interpolatemethod() { return interpolateMethod_; }
    private:

        bool openTemplateCDM(const std::string& fileName);
        bool extractBounds();
        bool extractPointIds();

        const CmdLine& options_;

        size_t interpolateMethod_;

        // CDMReader for template used in interpolation
        boost::shared_ptr<MetNoFimex::CDMReader> cdmTemplate_;

        // point ids found in cdm template
        set<string>                       ids2load_;
        vector<string>                    placenames_;
        boost::shared_array<unsigned int> pointids_;

        double northBound_;
        double southBound_;
        double westBound_;
        double eastBound_;

        boost::shared_ptr<FeltLoader>   felt_;
        boost::shared_ptr<GribLoader>   grib_;
        boost::shared_ptr<NetCDFLoader> netcdf_;
    };

} } } // end namespaces

#endif // POINTLOADER_HPP
