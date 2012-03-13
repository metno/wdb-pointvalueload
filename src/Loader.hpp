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
#include "WdbConnection.hpp"


namespace wdb { namespace load { namespace point {

    class FeltLoader;
    class GribLoader;

    class Loader
    {
    public:
        Loader(const CmdLine& cmdLine);
        ~Loader();

        void load();

    private:
        const CmdLine& options_;
        WdbConnection wdbConnection_;
        boost::shared_ptr<FeltLoader> felt_;
        boost::shared_ptr<GribLoader> grib_;
    };

} } } // end namespaces

#endif // POINTLOADER_HPP
