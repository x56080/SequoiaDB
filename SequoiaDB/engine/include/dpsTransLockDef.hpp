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

   Source File Name = dpsTransLockDef.hpp

   Descriptive Name = Operating System Services Types Header

   When/how to use: this program may be used on binary and text-formatted
   versions of OSS component. This file contains declare for data types used in
   SequoiaDB.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/14/2012  YW  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef DPSTRANSLOCKDEF_HPP_
#define DPSTRANSLOCKDEF_HPP_

#include "ossTypes.h"
#include <map>
#include <string>
#include "dms.hpp"
#include "ossAtomic.hpp"
#include "msg.h"
#include "../util/fromjson.hpp"

namespace engine
{
   class _pmdEDUCB;

   enum DPS_TRANSLOCK_TYPE
   {
      // note: don't modify the enum lightly,
      // check the fun( dpsTransLock::upgradeCheck
      // and dpsLockBucket::isLockCompatible ) before modify
      DPS_TRANSLOCK_IS = 0,
      DPS_TRANSLOCK_IX,
      DPS_TRANSLOCK_S,
      DPS_TRANSLOCK_X
   };

   /*enum DPS_TRANSLOCK_STATUS
   {
      DPS_TRANSLOCK_GOT = 0,
      DPS_TRANSLOCK_WAIT
   };*/

   typedef std::map<UINT32, MsgRouteID>      DpsTransNodeMap;

   class dpsTransLockId : public SDBObject
   {
   public:
      dpsTransLockId( UINT32 logicCSID,
                      UINT16 collectionID,
                      const _dmsRecordID *recordID );
      dpsTransLockId();
      ~dpsTransLockId();

      BOOLEAN operator<( const dpsTransLockId &rhs ) const;

      BOOLEAN operator==( const dpsTransLockId &rhs ) const;

      dpsTransLockId & operator=( const dpsTransLockId & rhs ) ;

      std::string toString() const;

      bson::BSONObj toBson() const ;

      void        reset() ;
      BOOLEAN     isValid() const ;

   public:
      UINT32               _logicCSID;
      dmsExtentID          _recordExtentID;
      dmsOffset            _recordOffset;
      UINT16               _collectionID;
   };

   class dpsTransCBLockInfo : public SDBObject
   {
   public:
      dpsTransCBLockInfo( DPS_TRANSLOCK_TYPE lockType );
      ~dpsTransCBLockInfo();
      INT64 incRef();
      INT64 decRef();
      BOOLEAN isLockMatch( DPS_TRANSLOCK_TYPE type );
      DPS_TRANSLOCK_TYPE getType();
      void setType( DPS_TRANSLOCK_TYPE lockType );
      _pmdEDUCB *getNextWaitCB();
      void setNextWaitCB( _pmdEDUCB *pWaitCB );
   private:
      _pmdEDUCB                  *_pNextWaitCB ;
      DPS_TRANSLOCK_TYPE         _lockType ;
      ossAtomicSigned64          *_pRef;
   };

   typedef std::map< dpsTransLockId, dpsTransCBLockInfo * >    DpsTransCBLockList;

}

#endif // DPSTRANSLOCKDEF_HPP_
