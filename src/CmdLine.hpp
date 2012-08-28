/*
 pointLoad

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

#ifndef POINTCMDLINE_H_
#define POINTCMDLINE_H_

// wdb
#include <wdbConfiguration.h>

// boost
#include <boost/program_options.hpp>

// std
#include <string>

using namespace std;

namespace wdb { namespace load { namespace point {

    class CmdLine : public WdbConfiguration
    {
    public:
        CmdLine();
        ~CmdLine();

        struct OutputOptions
        {
            string outFileName;
        };

        struct InputOptions
        {
            string type;
            vector<string> file;
        };

        struct LoadingOptions
        {
            string dataProviderName;
            string validtimeConfig;
            string dataproviderConfig;
            string valueparameterConfig;
            string levelparameterConfig;
            string leveladditionsConfig;
            string valueparameter2Config;
            string levelparameter2Config;
            string leveladditions2Config;
            string unitsConfig;
            string fimexConfig;
            string fimexTemplate;
            string fimexInterpolateMethod;
            string fimexProcessRotateVectorToLatLonX;
            string fimexProcessRotateVectorToLatLonY;
        };

        const InputOptions & input() const { return input_; }
        const OutputOptions & output() const { return output_; }
        const LoadingOptions & loading() const { return loading_; }
    private:
        InputOptions input_;
        OutputOptions output_;
        LoadingOptions loading_;
};

} } } // end namespaces
#endif /* POINTCMDLINE_H_ */
