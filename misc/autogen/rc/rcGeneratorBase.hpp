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

   Source File Name = rcGeneratorBase.hpp

   Descriptive Name = N/A

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who         Description
   ====== =========== =========== =============================================
          23/07/2025  fangjiabin  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef RC_GENERATOR_HPP
#define RC_GENERATOR_HPP

#include "../generateInterface.hpp"

// xml file
#define RC_FILENAME        "rclist.xml"
#define RC_FILE_PATH       "./"RC_FILENAME
#define RC_DESC_PATH       "sequoiadb/misc/autogen/"RC_FILENAME

struct RCInfo
{
   int    value ;
   bool   reserved ;
   string name ;
   string desc_en ;
   string desc_cn ;

   RCInfo(): value( 0 ),
             reserved( false )
   {
   }

   string getDesc( string lang ) const
   {
      if( "cn" == lang )
      {
         return desc_cn ;
      }
      else if( "en" == lang )
      {
         return desc_en ;
      }
      else
      {
         return "invalid lang" ;
      }
   }
} ;

class rcGeneratorBase : public generateBase
{
public:
   rcGeneratorBase() ;
   ~rcGeneratorBase() ;
   virtual int init() ;
   virtual bool hasNext() = 0 ;
   virtual int outputFile( int id, fileOutStream &fout,
                           string &outputPath ) = 0;
   virtual const char* name() = 0 ;

protected:
   int _loadRcList() ;
   int _buildStatement( int type, string &headerDesc ) ;

protected:
   int _maxFieldWidth ;

   vector<pair<string, int> > _conslist ;
   vector<RCInfo> _rcInfoList ;
} ;

#endif
