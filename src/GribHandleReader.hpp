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

#ifndef GRIBHANDLEREADER_H_
#define GRIBHANDLEREADER_H_

/**
 * @file
 * This file contains the definition of the GribHandleReaderInterface
 *
 * @see GribField
 */

#include "GribHandleReaderInterface.hpp"

// FORWARD REFERENCES
class WdbProjection;
class GridGeometry;
struct grib_handle;

namespace wdb { namespace load { namespace point {

    /**
      * Wraps reading of grib_handle. This class exists to ease testing of other
      * grib-related classes.
      */
    class GribHandleReader : public GribHandleReaderInterface
    {
    public:
        /** Constructor
         * @param	gribHandle		The handle to the GRIB field
         */
        GribHandleReader(grib_handle * gribHandle);
        /// Destructor
        virtual ~GribHandleReader();
        /** Get a long value from the grib_handle
         * @param	name	the attribute in the GRIB field
         * @return	value
         */
        virtual long getLong( const char * name );
        /** Get a double value from the grib_handle
         * @param	name	the attribute in the GRIB field
         * @return	value
         */
        virtual double getDouble( const char * name );
        /**
         * Get a string value from the grib_handle
         * @param	name	the attribute in the GRIB field
         * @return	value
         */
        virtual std::string getString( const char * name );
        /** Get the size of the value grid from the grib_handle
         * @return	the size of the value grid in number of doubles
         */
        virtual size_t getValuesSize( );
        /** Get the value grid from the grib_handle
         * @return	a pointer to an array of doubles
         */
        virtual double * getValues( );

    private:
        /** Check the return code of a GRIB API call for errors
         * @param	returnCode		The return code of the GRIB API call
         * @param	variable		The variable name that was given the in the GRIB API call
         * @throws runtime_error
         */
        void errorCheck( int returnCode, const char * variable );

        /// The GRIB handle
        grib_handle * gribHandle_;
    };

} } }

#endif /*GRIBHANDLEREADER_H_*/
