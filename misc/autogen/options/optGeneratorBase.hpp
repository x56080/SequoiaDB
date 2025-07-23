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

   Source File Name = optGeneratorBase.hpp

   Descriptive Name = N/A

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who         Description
   ====== =========== =========== =============================================
          23/07/2025  fangjiabin  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef OPT_GENERATOR_HPP
#define OPT_GENERATOR_HPP

#include "../generateInterface.hpp"
#include <map>

// xml file
#define OPTLIST_FILENAME      "optlist.xml"
#define OPTLIST_FILE_PATH     "./"OPTLIST_FILENAME
#define OPTLIST_DESC_PATH     "sequoiadb/misc/autogen/"OPTLIST_FILENAME

typedef struct _optionEle
{
   bool hiddentag ;
   char shorttag ;
   string nametag ;
   string longtag ;
   string defttag ;
   string catatag ;
   string cordtag ;
   string datatag ;
   string standtag;
   string typetag ;
   string typeofweb ;

   map<string, string> desc ;
   map<string, string> detail ;
   map<string, string> reloadable ;
   map<string, string> reloadstrategy ;

   _optionEle() : hiddentag( false ),
                  shorttag( 0 )
   {
   }

   string getDesc( string lang ){ return desc[lang] ; }
   string getDetail( string lang ){ return detail[lang] ; }
   string getReloadable( string lang ){ return reloadable[lang] ; }
   string getReloadstrategy( string lang ){ return reloadstrategy[lang] ; }
} OPTION_ELE ;

class optGeneratorBase : public generateBase
{
public:
   optGeneratorBase() ;
   ~optGeneratorBase() ;
   virtual int init() ;
   virtual bool hasNext() = 0 ;
   virtual int outputFile( int id, fileOutStream &fout,
                           string &outputPath ) = 0;
   virtual const char* name() = 0 ;

protected:
   int _loadOptList( const char *path ) ;
   int _buildStatement( int type, string &headerDesc ) ;

protected:
   vector<OPTION_ELE> _optionList ;
} ;

#endif
