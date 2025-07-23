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

   Source File Name = optGenForSAC.hpp

   Descriptive Name = N/A

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who         Description
   ====== =========== =========== =============================================
          23/07/2025  fangjiabin  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef OPT_GEN_FOR_SAC_HPP
#define OPT_GEN_FOR_SAC_HPP

#include "optGeneratorBase.hpp"

#define OPT_SAC_SDB_BUZ_FILENAME       "sequoiadb_config"

#define OPT_SAC_SDB_XML_FILE_PATH      SAC_PATH"config/"OPT_SAC_SDB_BUZ_FILENAME".in"
#define OPT_SAC_SDB_CONFIG_FILE_PATH   SAC_PATH"config/"OPT_SAC_SDB_BUZ_FILENAME

#define OPT_FILTER_FOR_SAC_LIST \
"help",\
"version",\
"confpath",\
"omaddr",\
"catalogaddr",\

typedef struct _SAC_XML_ELE
{
   bool   hidden ;
   string name ;
   string type ;
   string defaultVal ;
   string valid ;
   string display ;
   int level ;

   map<string, string> webName ;
   map<string, string> desc ;
   map<string, string> reloadable ;
   map<string, string> reloadstrategy ;
} SAC_XML_ELE ;

class optGenForSAC : public optGeneratorBase
{
DECLARE_GENERATOR_AUTO_REGISTER() ;

public:
   optGenForSAC() ;
   ~optGenForSAC() ;

   int init() ;
   bool hasNext() ;
   int outputFile( int id, fileOutStream &fout, string &outputPath ) ;
   const char* name(){ return "options for SAC" ; }

private:
   int _getTemplateConfigIndex( const string &name ) ;
   int _mergeConfig() ;
   int _loadConfigTemplateWithSAC() ;

private:
   bool _isFinish ;
   vector<SAC_XML_ELE> _sacXmlList ;
} ;

#endif