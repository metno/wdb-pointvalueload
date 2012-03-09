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
#include <wdb/LoaderConfiguration.h>

// project
#include <string>

namespace wdb { namespace load { namespace point {

    class CmdLine : public LoaderConfiguration
    {
    public:
        CmdLine();
        ~CmdLine();

        struct LoadingOptions
        {
            bool dryRun;
            std::string inputFileType;
            std::string mainCfgFileName;
            std::string referenceTime;
            std::string fimexReaderConfig;
            std::string fimexReaderTemplate;
//            std::string fimexReduceSouth;
//            std::string fimexReduceNorth;
//            std::string fimexReduceEast;
//            std::string fimexReduceWest;
        };

        const LoadingOptions & loading() const { return loading_; }

    private:
        LoadingOptions loading_;
};

} } } // end namespaces
#endif /* POINTCMDLINE_H_ */
