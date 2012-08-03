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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

// project
#include "CfgXmlElements.hpp"
#include "CfgXmlFileReader.hpp"

// wdb 
#include <wdbException.h>

// libxml2
#include <libxml/tree.h>
#include <libxml/parser.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>

// boost
#include <boost/regex.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string/trim.hpp>

// std
#include <cstdlib>
#include <iostream>
#include <stdexcept>

namespace
{
    struct XmlSession
    {
	XmlSession()
	{
		xmlInitParser();
	}
	~XmlSession()
	{
		xmlCleanupParser();
	}
    };
}

using namespace std;

namespace wdb { namespace load { namespace point {

    CfgXmlFileReader::CfgXmlFileReader(const boost::filesystem::path& configFile)
    {
        if(not exists(configFile))
            throw std::runtime_error(configFile.file_string() + ": No config-XML file");
        if(is_directory(configFile))
            throw std::runtime_error(configFile.file_string() + " is a directory, config-XML file expecdted");

        fileName_ = configFile.file_string();

        XmlSession session;
        boost::shared_ptr<xmlDoc> doc(xmlParseFile(fileName_.c_str()), xmlFreeDoc);
        if (not doc)
            throw std::runtime_error("Unable to parse doc");

        boost::shared_ptr<xmlXPathContext> xpathCtx(xmlXPathNewContext(doc.get()), xmlXPathFreeContext);
        if (not xpathCtx)
            throw std::runtime_error("unable to create xpath context");

        init_(xpathCtx.get());
    }

    CfgXmlFileReader::~CfgXmlFileReader() { }

    void CfgXmlFileReader::init_(xmlXPathContextPtr context)
    {
        boost::shared_ptr<xmlXPathObject>
           xpathObjLoad(xmlXPathEvalExpression((const xmlChar *) "/mapping/load/felt", context), xmlXPathFreeObject);

        if(xpathObjLoad) {
            xmlNodeSetPtr nodesLoad = xpathObjLoad->nodesetval;

            for(int i = 0; i < nodesLoad->nodeNr; ++ i)
            {
                xmlNodePtr elementNode =  nodesLoad->nodeTab[i];
                if(elementNode->type != XML_ELEMENT_NODE)
                    throw std::runtime_error("Expected element node");

//                cout<<"felt node found" << endl;
                CfgFelt felt(elementNode);
                string key;
                key.append(felt.id()).append(",").append(felt.verticalCoordinate());
                feltfloads_.insert(felt_multimap::value_type(key, felt));
            }
        }


        xpathObjLoad = boost::shared_ptr<xmlXPathObject>(xmlXPathEvalExpression((const xmlChar *) "/mapping/load/netcdf", context), xmlXPathFreeObject);
        if(xpathObjLoad) {
            xmlNodeSetPtr nodesLoad = xpathObjLoad->nodesetval;

            for(int i = 0; i < nodesLoad->nodeNr; ++ i)
            {
                xmlNodePtr elementNode =  nodesLoad->nodeTab[i];
                if(elementNode->type != XML_ELEMENT_NODE)
                    throw std::runtime_error("Expected element node");

//                cout<<"netcdf node found" << endl;
                CfgNetcdf netcdf(elementNode);
                netcdfloads_.insert(elements_multimap::value_type(netcdf.id(), netcdf));
            }
        }

        xpathObjLoad = boost::shared_ptr<xmlXPathObject>(xmlXPathEvalExpression((const xmlChar *) "/mapping/load/grib1", context), xmlXPathFreeObject);
        if(xpathObjLoad) {
            xmlNodeSetPtr nodesLoad = xpathObjLoad->nodesetval;

            for(int i = 0; i < nodesLoad->nodeNr; ++ i)
            {
                xmlNodePtr elementNode =  nodesLoad->nodeTab[i];
                if(elementNode->type != XML_ELEMENT_NODE)
                    throw std::runtime_error("Expected element node");

//                cout<<"grib1 node found" << endl;
                CfgGrib1 grib1(elementNode);

                stringstream key;
                key << grib1.center() << ","
                    << grib1.codetable2version() << ","
                    << grib1.id() << ","
                    << grib1.timerange() << ","
                    << grib1.thresholdindicator() << ","
                    << grib1.thresholdlower() << ","
                    << grib1.thresholdupper() << ","
                    << grib1.thresholdscale();
                grib1loads_.insert(grib1_multimap::value_type(key.str(), grib1));
            }
        }

        xpathObjLoad = boost::shared_ptr<xmlXPathObject>(xmlXPathEvalExpression((const xmlChar *) "/mapping/load/grib2", context), xmlXPathFreeObject);
        if(xpathObjLoad) {
            xmlNodeSetPtr nodesLoad = xpathObjLoad->nodesetval;

            for(int i = 0; i < nodesLoad->nodeNr; ++ i)
            {
                xmlNodePtr elementNode =  nodesLoad->nodeTab[i];
                if(elementNode->type != XML_ELEMENT_NODE)
                    throw std::runtime_error("Expected element node");

//                cout<<"grib2 node found" << endl;
                CfgGrib2 grib2(elementNode);
                grib2loads_.insert(grib2_multimap::value_type(grib2.id(), grib2));
            }
        }

        xpathObjLoad = boost::shared_ptr<xmlXPathObject>(xmlXPathEvalExpression((const xmlChar *) "/mapping/dataproviders", context), xmlXPathFreeObject);
        if(xpathObjLoad) {
            xmlNodeSetPtr nodesLoad = xpathObjLoad->nodesetval;

            for(int i = 0; i < nodesLoad->nodeNr; ++ i)
            {
                xmlNodePtr elementNode =  nodesLoad->nodeTab[i];
                if(elementNode->type != XML_ELEMENT_NODE)
                    throw std::runtime_error("Expected element node");

                for(xmlNodePtr child = elementNode->children; child != NULL; child = child->next)
                {
                    CfgDataProvider provider(child);
                    if(provider.key().empty())
                        continue;
                    dataproviders_.insert(dataprovider_multimap::value_type(provider.key(), provider));
                }

            }
        }
    }

