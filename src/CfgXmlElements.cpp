/*
 netcdfLoad

 Copyright (C) 2012 met.no

 Contact information:
 Norwegian Meteorological Institute
 Box 43 Blindern
 0313 OSLO
 NORWAY
 E-mail: post@met.no

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

// project
#include "CfgXmlElements.hpp"

// libxml2
#include <libxml/tree.h>
#include <libxml/parser.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>

// std
#include <sstream>
#include <iostream>
#include <stdexcept>

using namespace std;
using namespace boost;

namespace {
    string getAttribute(xmlNodePtr elementNode, const string & name, const string alternative = string())
    {
        for ( xmlAttrPtr attr = elementNode->properties; attr; attr = attr->next )
                if ( xmlStrEqual(attr->name,(xmlChar*) name.c_str()) )
                        return (char*) attr->children->content;
        return alternative;
    }
} // end namespace

CfgDataProvider::CfgDataProvider(xmlNodePtr provider)
{
    dataProviderName_ = getAttribute(provider, "dataprovider_name");
    if(xmlStrEqual(provider->name, (xmlChar*) "netcdf")) {
       key_ = "netcdf," + getAttribute(provider, "parameter_name");
    } else if(xmlStrEqual(provider->name, (xmlChar*) "felt")) {
       key_= "felt," + getAttribute(provider, "producer_number") + "," + getAttribute(provider, "grid_number");
    } else if(xmlStrEqual(provider->name, (xmlChar*) "grib")) {
       key_= "grib," + getAttribute(provider, "generating_center") + "," + getAttribute(provider, "generating_process");
    }
}

CfgDataProvider::~CfgDataProvider() { }


CfgNiveau::CfgNiveau(xmlNodePtr niveauNode)
{
    level1_ = getAttribute(niveauNode, "level1");
    level2_ = getAttribute(niveauNode, "level2");
    wdbFrom_ = getAttribute(niveauNode, "wdb_from");
    wdbTo_ = getAttribute(niveauNode, "wdb_to");
    wdbParameterName_ = getAttribute(niveauNode, "wdb_parameter_name");
    wdbParameterUnits_ = getAttribute(niveauNode, "wdb_parameter_units");
}

CfgNiveau::~CfgNiveau() { }



CfgZAxis::CfgZAxis(xmlNodePtr zaxisNode)
{
    wdbName_ = getAttribute(zaxisNode, "wdb_name");
    wdbUnits_ = getAttribute(zaxisNode, "wdb_units");
    typeOfLevel_ = getAttribute(zaxisNode, "typeoflevel");
    for(xmlNodePtr subNode = zaxisNode->children; subNode; subNode = subNode->next) {
        if(xmlStrEqual(subNode->name, (xmlChar*) "niveau"))
            addNiveauSpec_(subNode);
    }
}

CfgZAxis::~CfgZAxis() { }

void CfgZAxis::addNiveauSpec_(xmlNodePtr niveauNode)
{
    CfgNiveau niveau(niveauNode);
    niveaus_.insert(niveau_multimap::value_type(niveau.level1(), niveau));
}


void CfgXmlElement::addZAxisSpec_(xmlNodePtr zaxisNode)
{
    CfgZAxis zaxis(zaxisNode);
    zaxis_.insert(zaxis_multimap::value_type(zaxis.wdbName(), zaxis));
}

void CfgXmlElement::addElementSpec_(xmlNodePtr elementNode)
{
    id_ = getAttribute(elementNode, "parameterid");
    wdbName_ = getAttribute(elementNode, "wdb_name");
    wdbUnits_ = getAttribute(elementNode, "wdb_units");
    wdbValidTime_ = getAttribute(elementNode, "wdb_validtime");
    for(xmlNodePtr subNode = elementNode->children; subNode; subNode = subNode->next) {
        if(xmlStrEqual(subNode->name, (xmlChar*) "zaxis"))
            addZAxisSpec_(subNode);
    }
}

//#################################################################################################/

CfgFelt::CfgFelt(xmlNodePtr feltNode)
{
    addElementSpec_(feltNode);
}

CfgFelt::~CfgFelt() {}

void CfgFelt::addElementSpec_(xmlNodePtr elementNode)
{
    CfgXmlElement::addElementSpec_(elementNode);
    verticalCoordinate_ = getAttribute(elementNode, "vertical_coordinate");
}
//#################################################################################################/

CfgNetcdf::CfgNetcdf(xmlNodePtr netcdfNode)
{
    addElementSpec_(netcdfNode);
}

CfgNetcdf::~CfgNetcdf() {}
//#################################################################################################/

CfgGrib1::CfgGrib1(xmlNodePtr grib1)
{
    addElementSpec_(grib1);
}

CfgGrib1::~CfgGrib1() {}

void CfgGrib1::addElementSpec_(xmlNodePtr grib1)
{
    CfgXmlElement::addElementSpec_(grib1);
    center_ = getAttribute(grib1, "center");
    codetable2version_ = getAttribute(grib1, "codetable2version");
    timerange_ = getAttribute(grib1, "timerange");
    thresholdindicator_ = getAttribute(grib1, "thresholdindicator");
    thresholdlower_ = getAttribute(grib1, "thresholdlower");
    thresholdupper_ = getAttribute(grib1, "thresholdupper");
    thresholdscale_ = getAttribute(grib1, "thresholdscale");
}

void CfgGrib1::addZAxisSpec_(xmlNodePtr zaxisNode)
{
    CfgZAxis zaxis(zaxisNode);
    if(zaxis.typeOfLevel().empty()) {
        stringstream keyStr;
        keyStr << this->center() << ", " << this->id();
        throw runtime_error( "No typeoflevel found for " + keyStr.str());
    }
    zaxis_.insert(zaxis_multimap::value_type(zaxis.typeOfLevel(), zaxis));
}
//#################################################################################################/

CfgGrib2::CfgGrib2(xmlNodePtr grib2)
{
    addElementSpec_(grib2);
}

CfgGrib2::~CfgGrib2() {}

void CfgGrib2::addZAxisSpec_(xmlNodePtr zaxisNode)
{
    CfgZAxis zaxis(zaxisNode);
    if(zaxis.typeOfLevel().empty()) {
        stringstream key;
        key << this->id() << ", " << this->id();
        throw runtime_error( "No typeoflevel found for " + key.str());
    }
    zaxis_.insert(zaxis_multimap::value_type(zaxis.typeOfLevel(), zaxis));
}
