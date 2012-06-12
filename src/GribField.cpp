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
 * Implementation of the grib::Field class.
 * grib::Field encapulates the contents of a GRIB field
 */
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

// project
#include "GribField.hpp"
#include "GribHandleReader.hpp"
#include "GribGridDefinition.hpp"

// wdb
#include <wdbLogHandler.h>

// grib
#include <grib_api.h>

// boost
#include <boost/date_time/gregorian/gregorian.hpp>

// std
#include <limits>
#include <iostream>

using namespace std;
using namespace wmo::codeTable;
using namespace boost::posix_time;

// Support Functions
namespace {
    /**
      * Determine Time duration specified by GRIB data.
      * This function implements WMO Code Table 4.
      * @param	wmoTimeUnit		Time unit as specified by WMO Code Table 4
      * @param	gribTime		Amount of time specified
      * @return	Time duration specified by the above
      */
    time_duration duration( long int wmoTimeUnit, long int gribTime )
    {
    time_duration ret;
    switch ( wmoTimeUnit )
    {
    case 0: // Min
        return time_duration( 0, gribTime, 0, 0 );
    case 1: // Hour
        return time_duration( gribTime, 0, 0, 0 );
    case 2: // Day
        return time_duration( (gribTime * 24), 0, 0, 0 );
    case 3: // Month
        /// \bug{Month is assumed to be 30 days.}
        return time_duration( (gribTime * 24 * 30), 0, 0, 0 );
    case 4: // Year
        return time_duration( 0, 0, static_cast<int>(gribTime * 60 * 60 * 24 * 365.2425), 0 );
    case 5: // Deacde
        return time_duration( 0, 0, static_cast<int>(gribTime * 10 * 60 * 60 * 24 * 365.2425), 0 );
    case 6: // Normal (30 years)
        return time_duration( 0, 0, static_cast<int>(gribTime * 30 * 60 * 60 * 24 * 365.2425), 0 );
    case 7: // Century
        return time_duration( 0, 0, static_cast<int>(gribTime * 100 * 60 * 60 * 24 * 365.2425), 0 );
    case 10: // 3 hours
        return time_duration( gribTime * 3, 0, 0, 0 );
    case 11: // 6 housr
        return time_duration( gribTime * 6, 0, 0, 0 );
    case 12: // 12 hours
        return time_duration( gribTime * 12, 0, 0, 0 );
    case 254: // Second
        return time_duration( 0,  0, gribTime, 0 );
    default:
        throw std::runtime_error( "Invalid time unit in GRIB file" );
    }
}

    /**
      * Determine Time duration specified by GRIB data.
      * This function implements WMO Code Table 4.
      * @param	wmoTimeUnit		Time unit as specified by WMO Code Table 4
      * @param	gribTime		Amount of time specified
      * @return	Time duration specified by the above
      */
    time_duration duration( std::string wmoTimeUnit, long int gribTime )
    {
        time_duration ret;
        if ( wmoTimeUnit == "s" ) {
            return time_duration( 0, 0, gribTime, 0 );
        }
        if ( wmoTimeUnit == "m" ) {
            return time_duration( 0, gribTime, 0, 0 );
        }
        if ( wmoTimeUnit == "h" ) {
            return time_duration( gribTime, 0, 0, 0 );
        }
        if ( wmoTimeUnit == "3h" ) {
            return time_duration( gribTime * 3, 0, 0, 0 );
        }
        if ( wmoTimeUnit == "6h" ) {
            return time_duration( gribTime * 6, 0, 0, 0 );
        }
        if ( wmoTimeUnit == "12h" ) {
            return time_duration( gribTime * 12, 0, 0, 0 );
        }
        if ( wmoTimeUnit == "D" ) {
            return time_duration( gribTime * 24, 0, 0, 0 );
        }
        if ( wmoTimeUnit == "M" ) {
            /// \bug{Month is assumed to be 30 days.}
            return time_duration( (gribTime * 24 * 30), 0, 0, 0 );
        }
        if ( wmoTimeUnit == "Y" ) {
            return time_duration( 0, 0, static_cast<int>(gribTime * 60 * 60 * 24 * 365.2425), 0 );
        }
        if ( wmoTimeUnit == "10Y" ) {
            return time_duration( 0, 0, static_cast<int>(gribTime * 10 * 60 * 60 * 24 * 365.2425), 0 );
        }
        if ( wmoTimeUnit == "30Y" ) {
            return time_duration( 0, 0, static_cast<int>(gribTime * 30 * 60 * 60 * 24 * 365.2425), 0 );
        }
        if ( wmoTimeUnit == "C" ) {
            return time_duration( 0, 0, static_cast<int>(gribTime * 100 * 60 * 60 * 24 * 365.2425), 0 );
        }
        throw std::runtime_error( "Invalid time unit in GRIB file" );
    }


} // namespace Support

