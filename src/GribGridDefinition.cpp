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


/**
 * @file
 * Implementation of GribGridDefinition class
 */
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

// project
#include "GribGridDefinition.hpp"
#include "GribHandleReader.hpp"

// wdb
#include <wdbLogHandler.h>
#include <WdbProjection.h>
#include <GridGeometry.h>
// SYSTEM INCLUDES
#include <boost/assign/list_of.hpp>
#include <sstream>
#include <iostream>
#include <string>
#include <proj_api.h>

using namespace std;
using namespace wmo::codeTable;
using namespace wdb;

namespace
{

typedef std::map<wmo::codeTable::ScanMode, GridGeometry::Orientation> Wmo2InternalScanMode;

    const Wmo2InternalScanMode orientation =
            boost::assign::map_list_of
                (wmo::codeTable::LeftUpperHorizontal, GridGeometry::LeftUpperHorizontal)
                (wmo::codeTable::LeftLowerHorizontal, GridGeometry::LeftLowerHorizontal);

    /**
      * convert a type (i.e. int, float) to string representation
      */
    template<typename T>
    std::string type2string(T in)
    {
        std::ostringstream buffer;
        buffer << in;
        return buffer.str();
    }

    /**
      * normalize Longitude to be within [-180:180]
      * @param in longitude in degree
      * @return longitude in degree within [-180:180]
      */
    template<typename T>
    T normalizeLongitude180(T in)
    {
        while (in < -180) {
            in += 360;
        }
        while (in > 180) {
            in -= 360;
        }
        return in;
    }

}

