/*******************************************************************************

   Copyright (C) 2011-Present SequoiaDB Ltd.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

      http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

   Source File Name = rcGeneratorBase.cpp

   Descriptive Name = N/A

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who         Description
   ====== =========== =========== =============================================
          23/07/2025  fangjiabin  Initial Draft

   Last Changed =

*******************************************************************************/
#include "rcGeneratorBase.hpp"
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/foreach.hpp>

using namespace boost::property_tree ;

// xml element
#define XML_FIELD_CONSLIST    "rclist.conslist"
#define XML_FIELD_CODELIST    "rclist.codelist"
#define XML_FIELD_TAG_LIST    "rclist.taglist"
#define XML_FIELD_NAME        "name"
#define XML_FIELD_VALUE       "value"
#define XML_FIELD_BASE        "base"
#define XML_FIELD_TAG         "tag"
#define XML_FIELD_DESCRIPTION "description"
#define XML_FIELD_CN          LANG_CN
#define XML_FIELD_EN          LANG_EN

// xml value
#define XML_VALUE_RESERVED_ERROR  "SDB_ERROR_RESERVED"

rcGeneratorBase::rcGeneratorBase() : _maxFieldWidth( 0 )
{
}

rcGeneratorBase::~rcGeneratorBase()
{
}

int rcGeneratorBase::init()
{
   return _loadRcList() ;
}

int rcGeneratorBase::_loadRcList()
{
   int rc = 0 ;
   ptree pt ;
   string configPath ;

   configPath = utilCatPath( _rootPath.c_str(), RC_FILE_PATH ) ;
   configPath = utilGetRealPath2( configPath.c_str() ) ;
   map<int, vector<RCInfo> > rclistWithTag;

   try
   {
      read_xml( configPath.c_str(), pt ) ;
   }
   catch ( std::exception &e )
   {
      printLog( PD_ERROR ) << "Failed to read xml file: " << e.what() << endl ;
      rc = 1 ;
      goto error ;
   }

   try
   {
      int errNum = 0 ;

      BOOST_FOREACH ( ptree::value_type &v, pt.get_child( XML_FIELD_CONSLIST ) )
      {
         pair<string, int> constant (
            v.second.get<string>( XML_FIELD_NAME ),
            v.second.get<int>( XML_FIELD_VALUE )
         ) ;

         _conslist.push_back( constant ) ;
      }

      if (pt.get_child_optional( XML_FIELD_TAG_LIST ))
      {
         BOOST_FOREACH ( ptree::value_type &v, pt.get_child( XML_FIELD_TAG_LIST ) )
         {
            string name = v.second.get<string>(XML_FIELD_NAME);
            int base = v.second.get<int>(XML_FIELD_BASE);
            if (0 < _tags.count(name))
            {
               throw std::invalid_argument("duplicated tag name in rclist.xml");
            }
            if (0 == base)
            {
               throw std::invalid_argument("base of tag can not be zero");
            }
            _tags[name] = base;
         }
      }

      BOOST_FOREACH( ptree::value_type &v, pt.get_child ( XML_FIELD_CODELIST ) )
      {
         RCInfo rcInfo ;
         ptree vv = v.second.get_child( XML_FIELD_DESCRIPTION ) ;

         rcInfo.name = v.second.get<string>( XML_FIELD_NAME ) ;
         rcInfo.desc_cn = vv.get<string>( XML_FIELD_CN ) ;
         rcInfo.desc_en = vv.get<string>( XML_FIELD_EN ) ;
         if (!v.second.get_optional<string>(XML_FIELD_TAG))
         {
            rcInfo.value = -( errNum + 1 ) ;

            if ( utilStrStartsWith( rcInfo.name, XML_VALUE_RESERVED_ERROR ) )
            {
               rcInfo.reserved = true ;
            }

            _rcInfoList.push_back( rcInfo ) ;

            ++errNum ;
         }
         else
         {
            string tag = v.second.get<string>(XML_FIELD_TAG);
            map<string, int>::const_iterator itr = _tags.find(tag);
            if (itr == _tags.end())
            {
               throw std::invalid_argument("tag specified in code not found");
            }
            vector<RCInfo> & rcVec = rclistWithTag[itr->second];
            rcInfo.value = -(rcVec.size() + itr->second);
            if ( utilStrStartsWith( rcInfo.name, XML_VALUE_RESERVED_ERROR ) )
            {
               rcInfo.reserved = true ;
            }
            rcVec.push_back(rcInfo);
         }

         _maxFieldWidth = utilGetMaxInt( _maxFieldWidth, (int)rcInfo.name.length() ) ;
      }
   }
   catch ( std::exception &e )
   {
      printLog( PD_ERROR ) << "Failed to parse xml: " << e.what() << endl ;
      rc = 1 ;
      goto error ;
   }

   for (map<int, vector<RCInfo> >::const_iterator itr = rclistWithTag.begin();
        itr != rclistWithTag.end(); ++itr)
   {
      const vector<RCInfo> &rl = itr->second;
      for (int i = 0; i < rl.size(); ++i)
      {
         _rcInfoList.push_back(rl.at(i));
      }
   }

done:
   return rc ;
error:
   goto done ;
}

/*
   type
      1: C-style comment
      2: python comment
*/
int rcGeneratorBase::_buildStatement( int type, string &headerDesc )
{
   int rc = 0 ;
   char buff[2048] = { 0 } ;
   vector<string> statementLines ;
   string comment ;

   headerDesc = "" ;

   if ( type == 1 )
   {
      comment = "//" ;
      headerDesc += GLOBAL_LICENSE2"\n\n" ;
   }
   else if ( type == 2 )
   {
      vector<string> licenseLines ;

      comment = "#" ;

      licenseLines = stringSplit( GLOBAL_LICENSE, "\n" ) ;
      for( int i = 0; i < licenseLines.size(); ++i )
      {
         headerDesc += comment + licenseLines.at( i ) + "\n" ;
      }

      headerDesc += "\n\n" ;
   }
   else
   {
      rc = 1 ;
      goto error ;
   }

   statementLines = stringSplit( DEFAULT_FILE_STATEMENT, "\n" ) ;
   for( int i = 0; i < statementLines.size(); ++i )
   {
      headerDesc += comment + statementLines.at( i ) + "\n" ;
   }

   rc = (int)utilSnprintf( buff, 2048, headerDesc.c_str(),
                           utilGetCurrentYear(), RC_DESC_PATH ) ;
   if ( rc < 0 )
   {
      rc = 1 ;
      goto error ;
   }

   rc = 0 ;
   headerDesc = buff ;

done:
   return rc ;
error:
   goto done ;
}
