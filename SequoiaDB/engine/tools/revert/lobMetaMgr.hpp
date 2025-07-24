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

   Source File Name = lobMetaMgr.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          06/12/2023  Yang Qincheng  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef REVERT_LOB_MATE_MGR_HPP_
#define REVERT_LOB_MATE_MGR_HPP_

#include "ossTypes.hpp"
#include "../bson/bson.hpp"
#include "dpsDef.hpp"
#include "ossLatch.hpp"
#include <map>
#include <vector>

using namespace std ;

namespace sdbrevert
{
   class lobMeta : public SDBObject
   {
      public:
         lobMeta( const bson::OID &oid, UINT32 sequence ) ;
         ~lobMeta() ;
         BOOLEAN operator<( const lobMeta& other ) const ;

      private:
         bson::OID _oid ;
         UINT32    _sequence ;
   } ;

   class lobMetaMgr : public SDBObject
   {
      public:
         lobMetaMgr() ;
         ~lobMetaMgr() ;
         BOOLEAN checkLSN( const bson::OID &oid, UINT32 sequence, DPS_LSN_OFFSET lsn ) ;

      private:
         map<lobMeta, DPS_LSN_OFFSET>   _lobMetaMap ;
         ossSpinXLatch                  _mutex ;
   } ;

   class lobLockMap : public SDBObject
   {
      public:
         lobLockMap( UINT32 size ) ;
         ~lobLockMap() ;
         INT32 getLobLock( const bson::OID &oid, ossSpinXLatch *&lobLock ) ;

      private:
         UINT32                  _size ;
         vector<ossSpinXLatch*>  _lobLockVec ;
   } ;
}

#endif /* REVERT_LOB_MATE_MGR_HPP_ */