using namespace wdb::load;

const wmo::codeTable::ScanMode wdbStandardScanMode = LeftLowerHorizontal;

namespace wdb { namespace load { namespace point {

GribField::GribField( grib_handle * gribHandle )
        : values_(0)
        , sizeOfValues_(0)
{
    gribHandleReader_ = new GribHandleReader( gribHandle );
    grid_ = new GribGridDefinition( * gribHandleReader_ );

    initializeData( wdbStandardScanMode );
}

GribField::GribField( GribHandleReader * gribHandleReader ) :
                values_(0),
                sizeOfValues_(0),
                gribHandleReader_(gribHandleReader)
{
    grid_ = new GribGridDefinition( * gribHandleReader_ );
        initializeData( wdbStandardScanMode );
}



// Destructor
GribField::~GribField()
{
        if (values_ != 0)
                delete [] values_;
        delete grid_;
        delete gribHandleReader_;
}

// Initialize the Data
void
GribField::initializeData( wmo::codeTable::ScanMode defaultMode )
{
        // Retrieve the Values itself
        retrieveValues();
        // Convert Scanmode to default WDB Scanning Mode
        switch ( defaultMode ) {
        case LeftUpperHorizontal:
            gridToLeftUpperHorizontal( );
            break;
        case LeftLowerHorizontal:
            gridToLeftLowerHorizontal( );
            break;
        default:
            throw std::runtime_error( "Unsupported field conversion in GribField" );
        }
};

long int
GribField::getGeneratingCenter() const
{
        return gribHandleReader_->getLong( "centre" );
}

long int
GribField::getGeneratingProcess() const
{
        return gribHandleReader_->getLong( "generatingProcessIdentifier" );
}

string
GribField::getReferenceTime() const
{
        WDB_LOG & log = WDB_LOG::getInstance( "wdb.gribLoad.gribField" );
        long int date = gribHandleReader_->getLong( "dataDate" );
        long int time = gribHandleReader_->getLong( "dataTime" );
        long int year = date / 10000;
        long int month = (date - (year * 10000)) / 100;
        long int day = (date - (year * 10000)) - (month * 100);
        long int hour = time / 100;
        long int minute = time - (hour * 100);
        ptime referenceTime( boost::gregorian::date(year, month, day),
                                 time_duration( hour, minute, 0 ) );
    referenceTime_ = referenceTime;
    string ret = to_iso_extended_string(referenceTime);
    std::replace( ret.begin(), ret.end(), ',', '.' );
    ret += " UTC";
    log.debugStream() << "Got reference time: " << ret;
    return ret;
}

string
GribField::getValidityTime() const
{
        WDB_LOG & log = WDB_LOG::getInstance( "wdb.gribLoad.gribField" );
        long int date = gribHandleReader_->getLong( "validityDate" );
        long int time = gribHandleReader_->getLong( "validityTime" );
        long int year = date / 10000;
        long int month = (date - (year * 10000)) / 100;
        long int day = (date - (year * 10000)) - (month * 100);
        long int hour = time / 100;
        long int minute = time - (hour * 100);
        ptime validityTime( boost::gregorian::date(year, month, day),
                                 time_duration( hour, minute, 0 ) );
    validityTime_ = validityTime;
    string ret = to_iso_extended_string(validityTime);
    std::replace( ret.begin(), ret.end(), ',', '.' );
    ret += " UTC";
    log.debugStream() << "Got validity time: " << ret;
    return ret;
}


string
GribField::getValidTimeFrom() const
{
        getValidityTime();
        ptime validTimeF;
    validTimeF = validityTime_;
    string ret = to_iso_extended_string(validTimeF);
    std::replace( ret.begin(), ret.end(), ',', '.' );
    ret += " UTC";
        WDB_LOG & log = WDB_LOG::getInstance( "wdb.gribLoad.gribField" );
    log.debugStream() << "Valid From: " << ret;
    return ret;
}

string
GribField::getValidTimeTo() const
{
        getValidityTime();
        ptime validTimeT;
        std::string timeUnit = gribHandleReader_->getString( "stepUnits" );
        long int timeP1 = gribHandleReader_->getLong( "startStep" );
        long int timeP2 = gribHandleReader_->getLong( "endStep" );
        if (timeP2 > timeP1) {
            validTimeT = validityTime_ + duration( timeUnit, timeP2 - timeP1 );
        }
    validTimeT = validityTime_;
    string ret = to_iso_extended_string(validTimeT);
    std::replace( ret.begin(), ret.end(), ',', '.' );
    ret += " UTC";
        WDB_LOG & log = WDB_LOG::getInstance( "wdb.gribLoad.gribField" );
    log.debugStream() << "Valid To: " << ret;
    return ret;
}

