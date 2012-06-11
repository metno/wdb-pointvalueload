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


#ifndef WCITRANSACTORS_HPP
#define WCITRANSACTORS_HPP

// wdb
#include <wdbException.h>

// libpqxx
#include <pqxx/transactor>
#include <pqxx/result>

// boost
#include <boost/shared_ptr.hpp>

// std
#include <iostream>
#include <string>

namespace wdb { namespace load { namespace point {
/**
 * Transactor to Begin the WCI
 */
class WciBegin : public pqxx::transactor<>
{
public:
    WciBegin(const std::string & wciUser) : pqxx::transactor<>("WciBegin"), wciUser_(wciUser)
    {
    }

    WciBegin(const std::string & wciUser, int dataprovidernamespaceid, int placenamespaceid, int parameternamespaceid)
        : pqxx::transactor<>("WciBegin"),
          wciUser_(wciUser),
          nameSpace_(new NameSpace(dataprovidernamespaceid, placenamespaceid, parameternamespaceid))
    {
    }

    /**
      *The transactors functor executes the query.
      */
    void operator()(argument_type &T)
    {
        std::ostringstream query;
        query << "SELECT wci.begin('" << wciUser_ << "'";
        if ( nameSpace_ )
            query << ", " << nameSpace_->dataprovider << ", " << nameSpace_->place << ", " << nameSpace_->parameter;

        query << ')';
        pqxx::result R = T.exec(query.str());
        std::clog << query.str() << std::endl;
    }

    /**
      * Commit handler. This is called if the transaction succeeds.
      */
    void on_commit()
    {
        std::clog << "wci.begin call complete" << std::endl;
    }

    /**
      * Abort handler. This is called if the transaction fails.
      * @param	Reason	The reason for the abort of the call
      */
    void on_abort(const char Reason[]) throw ()
    {
        std::cerr << "Transaction " << Name() << " failed " << Reason << std::endl;
    }

    /**
      * Special error handler. This is called if the transaction fails with an unexpected error.
      * Read the libpqxx documentation on transactors for details.
      */
    void on_doubt() throw ()
    {
        std::cerr << "Transaction " << Name() << " in indeterminate state" << std::endl;
    }

private:
    // wci user for wci.begin call
    std::string wciUser_;

    struct NameSpace
    {
        int dataprovider;
        int place;
        int parameter;

        NameSpace(int dataprovidernamespaceid, int placenamespaceid, int parameternamespaceid) :
            dataprovider(dataprovidernamespaceid),
            place(placenamespaceid),
            parameter(parameternamespaceid)
        {}
    };
    boost::shared_ptr<NameSpace> nameSpace_;
};

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
        std::clog << "Value inserted in database" << std::endl;
    }

    /**
      * Abort handler. This is called if the transaction fails.
      * @param	Reason	The reason for the abort of the call
      */
    virtual void on_abort(const char Reason[]) throw ()
    {
        std::clog  << "Transaction " << Name()
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
                          << "   Reason    " << Reason << std::endl;
    }

    /**
      * Special error handler. This is called if the transaction fails with an unexpected error.
      * Read the libpqxx documentation on transactors for details.
      */
    virtual void on_doubt() throw ()
    {
        std::clog << "Transaction " << Name() << " in indeterminate state" << std::endl;
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

            std::clog  << "Transaction " << Name()
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
                              << "   Reason    " << Reason << std::endl;
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

    /**
     * Transactor to retrieve the SI Unit conversion information required for a given
     * unit.
     */
    class InfoParameterUnit : public pqxx::transactor<>
    {
    public:
            /**
             * Default constructor.
             * @param	term		term 1
             * @param	coeff		coefficient 1
             * @param	srid		Descriptive PROJ string (srid)
             */
            InfoParameterUnit(float * coeff, float * term, const std::string unit) :
            pqxx::transactor<>("InfoParameterUnit"),
            coeff_(coeff),
            term_(term),
            unit_(unit)
        {
            // NOOP
        }

            /**
             * Functor. The transactors functor executes the query.
             */
            void operator()(argument_type &T)
            {
                    std::clog << "Checking unit conversion information for: " << unit_ << std::endl;
                    R = T.prepared("InfoParameterUnit")
                                              (unit_).exec();
                    if ( R.size() == 1 ) {
                            if ( R.at(0).at(4).is_null() ) {
                                    std::clog << "Did not find any conversion data for " << unit_ << std::endl;
                            }
                            else {
                                    R.at(0).at(4).to( *coeff_ );
                                    R.at(0).at(5).to( *term_ );
                            }
                    }
                    if ( R.size() != 1 ) {
                            std::cerr << "Problem finding unit data for " << unit_ << ". " << R.size() << " rows returned" << std::endl;
                    throw std::runtime_error("Transaction InfoParameterUnit did not return correct number of values. This suggests an error in the metadata.");
                    }
            }

            /**
             * Commit handler. This is called if the transaction succeeds.
             */
            void on_commit()
            {
                    // NOOP
            }

            /**
             * Abort handler. This is called if the transaction fails.
             * @param	Reason	The reason for the abort of the call
             */
            void on_abort(const char Reason[]) throw ()
            {
                    std::cerr << "Transaction " << Name() << " failed " << Reason << std::endl;
            }

            /**
             * Special error handler. This is called if the transaction fails with an unexpected error.
             * Read the libpqxx documentation on transactors for details.
             */
            void on_doubt() throw ()
            {
                std::cerr << "Transaction " << Name() << " in indeterminate state" << std::endl;
            }

    private:
            // Coefficient
            float * coeff_;
            // Term
            float * term_;
            /// The result returned by the query
            pqxx::result R;
            /// Value unit
            std::string unit_;

    };

} } } // namespace

#endif // WCITRANSACTORS_HPP
