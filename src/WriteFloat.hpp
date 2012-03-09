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

#ifndef WRITEFLOAT_HPP
#define WRITEFLOAT_HPP


/**
 * @file
 * Definition and implementation of float loading transactors used in the loader applications.
 */

// wdb
#include <wdbException.h>
#include <wdbLogHandler.h>

// libpqxx
#include "libpq-fe.h"
#include <pqxx/transactor>
#include <pqxx/result>
#include <pqxx/largeobject>

// std
#include <iostream>
#include <sstream>
#include <fstream>
#include <string>
#include <vector>

namespace wdb { namespace load { namespace point {
/**
 * Transactor to write a binary value. If the transaction fails,
 * it logs the error and throws a pqxx exception.
 */
class WriteFloat : public pqxx::transactor<>
{
public:
    /**
     * Default constructor.
     * @param	value				A value (doubles) in the point
     * @param	placeName			The place name of the fields grid description
     * @param	referenceTime		The reference time of the field
     * @param	validTimeFrom		Valid time from of the field
     * @param	validTimeTo			Valid time to of the field
     * @param	valueParameterName	The WDB name designation of the value parameter
     * @param	levelParameterName	The WDB name designation of the level parameter
     * @param	levelFrom			The lower level bound of the data
     * @param	levelTo				The upper level bound of the data
     */
    WriteFloat(
               const float value,
               const std::string& placeName,
               const std::string& referenceTime,
               const std::string& validTimeFrom,
               const std::string& validTimeTo,
               const std::string& valueParameterName,
               const std::string& levelParameterName,
               const float levelFrom,
               const float levelTo
              )
        : pqxx::transactor<>("WriteFloat"),
          placeName_(placeName),
          referenceTime_(referenceTime),
          validTimeFrom_(validTimeFrom),
          validTimeTo_(validTimeTo),
          valueParameterName_(valueParameterName),
          levelParameterName_(levelParameterName),
          levelFrom_(levelFrom),
          levelTo_(levelTo),
          value_(value)
    {
        // NOOP
    }

    /**
      * Functor. The transactors functor executes the query.
      */
    virtual void operator()(argument_type &T)
    {
        // Write
        R = T.prepared("WCIWriteFloat")
                (value_)
                (placeName_)
                (referenceTime_)
                (validTimeFrom_)
                (validTimeTo_)
                (valueParameterName_)
                (levelParameterName_)
                (levelFrom_)
                (levelTo_).exec();
    }

    /**
      * Commit handler. This is called if the transaction succeeds.
      */
    virtual void on_commit()
    {
        // Log
        WDB_LOG & log = WDB_LOG::getInstance( "wdb.baseload" );
        log.infoStream() << "Value inserted in database";
    }

    /**
      * Abort handler. This is called if the transaction fails.
      * @param	Reason	The reason for the abort of the call
      */
    virtual void on_abort(const char Reason[]) throw ()
    {
        WDB_LOG & log = WDB_LOG::getInstance( "wdb.baseload" );
        log.warnStream()  << "Transaction " << Name()
                          << " failed while trying wci.write ( "
                          << value_ <<", "
                          << placeName_ << ", "
                          << referenceTime_ << ", "
                          << validTimeFrom_ << ", "
                          << validTimeTo_ << ", "
                          << valueParameterName_ << ", "
                          << levelParameterName_ << ", "
                          << levelFrom_ << ", "
                          << levelTo_ << ")"
                          << "   Reason    " << Reason;
    }

    /**
      * Special error handler. This is called if the transaction fails with an unexpected error.
      * Read the libpqxx documentation on transactors for details.
      */
    virtual void on_doubt() throw ()
    {
        WDB_LOG & log = WDB_LOG::getInstance( "wdb.baseload" );
        log.warnStream() << "Transaction " << Name() << " in indeterminate state";
    }

protected:
    /// DataProvider
//    std::string dataProviderName_;
    /// Place Definition of Value
    std::string placeName_;
    /// Reference Time of Value
    std::string referenceTime_;
    /// Valid Time From
    std::string validTimeFrom_;
    /// Valid Time To
    std::string validTimeTo_;
    /// Value Parameter
    std::string valueParameterName_;
    /// Level Parameter
    std::string levelParameterName_;
    /// Level from
    float levelFrom_;
    /// Level to
    float levelTo_;
    /// Data Version of Value
//    int dataVersion_;
    /// Confidence Code
//    int confidenceCode_;
    /// Value
    const float value_;
    /// Number of Values
//    int noOfValues_;
    /// Result
    pqxx::result R;

};

    class Write : public WriteFloat
    {
    public:
        Write(
              const float value,
              const std::string& dataProvider,
              const std::string& placeName,
              const std::string& referenceTime,
              const std::string& validTimeFrom,
              const std::string& validTimeTo,
              const std::string& valueParameterName,
              const std::string& levelParameterName,
              const float levelFrom,
              const float levelTo,
              const int dataVersion
             )
            : WriteFloat(value,placeName,referenceTime,
                         validTimeFrom,validTimeTo,valueParameterName,
                         levelParameterName,levelFrom,levelTo
                         ),
              dataProvider_(dataProvider), dataVersion_(dataVersion)
        {
            // NOOP
        }

        /**
          * Abort handler. This is called if the transaction fails.
          * @param	Reason	The reason for the abort of the call
          */
        virtual void on_abort(const char Reason[]) throw ()
        {
            WDB_LOG & log = WDB_LOG::getInstance( "wdb.baseload" );
            log.warnStream()  << "Transaction " << Name()
                              << " failed while trying wci.write ( "
                              << value_ <<", "
                              << dataProvider_<<", "
                              << placeName_ << ", "
                              << referenceTime_ << ", "
                              << validTimeFrom_ << ", "
                              << validTimeTo_ << ", "
                              << valueParameterName_ << ", "
                              << levelParameterName_ << ", "
                              << levelFrom_ << ", "
                              << levelTo_ << ", "
                              << dataVersion_ << ")"
                              << "   Reason    " << Reason;
        }
        void operator()(argument_type &T)
        {
            // Write
            R = T.prepared("WciWritePoint")
                    (value_)
                    (dataProvider_)
                    (placeName_)
                    (referenceTime_)
                    (validTimeFrom_)
                    (validTimeTo_)
                    (valueParameterName_)
                    (levelParameterName_)
                    (levelFrom_)
                    (levelTo_)
                    (dataVersion_).exec();
        }
    private:
        std::string dataProvider_;
        int dataVersion_;
    };

} } } // end namepaces
#endif // WRITEFLOAT_HPP