    string CfgXmlFileReader::dataProviderName4Netcdf(const string& variablename) const
    {
        stringstream keyStr;
        keyStr << "netcdf" << "," << variablename;
        if(dataproviders_.count(keyStr.str()) == 0) return string();

        const CfgDataProvider& provider = dataproviders_.find(keyStr.str())->second;
        return provider.dataProviderName();
    }

    string CfgXmlFileReader::valueParameterName4Netcdf(const string& variablename) const
    {
        if(netcdfloads_.count(variablename) == 0)
            return string();

        return netcdfloads_.find(variablename)->second.wdbName();
    }

    void CfgXmlFileReader::levelValues4Netcdf(vector<Level>& levels, const string& variablename) const
    {
        if(netcdfloads_.count(variablename) == 0) return;

        const CfgXmlElement& netcdf = netcdfloads_.find(variablename)->second;
        if(netcdf.zaxis().begin() == netcdf.zaxis().end()) return;

        zaxis_multimap::const_iterator zit = netcdf.zaxis().begin();
        const CfgZAxis& zaxis = zit->second;

        for(niveau_multimap::const_iterator nit = zaxis.niveaus().begin(); nit != zaxis.niveaus().end(); ++nit) {
            wdb::load::Level level(zaxis.wdbName(), boost::lexical_cast<float>(nit->second.wdbFrom()), boost::lexical_cast<float>(nit->second.wdbTo()));
            levels.push_back(level);
//            cout << level.levelFrom_ << " - " << level.levelTo_ << endl;
        }

        return;
    }

    string CfgXmlFileReader::dataProviderName4Felt(const string& producer, const string& gridarea) const
    {
        stringstream keyStr;
        keyStr << "felt" << "," << producer << "," << gridarea;
        if(dataproviders_.count(keyStr.str()) == 0) return string();

        const CfgDataProvider& provider = dataproviders_.find(keyStr.str())->second;
        return provider.dataProviderName();
    }

