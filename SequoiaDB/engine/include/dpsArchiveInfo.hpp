/*******************************************************************************


   Copyright (C) 2011-2018 SequoiaDB Ltd.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

   http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

   Source File Name = dpsArchiveInfo.hpp

   Descriptive Name = Data Protection Services Log Archive Info

   When/how to use: this program may be used on binary and text-formatted
   versions of DPS component. This file contains code logic for log page
   operations

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          7/28/2016  David Li  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef DPSARCHIVEINFO_HPP_
#define DPSARCHIVEINFO_HPP_

#include "dpsLogDef.hpp"
#include "ossFile.hpp"
#include "ossLatch.hpp"
#include "../bson/bsonobj.h"
#include <string>

using namespace std ;
using namespace bson ;

namespace engine
{
   struct dpsArchiveInfo: public SDBObject
   {
      DPS_LSN startLSN ;

      dpsArchiveInfo& operator=( const dpsArchiveInfo& info )
      {
         startLSN = info.startLSN ;
         return *this ;
      }
   } ;

   class dpsArchiveInfoMgr: public SDBObject
   {
   public:
      dpsArchiveInfoMgr() ;
      ~dpsArchiveInfoMgr() ;
      INT32 init( const CHAR* archivePath ) ;
      dpsArchiveInfo getInfo() ;
      INT32 updateInfo( dpsArchiveInfo& info ) ;

   private:
      INT32 _initInfo() ;
      INT32 _open( const string& fileName, ossFile& file ) ;
      INT32 _toBson( BSONObj& data ) ;
      INT32 _fromBson( const BSONObj& data, 
                       dpsArchiveInfo& info,
                       INT64& count ) ;

   private:
      string         _path ;
      ossFile        _file1 ;
      ossFile        _file2 ;
      dpsArchiveInfo _info ;
      _ossSpinXLatch _mutex ;
      INT64          _count ;
   } ;
}

#endif /* DPSARCHIVEINFO_HPP_ */

