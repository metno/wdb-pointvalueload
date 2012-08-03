/*
 netcdfLoad

 Copyright (C) 2011 met.no

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

#ifndef CFGXMLELEMENT_H_
#define CFGXMLELEMENT_H_

// boost
#include <boost/shared_ptr.hpp>

// std
#include <string>
#include <tr1/unordered_map>

extern "C"
{
    typedef struct _xmlNode xmlNode;
    typedef xmlNode* xmlNodePtr;
}

using namespace std;
using namespace std::tr1;

class CfgDataProvider
{
public:
    explicit CfgDataProvider(xmlNodePtr providerNode);
    ~CfgDataProvider();

    const string& key() const { return key_; }
    const string& dataProviderName() const { return dataProviderName_; }
protected:
    string key_;
    string dataProviderName_;
};
typedef unordered_multimap<string, CfgDataProvider> dataprovider_multimap;

class CfgNiveau
{
public:
    explicit CfgNiveau(xmlNodePtr niveaNode);
    ~CfgNiveau();

    const string& level1() const { return level1_; }
    const string& level2() const { return level2_; }
    const string& wdbFrom() const { return wdbFrom_; }
    const string& wdbTo() const { return wdbTo_; }
    const string& wdbParameterName() const { return wdbParameterName_; }
    const string& wdbParameterUnits() const { return wdbParameterUnits_; }
protected:
    string level1_;
    string level2_;
    string wdbFrom_;
    string wdbTo_;
    string wdbParameterName_;
    string wdbParameterUnits_;
};
typedef unordered_multimap<string, CfgNiveau> niveau_multimap;

class CfgZAxis
{
public:
    explicit CfgZAxis(xmlNodePtr loadNode);
    ~CfgZAxis();

    const string& typeOfLevel() const { return typeOfLevel_; }
    const string& wdbName() const { return wdbName_; }
    const string& wdbUnits() const { return wdbUnits_; }
    const niveau_multimap& niveaus() const { return niveaus_; }
protected:
    void addNiveauSpec_(xmlNodePtr niveauNode);
    string typeOfLevel_;
    string wdbName_;
    string wdbUnits_;
    niveau_multimap niveaus_;
};
typedef unordered_multimap<string, CfgZAxis> zaxis_multimap;

class CfgXmlElement
{

public:
    const string& id() const { return id_; }
    const string& wdbName() const { return wdbName_; }
    const string& wdbUnits() const { return wdbUnits_; }
    const string& wdbValidTime() const { return wdbValidTime_; }
    const zaxis_multimap& zaxis() const { return zaxis_; }
protected:
    string id_;
    string wdbName_;
    string wdbUnits_;
    string wdbValidTime_;
    zaxis_multimap zaxis_;

    virtual void addElementSpec_(xmlNodePtr elementNode);
    virtual void addZAxisSpec_(xmlNodePtr zaxisNode);
};

class CfgNetcdf : public CfgXmlElement
{
public:
    explicit CfgNetcdf(xmlNodePtr loadNode);
    ~CfgNetcdf();
};

class CfgFelt : public CfgXmlElement
{
public:
    explicit CfgFelt(xmlNodePtr loadNode);
    ~CfgFelt();

    const string& verticalCoordinate() const { return verticalCoordinate_; }
protected:
    void addElementSpec_(xmlNodePtr elementNode);
    string verticalCoordinate_;
};

typedef unordered_multimap<string, CfgXmlElement> elements_multimap;
typedef unordered_multimap<string, CfgFelt> felt_multimap;

class CfgGrib1 : public CfgXmlElement
{
public:
    explicit CfgGrib1(xmlNodePtr grib1Node);
    ~CfgGrib1();

    const string& center() const { return center_; }
    const string& codetable2version() const { return codetable2version_; }
    const string& timerange() const { return timerange_; }
    const string& thresholdindicator() const { return thresholdindicator_; }
    const string& thresholdlower() const { return thresholdlower_; }
    const string& thresholdupper() const { return thresholdupper_; }
    const string& thresholdscale() const { return thresholdscale_; }

protected:
    string center_;
    string codetable2version_;
    string timerange_;
    string thresholdindicator_;
    string thresholdlower_;
    string thresholdupper_;
    string thresholdscale_;

    void addElementSpec_(xmlNodePtr grib1Node);
    void addZAxisSpec_(xmlNodePtr zaxisNode);
};
typedef unordered_multimap<string, CfgGrib1> grib1_multimap;

class CfgGrib2 : public CfgXmlElement
{
public:
    explicit CfgGrib2(xmlNodePtr grib2);
    ~CfgGrib2();
protected:
     void addZAxisSpec_(xmlNodePtr zaxisNode);
};
typedef unordered_multimap<string, CfgGrib2> grib2_multimap;

#endif /* CFGXMLELEMENT_H_ */
