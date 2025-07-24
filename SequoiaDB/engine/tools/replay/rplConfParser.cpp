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

   Source File Name = rplConfParser.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          01/04/2019  Linyoubin  Initial Draft

   Last Changed =

*******************************************************************************/
#include "rplConfParser.hpp"
#include "rplTableMapping.hpp"
#include "ossFile.hpp"
#include "utilJsonFile.hpp"
#include "rplConfDef.hpp"
#include "../bson/bson.hpp"
#include "oss.hpp"
#include <string>

using namespace std ;
using namespace bson;
using namespace engine ;

namespace replay
{
   rplConfParser::rplConfParser()
   {
   }

   rplConfParser::~rplConfParser()
   {
   }

   INT32 rplConfParser::init( const CHAR *confFile )
   {
      INT32 rc = SDB_OK ;
      ossFile file ;

      rc = file.open( confFile, OSS_READONLY, OSS_DEFAULTFILE ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to open ini file[%s], rc=%d",
                   confFile, rc ) ;

      rc = utilJsonFile::read( file, _conf ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to read json from conf file[%s], "
                   "rc=%d", confFile, rc) ;
   done:
      if ( file.isOpened() )
      {
         file.close() ;
      }

      return rc ;
   error:
      goto done ;
   }

   BSONObj rplConfParser::getConf() const
   {
      return _conf ;
   }

   string rplConfParser::getType() const
   {
      return _conf.getStringField( RPL_CONF_OUTPUT_TYPE ) ;
   }
}

