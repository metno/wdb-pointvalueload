/*
 gribPointLoad

 Copyright (C) 2012 met.no

 Contact information:
 Norwegian Meteorological Institute
 Box 43 Blindern
 0313 OSLO
 NORWAY
 E-mail: post@met.no

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

#ifndef GRIBFILE_HPP
#define GRIBFILE_HPP

// boost
#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>

// std
#include <string>

extern "C"
{
    struct grib_handle;
}

namespace wdb { namespace load { namespace point {

    class GribField;

    /**
      * Simple way of reading a GRIB file. Each field in the file may be retrieved by calling next()
      */
    class GribFile : boost::noncopyable
    {
    public:
        /**
         * @throws exception if unable to open file
         */
        GribFile(const std::string & fileName);
        ~GribFile();

        /// Return value from next() method
        typedef boost::shared_ptr<GribField> Field;

        /**
         * Get the next field in the GRIB file, or 0 if none are left
         */
        Field next();

        /// Get the name of the file being read
        const std::string & fileName() const { return fileName_; }

    private:
        grib_handle * getNextHandle_();

        const std::string fileName_;
        FILE * gribFile_;
};

} } }// end namespace

#endif // GRIBFILE_HPP