    long int GribField::getCodeTableVersionNumber() const
    {
        return  gribHandleReader_->getLong( "gribTablesVersionNo" );
    }

    long int GribField::getParameter1() const
    {
        if(getEditionNumber() == 1) {
            return gribHandleReader_->getLong( "indicatorOfParameter" );
        } else {
            return -1;
        }
    }

    long int GribField::getParameter2() const
    {
        if(getEditionNumber() == 2) {
            return gribHandleReader_->getLong( "parameterNumber" );
        } else {
            return -1;
        }
    }

    long int GribField::getTimeRange() const
    {
        return gribHandleReader_->getLong( "timeRangeIndicator" );
    }

    long int GribField::getLevelParameter1() const
    {
        if(getEditionNumber() == 1) {
            return gribHandleReader_->getLong( "indicatorOfTypeOfLevel" );
        } else {
            return -1;
        }
    }

    std::string GribField::getLevelParameter2() const
    {
        return gribHandleReader_->getString( "typeOfLevel" );
    }

    /**
      * @todo MiA 20070508 getLevelFrom and getLevelTo do not yet handle leveltypes with differing
      * top and bottom layers
      */
    double GribField::getLevelFrom() const
    {
        return gribHandleReader_->getDouble( "level" );
    }

    double GribField::getLevelTo( ) const
    {
        return gribHandleReader_->getDouble( "level" );
    }

