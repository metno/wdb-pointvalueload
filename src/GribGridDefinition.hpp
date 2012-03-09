/*
    wdb - weather and water data storage

    Copyright (C) 2007 met.no

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

#ifndef GRIBGRIDDEFINITION_H_
#define GRIBGRIDDEFINITION_H_

/**
 * @addtogroup loader
 * @{
 * @addtogroup gribload
 * @{
 */

/**
 * @file
 * This file contains the definition of the GribGridDefinition class.
 *
 * @see GribField
 */

// PROJECT INCLUDES
//

// SYSTEM INCLUDES
#include <vector>
#include <string>
#include <boost/noncopyable.hpp>

// FORWARD REFERENCES
struct grib_handle;
class WdbProjection;
class GridGeometry;

namespace wmo { namespace codeTable {

    /// Scan Modes defined in the WMO Code Tables for GRIB
    enum ScanMode
    {
        LeftUpperHorizontal = 0,
        // Octal 000 - west to east (left to right), north to south (top to bottom), adjacent i points
        LeftLowerHorizontal = 64
        // Octal 100 - west to east (left to right), north to south (top to bottom), adjacent i points
    };

} }

namespace wdb { namespace load { namespace point {

// FORWARD REFERENCES
class GribHandleReaderInterface;

/**
 * GribGridDefinition encapsulates the grid definition of a GRIB field.
 * It handles all the calculations required to identify a place definition
 * in the database.
 *
 * @see GribField
 */
class GribGridDefinition : boost::noncopyable
{
public:
        // LIFECYCLE
    /**
     * Constructor
     */
    explicit GribGridDefinition( GribHandleReaderInterface & reader);

    /**
     * Default Destructor
     */
    ~GribGridDefinition();

        // OPERATIONS

        // ACCESS

        /**
         * Return the scanning mode of the data in the grid
     * @return	scanmode of grid
     */
    wmo::codeTable::ScanMode getScanMode() const;

    /**
     * Set the scanning mode of the data in the grid
     * @param	mode		scanmode to set the grid to
     */
    void setScanMode( wmo::codeTable::ScanMode mode );

    // Projection Information
    /** Return the number of points on the X axis
     * @return	numberX
     */
    virtual int numberX() const;
    /** Return the number of points on the Y axis
     * @return	numberY
     */
    virtual int numberY() const;
    /** Return the increment distance between points on the X axis
     * @return	incrementX
     */
    virtual float incrementX() const;
    /** Return the increment distance between points on the Y axis
     * @return	incrementY
     */
    virtual float incrementY() const;
    /** Return the starting point on the X axis
     * @return	startX
     */
    virtual float startX() const;
    /** Return the starting point on the Y axis
     * @return	startY
     */
    virtual float startY() const;
    /** Return the PROJ definition of the Grid Definition
     * @return	PROJ.4 string
     */
        std::string getProjDefinition() const;
        /** Get the Geometry of the GRID
         * @return The WKT string
         */
    std::string getGeometry() const;

        // INQUIRY


private:


    std::string getEarthsFigure() const;
    std::string getEarthsSphericalFigure() const;
    std::string getEarthsOblateFigure(long factorToM) const;

    /*
     * Sets up the array information for a regular lat/long grid (equidistant cylindrical)
     */
    std::string regularLatLonProjDefinition() const;
    /*
     * Sets up the array information for a rotated lat/long grid (equidistant cylindrical)
     */
    std::string rotatedLatLonProjDefinition() const;
    /*
     * Sets up the array information for a rotated lat/long grid (equidistant cylindrical)
     */
    std::string lambertProjDefinition() const;

    /// Perform initial setup of object. Called by all constructors
    void setup();

    /// Grid Geomerty
    GridGeometry * geometry_;

    /// Wraps reading of grib_handle (in order to facilitate testing).
    GribHandleReaderInterface & gribHandleReader_;


    // Grid Types - WMO Code Table 6
    enum  grid_type {
        UNDEFINED_GRID,
        REGULAR_LONLAT,
        ROTATED_LONLAT,
        REGULAR_GAUSSIAN,
        LAMBERT
    };
    grid_type getGridType() const;

};

} } }

#endif // GRIBGRIDDEFINITION_H_
