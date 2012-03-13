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

#ifndef POINTWDBCONNECTION_HPP
#define POINTWDBCONNECTION_HPP


// wdb loader
#include <wdb/LoaderConfiguration.h>
#include <wdb/LoaderDatabaseConnection.h>

namespace wdb { namespace load { namespace point {

    class WdbConnection : public pqxx::connection
    {
    public:
        WdbConnection(const WdbConfiguration& config, std::string placenamespace);

        ~WdbConnection();

        void wcibegin(const std::string& dataProvider);

        void readUnit( const std::string & unit, float * coeff, float * term );

        void write(
                const float value,
                const std::string& placeName,
                const std::string& referenceTime,
                const std::string& validTimeFrom,
                const std::string& validTimeTo,
                const std::string& valueParameterName,
                const std::string& levelParameterName,
                const float levelFrom,
                const float levelTo
                );

        void write(
                const float value,
                const std::string& dataprovder,
                const std::string& placeName,
                const std::string& referenceTime,
                const std::string& validTimeFrom,
                const std::string& validTimeTo,
                const std::string& valueParameterName,
                const std::string& levelParameterName,
                const float levelFrom,
                const float levelTo,
                const int dataversion
                );
    private:
        void setup_();
        const WdbConfiguration * config_;
    };

} } } // end namespaces

#endif // POINTDBCONNECTION_HPP
