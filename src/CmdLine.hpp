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

namespace wdb { namespace load { namespace point {

    class CmdLine : public WdbConfiguration
    {
    public:
        CmdLine();
        ~CmdLine();

        struct OutputOptions
        {
            bool dry_run;
        };

        struct InputOptions
        {
            std::string type;
            std::vector<std::string> file;
        };

        struct LoadingOptions
        {
            std::string nameSpace;
            std::string validtimeConfig;
            std::string dataproviderConfig;
            std::string valueparameterConfig;
            std::string levelparameterConfig;
            std::string leveladditionsConfig;
            std::string valueparameter2Config;
            std::string levelparameter2Config;
            std::string leveladditions2Config;
            std::string fimexConfig;
            std::string fimexTemplate;
//            std::string fimexReduceSouth;
//            std::string fimexReduceNorth;
//            std::string fimexReduceEast;
//            std::string fimexReduceWest;
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