namespace wdb { namespace load { namespace point {

void GribGridDefinition::setup()
{
    std::string sridProj = getProjDefinition();

    wmo::codeTable::ScanMode scanMode = (wmo::codeTable::ScanMode) gribHandleReader_.getLong("scanningMode");
    Wmo2InternalScanMode::const_iterator f = orientation.find(scanMode);
    if ( f == orientation.end() )
        throw std::runtime_error("Unrecognized scan mode");
    GridGeometry::Orientation o = f->second;

    long iNumber, jNumber;
    double iIncrement, jIncrement, startI, startJ;

        ostringstream errMsg;
    switch (getGridType()) {
    case REGULAR_LONLAT:
    case ROTATED_LONLAT:
        // X and Y size
        iNumber = gribHandleReader_.getLong("Ni");
                jNumber = gribHandleReader_.getLong("Nj");
                // X/Y increment
                iIncrement = gribHandleReader_.getDouble("iDirectionIncrementInDegrees");// * DEG_TO_RAD;
                if ( gribHandleReader_.getLong("iScansNegatively") )
                        iIncrement *= -1;
                jIncrement = gribHandleReader_.getDouble("jDirectionIncrementInDegrees");// * DEG_TO_RAD;
                if ( ! gribHandleReader_.getLong("jScansPositively") )
                        jIncrement *= -1;
                // Start X/Y
                startI = gribHandleReader_.getDouble("longitudeOfFirstGridPointInDegrees");// * DEG_TO_RAD;
                startJ = gribHandleReader_.getDouble("latitudeOfFirstGridPointInDegrees");// * DEG_TO_RAD;
                break;
    case LAMBERT:
        // X and Y size
        iNumber = gribHandleReader_.getLong("Nx");
                jNumber = gribHandleReader_.getLong("Ny");
                // X/Y increment
                iIncrement = gribHandleReader_.getDouble("DxInMetres");// * DEG_TO_RAD;
                if ( gribHandleReader_.getLong("iScansNegatively") )
                        iIncrement *= -1;
                jIncrement = gribHandleReader_.getDouble("DyInMetres");// * DEG_TO_RAD;
                if ( ! gribHandleReader_.getLong("jScansPositively") )
                        jIncrement *= -1;
                // Start X/Y
                startI = gribHandleReader_.getDouble("longitudeOfFirstGridPointInDegrees");// * DEG_TO_RAD;
                startJ = gribHandleReader_.getDouble("latitudeOfFirstGridPointInDegrees");// * DEG_TO_RAD;
                break;
    default:
        errMsg << "Cannot specify the grid geometry.";
        throw std::runtime_error( errMsg.str() );
        break;
    }
    WDB_LOG & log = WDB_LOG::getInstance( "wdb.gribLoad.gribGridDefinition" );
    log.debugStream() << "Creating geometry with ("
                                          << iNumber << ", "
                                          << jNumber << ", "
                                          << iIncrement << ", "
                                          << jIncrement << ", "
                                          << startI << ", "
                                          << startJ << ")";
    geometry_ = new GridGeometry(sridProj, o, iNumber, jNumber, iIncrement, jIncrement, startI, startJ );
}

GribGridDefinition::GribGridDefinition(GribHandleReader& reader)
        : geometry_(0), gribHandleReader_(reader)
{
        setup();
}

GribGridDefinition::~GribGridDefinition()
{
        delete geometry_;
}

// Operations
//---------------------------------------------------------------------------


// Access
//---------------------------------------------------------------------------

int
GribGridDefinition::numberX() const
{
    return geometry_->xNumber_;
};

int
GribGridDefinition::numberY() const
{
    return geometry_->yNumber_;
};

float
GribGridDefinition::incrementX() const
{
        return geometry_->xIncrement_;
};

float
GribGridDefinition::incrementY() const
{
        return geometry_->yIncrement_;
};

float
GribGridDefinition::startX() const
{
        return geometry_->startX_;
};

float
GribGridDefinition::startY() const
{
        return geometry_->startY_;
};

std::string GribGridDefinition::getGeometry() const
{
        return geometry_->wktRepresentation();
}

void
GribGridDefinition::setScanMode( wmo::codeTable::ScanMode mode )
{
        Wmo2InternalScanMode::const_iterator find = orientation.find(mode);
        if ( find == orientation.end() )
        {
        throw std::runtime_error( "Unsupported field conversion in GribGridDefinition" );
        }
        geometry_->setOrientation(find->second);
}

wmo::codeTable::ScanMode GribGridDefinition::getScanMode() const
{
        for ( Wmo2InternalScanMode::const_iterator it = orientation.begin(); it != orientation.end(); ++ it )
                if ( it->second == geometry_->orientation() )
                        return it->first;

        // If we get here, we could not understand the scan mode:
        ostringstream errMsg;
        errMsg << "Scan mode " << geometry_->orientation() << " is unknown to the grid loader";
        throw std::runtime_error( errMsg.str() );
}

GribGridDefinition::grid_type GribGridDefinition::getGridType() const
{
        std::string gridType = gribHandleReader_.getString("gridType");
    WDB_LOG & log = WDB_LOG::getInstance( "wdb.gribLoad.gribGridDefinition" );
    log.debugStream() << "GridType is " << gridType;

    if (gridType == "regular_ll")
        return REGULAR_LONLAT;

    if (gridType == "rotated_ll")
        return ROTATED_LONLAT;

    if (gridType == "lambert")
        return LAMBERT;

    if (gridType == "regular_gg")
        return REGULAR_GAUSSIAN;

    log.warnStream() << "Could not identify gridType: " << gridType;
    return UNDEFINED_GRID;
}

std::string GribGridDefinition::getProjDefinition() const
{
    ostringstream errMsg;
    switch (getGridType()) {
    case REGULAR_LONLAT:
        return regularLatLonProjDefinition();
    case ROTATED_LONLAT:
        return rotatedLatLonProjDefinition();
    case LAMBERT:
        return lambertProjDefinition();
    case UNDEFINED_GRID:
        errMsg << "Undefined grid type in GRIB file.";
        throw std::runtime_error( errMsg.str() );
        break;
    default:
        errMsg << "Unsupported grid type in GRIB file.";
        throw std::runtime_error( errMsg.str() );
    }

    return errMsg.str();
}

std::string GribGridDefinition::getEarthsOblateFigure(long factorToM) const
{
    long majorFactor = gribHandleReader_.getLong("scaleFactorOfMajorAxisOfOblateSpheroidEarth");
    long minorFactor = gribHandleReader_.getLong("scaleFactorOfMinorAxisOfOblateSpheroidEarth");

    double majorValue = gribHandleReader_.getDouble("scaledValueOfMajorAxisOfOblateSpheroidEarth");
    double minorValue = gribHandleReader_.getDouble("scaledValueOfMinorAxisOfOblateSpheroidEarth");


    while (majorFactor > 0) {majorValue *= 10; majorFactor--;}
    while (majorFactor < 0) {majorValue /= 10; majorFactor++;}
    // transfer km to m
    majorFactor *= factorToM;

    while (minorFactor > 0) {minorValue *= 10; minorFactor--;}
    while (minorFactor < 0) {minorValue /= 10; minorFactor++;}
    // transfer (km|m) to m
    minorFactor *= factorToM;

    return "+a=" + type2string(majorValue) + " +b=" + type2string(minorValue);
}

std::string GribGridDefinition::getEarthsSphericalFigure() const
{
    double radius = gribHandleReader_.getDouble("radiusInMetres");
    return "+a=" + type2string(radius) + " +e=0";
}

std::string GribGridDefinition::getEarthsFigure() const
{
    long edition = gribHandleReader_.getLong( "editionNumber" );

    string earth;
    if (edition == 1) {
        long earthIsOblate = gribHandleReader_.getLong("earthIsOblate");
        if (earthIsOblate == 0) {
            earth = "+a=6367470 +e=0"; // sphere
        } else {
            earth = "+a=6378160 +b=6356775"; // oblate, maybe more or better defined in grib2???
        }
    } else {
        long shapeOfTheEarth = gribHandleReader_.getLong("shapeOfTheEarth");

        switch (shapeOfTheEarth)
        { // see code table 3.2
            case 0: earth = "+a=6367470 +e=0"; break;
            case 1: earth = getEarthsSphericalFigure(); break;
            case 2: earth = "+a=6378160 +b=6356775"; break;
            case 3: earth = getEarthsOblateFigure(1000); break;// number in km
            case 4: earth = "+a=6378137 +b=6356752.314"; break;
            case 5: earth = "+a=6378137 +b=6356752.314245"; break;// WGS84
            case 6: earth = "+a=6371229"; break;
            case 7: earth = getEarthsOblateFigure(1); break;// numbers in m
            case 8: earth = "+a=6371200 +e=0"; break;// TODO: definition not fully understood
        default: throw std::runtime_error("GRIB - undefined shape of the earth: " + type2string(shapeOfTheEarth));
        }
    }
    return earth;
}

std::string GribGridDefinition::regularLatLonProjDefinition() const
{
        // Define the PROJ definitions of the calculations
    std::ostringstream srcProjDef;
    srcProjDef << "+proj=longlat";
        long int earthIsOblate;
        earthIsOblate = gribHandleReader_.getLong("earthIsOblate");;
    if (earthIsOblate)
    {
        srcProjDef << " +a=6378160.0 +b=6356775.0";
    }
    else
    {
        srcProjDef << " +a=6367470.0";
    }
    srcProjDef << " +towgs84=0,0,0 +no_defs";

        // Set the PROJ string for SRID
    return srcProjDef.str();
}

std::string GribGridDefinition::rotatedLatLonProjDefinition() const
{
    double latRot, lonRot;
    lonRot = gribHandleReader_.getDouble("longitudeOfSouthernPoleInDegrees");
    latRot = gribHandleReader_.getDouble("latitudeOfSouthernPoleInDegrees");

    ostringstream oss;
    oss << "+proj=ob_tran +o_proj=longlat +lon_0=" << normalizeLongitude180(lonRot) << " +o_lat_p=" << (-1 * latRot) << " " + getEarthsFigure();
    string proj =  oss.str();

    return proj;
}

std::string
GribGridDefinition::lambertProjDefinition() const
{
    // Define the PROJ definitions used for the computation of the
    // Rotated projection
    std::ostringstream srcProjDef;
    srcProjDef << "+proj=lcc";
    srcProjDef << " +lat_1=";
    srcProjDef << gribHandleReader_.getDouble("Latin1InDegrees");
    srcProjDef << " +lat_2=";
    srcProjDef << gribHandleReader_.getDouble("Latin2InDegrees");
    srcProjDef << " +lon_0=";
        srcProjDef << gribHandleReader_.getDouble("longitudeOfFirstGridPointInDegrees");
        srcProjDef << " +lat_0=";
        srcProjDef << - gribHandleReader_.getDouble("latitudeOfFirstGridPointInDegrees");
        long int earthShape, radius;
        ostringstream errMsg;
        earthShape = gribHandleReader_.getLong("shapeOfTheEarth");
    switch (earthShape) {
    case 0:
        srcProjDef << " +a=6367470";
        break;
    case 1:
        radius = gribHandleReader_.getLong("scaledValueOfRadiusOfSphericalEarth");
        srcProjDef << " +a=" << radius;
        break;
    default:
        errMsg << "Can not handle given earth shape of lambert projection.";
        throw std::runtime_error( errMsg.str() );
    }
    srcProjDef << " +units=m +no_defs";
    // Set the PROJ string for SRID
    return srcProjDef.str();
}

} } } // end namespace
