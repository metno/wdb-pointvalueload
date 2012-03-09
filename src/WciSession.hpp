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


#ifndef WCISESSION_H_
#define WCISESSION_H_

// wdb
#include <wdbException.h>
#include <wdbLogHandler.h>

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
        WDB_LOG & log = WDB_LOG::getInstance( "wdb.load.beginwci" );
        log.infoStream() << query.str();
    }

    /**
      * Commit handler. This is called if the transaction succeeds.
      */
    void on_commit()
    {
        WDB_LOG & log = WDB_LOG::getInstance( "wdb.load.beginwci" );
        log.infoStream() << "wci.begin call complete";
    }

    /**
      * Abort handler. This is called if the transaction fails.
      * @param	Reason	The reason for the abort of the call
      */
    void on_abort(const char Reason[]) throw ()
    {
        WDB_LOG & log = WDB_LOG::getInstance( "wdb.load.beginwci" );
        log.errorStream() << "Transaction " << Name() << " failed "
                          << Reason;
    }

    /**
      * Special error handler. This is called if the transaction fails with an unexpected error.
      * Read the libpqxx documentation on transactors for details.
      */
    void on_doubt() throw ()
    {
        WDB_LOG & log = WDB_LOG::getInstance( "wdb.load.beginwci" );
        log.errorStream() << "Transaction " << Name() << " in indeterminate state";
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

} } } // namespace
#endif /*WCISESSION_H_*/
