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

#ifndef GRIBHANDLEREADERINTERFACE_H_
#define GRIBHANDLEREADERINTERFACE_H_

/**
 * @file
 * This file contains the definition of the GribHandleReaderInterface
 *
 * @see GribField
 */


// standard
#include <stdlib.h>
#include <string>

namespace wdb { namespace load { namespace point {


    /// Interface for grib_handle wrapper.
    class GribHandleReaderInterface
    {
    public:
        /// Destructor
        virtual ~GribHandleReaderInterface() {}

        /**
         * Get a long value from the grib_handle
         * @param	name	the attribute in the GRIB field
         * @return	value
         */
        virtual long getLong( const char * name ) =0;

        /**
         * Get a double value from the grib_handle
         * @param	name	the attribute in the GRIB field
         * @return	value
         */
        virtual double getDouble( const char * name ) =0;

        /**
         * Get a string value from the grib_handle
         * @param	name	the attribute in the GRIB field
         * @return	value
         */
        virtual std::string getString( const char * name ) =0;

        /**
         * Get the size of the value grid from the grib_handle
         * @return	the size of the value grid in number of doubles
         */
        virtual size_t getValuesSize( ) = 0;

        /**
         * Get the value grid from the grib_handle
         * @return	a pointer to an array of doubles
         */
        virtual double * getValues( ) = 0;
    };

} } }

#endif /*GRIBHANDLEREADERINTERFACE_H_*/