    string CfgXmlFileReader::valueParameterName4Felt(const string& parameter, const string& verticalcoordinate, const string& level1) const
    {
        stringstream keyStr;
        keyStr << parameter << "," << verticalcoordinate;

        if(feltfloads_.count(keyStr.str()) == 0) return string();

//        cout << __FUNCTION__ << ":" << __LINE__ << endl;
        const CfgFelt& felt = feltfloads_.find(keyStr.str())->second;
//        cout << __FUNCTION__ << ":" << __LINE__ << endl;
        zaxis_multimap::const_iterator zit = felt.zaxis().begin();
        const CfgZAxis& zaxis = zit->second;

        if(zaxis.niveaus().count(level1) == 0)
            return felt.wdbName();

        string override_name = zaxis.niveaus().find(level1)->second.wdbParameterName();
        return override_name.empty() ? felt.wdbName() : override_name;
    }

    string CfgXmlFileReader::valueParameterUnits4Felt(const string& parameter, const string& verticalcoordinate, const string& level1) const
    {
        stringstream keyStr;
        keyStr << parameter << "," << verticalcoordinate;

        if(feltfloads_.count(keyStr.str()) == 0) return string();

//        cout << __FUNCTION__ << ":" << __LINE__ << endl;
        const CfgFelt& felt = feltfloads_.find(keyStr.str())->second;
//        cout << __FUNCTION__ << ":" << __LINE__ << endl;
        zaxis_multimap::const_iterator zit = felt.zaxis().begin();
        const CfgZAxis& zaxis = zit->second;

        if(zaxis.niveaus().count(level1) == 0)
            return felt.wdbUnits();

        string override_units = zaxis.niveaus().find(level1)->second.wdbParameterUnits();
        return override_units.empty() ? felt.wdbUnits() : override_units;
    }

    void CfgXmlFileReader::levelValues4Felt(vector<Level>& levels, const string& parameter, const string& verticalcoordinate, const string& level1, const string& level2) const
    {
        stringstream keyStr;
        keyStr << parameter << "," << verticalcoordinate;

        if(feltfloads_.count(keyStr.str()) == 0) return;

        const CfgFelt& felt = feltfloads_.find(keyStr.str())->second;

        zaxis_multimap::const_iterator zit = felt.zaxis().begin();
        const CfgZAxis& zaxis = zit->second;

        if(felt.zaxis().begin() == felt.zaxis().end()) return;

        if(zaxis.niveaus().count(level1) == 0) return;

        pair<niveau_multimap::const_iterator, niveau_multimap::const_iterator> range = zaxis.niveaus().equal_range(level1);

        for(niveau_multimap::const_iterator nit = range.first; nit != range.second; ++nit) {
            if(nit->second.level2() == level2) {
                wdb::load::Level level(zaxis.wdbName(), boost::lexical_cast<float>(nit->second.wdbFrom()), boost::lexical_cast<float>(nit->second.wdbTo()));
                levels.push_back(level);
//                cout << level.levelFrom_ << " - " << level.levelTo_ << endl;
                break;
            }
        }

        return;
    }


    string CfgXmlFileReader::dataProviderName4Grib(const string& center, const string& process) const
    {
        stringstream keyStr;
        keyStr << "grib" << "," << center << "," << process;
        if(dataproviders_.count(keyStr.str()) == 0) return string();

        const CfgDataProvider& provider = dataproviders_.find(keyStr.str())->second;
        return provider.dataProviderName();
    }

    string CfgXmlFileReader::valueParameterName4Grib1(const string& center,
                                                      const string& codetable2version,
                                                      const string& parameterid,
                                                      const string& timerange,
                                                      const string& thresholdindicator,
                                                      const string& thresholdlower,
                                                      const string& thresholdupper,
                                                      const string& thresholdscale) const
    {
        stringstream key;
        key << center << ","
            << codetable2version << ","
            << parameterid << ","
            << timerange << ","
            << thresholdindicator << ","
            << thresholdlower << ","
            << thresholdupper << ","
            << thresholdscale;

        if(grib1loads_.count(key.str()) == 0) return string();

//        cout << __FUNCTION__ << ":" << __LINE__ << endl;
        const CfgGrib1& grib1 = grib1loads_.find(key.str())->second;
//        cout << __FUNCTION__ << ":" << __LINE__ << endl;

        return grib1.wdbName();
    }