    int GribField::getDataVersion() const
    {
        if(getEditionNumber() == 1) {
            long int localUsage = gribHandleReader_->getLong( "localUsePresent" );
            WDB_LOG & log = WDB_LOG::getInstance( "wdb.gribLoad.gribField" );
            log.debugStream() << "Got LocalUsage: " << localUsage;
            if( localUsage == 0 ) {
                return 0; // No local use section - no definition of data version
            } else {
                long int marsType = gribHandleReader_->getLong( "marsType" );
                log.debugStream() << "Got MARS Type: " << marsType;
                if ( marsType == 11 ) { // Perturbed Forecast
                    long int pertubNumber = gribHandleReader_->getLong( "perturbationNumber" );
                    log.debugStream() << "Got Perturbation Number: " << pertubNumber;
                    return pertubNumber;
                }
            }
            // Return default
            return 0;
        } else {
            return 0;
        }
    }

int GribField::getEditionNumber() const
{
    int ret = gribHandleReader_->getLong( "editionNumber" );
    WDB_LOG & log = WDB_LOG::getInstance( "wdb.gribLoad.gribField" );
    log.debugStream() << "Got GRIB Version: " << ret;
    return ret;
}

int GribField::numberX() const
{
    return grid_->numberX();
};

int
GribField::numberY() const
{
    return grid_->numberY();
};

float
GribField::incrementX() const
{
        return grid_->incrementX();
};

float
GribField::incrementY() const
{
        return grid_->incrementY();
};

float
GribField::startX() const
{
        return grid_->startX();
};

float
GribField::startY() const
{
        return grid_->startY();
};

std::string
GribField::getProjDefinition() const
{
        return grid_->getProjDefinition();
}

// Get grid values
const double *
GribField::getValues( ) const
{
        return values_;
}

size_t
GribField::getValuesSize() const
{
        return sizeOfValues_;
}

double GribField::getMissingValue() const
{
        return gribHandleReader_->getDouble("missingValue");
}


void
GribField::retrieveValues()
{
    WDB_LOG & log = WDB_LOG::getInstance( "wdb.gribLoad.gribfield" );
        sizeOfValues_ = gribHandleReader_->getValuesSize();
        values_ =  gribHandleReader_->getValues( );
    log.debugStream() << "Retrieved " << sizeOfValues_ << " values from the field";
    if (sizeOfValues_ < 1) {
        string errorMessage = "Size of value grid is less than 1 byte";
        log.errorStream() << errorMessage;
        throw std::runtime_error( errorMessage );
    }
    unsigned int gridSize = (grid_->numberX() * grid_->numberY());
    if ( sizeOfValues_ != gridSize ) {
        string errorMessage = "Size of value grid is inconsistent with definition";
        throw std::runtime_error( errorMessage );
    }
}

void
GribField::gridToLeftUpperHorizontal( )
{
    WDB_LOG & log = WDB_LOG::getInstance( "wdb.gribLoad.gribField" );
        wmo::codeTable::ScanMode fromMode = grid_->getScanMode();
    int nI = grid_->numberX();
    int nJ = grid_->numberY();

    switch( fromMode )
    {
        case LeftUpperHorizontal:
            log.debugStream() << "Grid was already in requested format";
            break;
        case LeftLowerHorizontal:
            // Todo: Implementation needs to be tested...
            log.debugStream() << "Swapping LeftLowerHorizontal to LeftUpperHorizontal";
            for ( int j = 1; j <= nJ / 2; j ++ ) {
                for ( int i = 0; i < nI; i ++ ) {
                    swap( values_[((nJ - j) * nI) + i], values_[((j - 1) * nI) + i] );
                }
            }
            grid_->setScanMode( LeftUpperHorizontal );
            break;
        default:
            throw std::runtime_error( "Unsupported field conversion in gridToLeftUpperHorizontal");
    }
}

void
GribField::gridToLeftLowerHorizontal( )
{
    WDB_LOG & log = WDB_LOG::getInstance( "wdb.gribLoad.gribField" );
        wmo::codeTable::ScanMode fromMode = grid_->getScanMode();

    int nI = grid_->numberX();
    int nJ = grid_->numberY();

    switch( fromMode )
    {
        case LeftUpperHorizontal:
            log.debugStream() << "Swapping LeftUpperHorizontal to LeftLowerHorizontal";
            for ( int j = 1; j <= nJ / 2; j ++ ) {
                for ( int i = 0; i < nI; i ++ ) {
                    swap( values_[((nJ - j) * nI) + i], values_[((j - 1) * nI) + i] );
                }
            }
            grid_->setScanMode( LeftLowerHorizontal );
            break;
        case LeftLowerHorizontal:
            log.debugStream() << "Grid was already in requested format";
            break;
        default:
            throw std::runtime_error( "Unsupported field conversion in gridToLeftLowerHorizontal" );
    }
}

    std::string GribField::toString() const
    {
        std::ostringstream buffer;
        buffer << " edition          :" << getEditionNumber()     << std::endl
               << " dataverson       :" << getDataVersion()       << std::endl
               << " center           :" << getGeneratingCenter()  << std::endl
               << " process          :" << getGeneratingProcess() << std::endl
               << " parameter1       :" << getParameter1()        << std::endl
               << " parameter2       :" << getParameter2()        << std::endl
               << " levelparameter1  :" << getLevelParameter1()   << std::endl
               << " levelparameter2  :" << getLevelParameter2()   << std::endl
               << " levelfrom        :" << getLevelFrom()         << std::endl
               << " levelto          :" << getLevelTo()           << std::endl
               << " projection       :" << getProjDefinition()    << std::endl;

        return buffer.str();
    }

} } } // end namspaces