    string CfgXmlFileReader::valueParameterUnits4Grib1(const string& center,
                                                       const string& codetable2version,
                                                       const string& parameterid,
                                                       const string& timerange,
                                                       const string& thresholdindicator,
                                                       const string& thresholdlower,
                                                       const string& thresholdupper,
                                                       const string& thresholdscale) const
    {
        stringstream key;
        key << center << ","
            << codetable2version << ","
            << parameterid << ","
            << timerange << ","
            << thresholdindicator << ","
            << thresholdlower << ","
            << thresholdupper << ","
            << thresholdscale;

        if(grib1loads_.count(key.str()) == 0) return string();

//        cout << __FUNCTION__ << ":" << __LINE__ << endl;
        const CfgGrib1& grib1 = grib1loads_.find(key.str())->second;
//        cout << __FUNCTION__ << ":" << __LINE__ << endl;

        return grib1.wdbUnits();
    }

    void CfgXmlFileReader::levelValues4Grib1(vector<wdb::load::Level>& levels,
                                             const string& center,
                                             const string& codetable2version,
                                             const string& parameterid,
                                             const string& timerange,
                                             const string& thresholdindicator,
                                             const string& thresholdlower,
                                             const string& thresholdupper,
                                             const string& thresholdscale,
                                             const string& typeoflevel,
                                             const string& level) const
    {
        stringstream key;
        key << center << ","
            << codetable2version << ","
            << parameterid << ","
            << timerange << ","
            << thresholdindicator << ","
            << thresholdlower << ","
            << thresholdupper << ","
            << thresholdscale;

        if(grib1loads_.count(key.str()) == 0) return;

        const CfgGrib1& grib1 = grib1loads_.find(key.str())->second;

        zaxis_multimap::const_iterator zit = grib1.zaxis().find(typeoflevel);
        if(grib1.zaxis().begin() == grib1.zaxis().end()) return;

        const CfgZAxis& zaxis = zit->second;

        if(zaxis.niveaus().count(level) == 0) return;

        pair<niveau_multimap::const_iterator, niveau_multimap::const_iterator> range = zaxis.niveaus().equal_range(level);

        niveau_multimap::const_iterator nit = range.first;

        wdb::load::Level wdblevel(zaxis.wdbName(), boost::lexical_cast<float>(nit->second.wdbFrom()), boost::lexical_cast<float>(nit->second.wdbTo()));
        levels.push_back(wdblevel);
//        cout << wdblevel.levelFrom_ << " - " << wdblevel.levelTo_ << endl;

        return;
    }

    string CfgXmlFileReader::valueParameterName4Grib2(const string& parameterid) const
    {
        if(netcdfloads_.count(parameterid) == 0)
            return string();

        return netcdfloads_.find(parameterid)->second.wdbName();
    }

    string CfgXmlFileReader::valueParameterUnits4Grib2(const string& parameterid) const
    {
        if(netcdfloads_.count(parameterid) == 0)
            return string();

        return netcdfloads_.find(parameterid)->second.wdbUnits();
    }

    void CfgXmlFileReader::levelValues4Grib2(vector<wdb::load::Level>& levels, const string& parameterid, const string& typeoflevel, const string& level) const
    {
        stringstream key;
        key << parameterid;

        if(grib2loads_.count(key.str()) == 0) return;

        const CfgGrib2& grib2 = grib2loads_.find(key.str())->second;

        zaxis_multimap::const_iterator zit = grib2.zaxis().find(typeoflevel);
        if(grib2.zaxis().begin() == grib2.zaxis().end()) return;

        const CfgZAxis& zaxis = zit->second;

        if(zaxis.niveaus().count(level) == 0) return;

        pair<niveau_multimap::const_iterator, niveau_multimap::const_iterator> range = zaxis.niveaus().equal_range(level);

        niveau_multimap::const_iterator nit = range.first;

        wdb::load::Level wdblevel(zaxis.wdbName(), boost::lexical_cast<float>(nit->second.wdbFrom()), boost::lexical_cast<float>(nit->second.wdbTo()));
        levels.push_back(wdblevel);
//        cout << wdblevel.levelFrom_ << " - " << wdblevel.levelTo_ << endl;

        return;
    }
} } } // end namespaces
