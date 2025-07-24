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

   Source File Name = dmsPersistGuard.hpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          11/20/2023  HGM Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef SDB_DMS_PERSIST_GUARD_HPP_
#define SDB_DMS_PERSIST_GUARD_HPP_

#include "dmsPersistUnit.hpp"
#include "ossUtil.hpp"
#include "interface/IStorageService.hpp"
#include "dms.hpp"
#include "dmsMetadata.hpp"
#include "pmdDummySession.hpp"

namespace engine
{

   // forward declaration
   class _pmdEDUCB ;
   class _dmsStorageDataCommon ;
   class _dmsMBContext ;

   /*
      _dmsPersistGuard define
    */
   class _dmsPersistGuard : public SDBObject
   {
   public:
      _dmsPersistGuard() ;
      _dmsPersistGuard( IStorageService *service,
                        _dmsStorageDataCommon *su,
                        _dmsMBContext *mbContext,
                        _pmdEDUCB *cb,
                        BOOLEAN isEnabled = TRUE ) ;
      ~_dmsPersistGuard() ;

      BOOLEAN isEnabled() const
      {
         return _isEnabled ;
      }

      INT32 init() ;
      INT32 fini() ;

      INT32 begin() ;
      INT32 commit() ;
      INT32 abort( BOOLEAN isForced = FALSE ) ;

      void incRecordCount( UINT64 count = 1 ) ;
      void decRecordCount( UINT64 count = 1 ) ;
      void incDataLen( UINT64 dataLen ) ;
      void decDataLen( UINT64 dataLen ) ;
      void incOrgDataLen( UINT64 orgDataLen ) ;
      void decOrgDataLen( UINT64 orgDataLen ) ;

   protected:
      IStorageService *_service = nullptr ;
      IPersistUnit *_persistUnit = nullptr ;
      _dmsStorageDataCommon *_su = nullptr ;
      _dmsMBStatInfo *_mbStat = nullptr ;
      pmdDummySession _dummySession ;
      utilCLUniqueID _clUniqueID = UTIL_UNIQUEID_NULL ;
      utilThreadLocalPtr<dmsStatPersistUnit> _statUnitPtr ;
      _pmdEDUCB *_eduCB ;
      BOOLEAN _isEnabled ;
      BOOLEAN _hasBegin ;
   } ;

   typedef class _dmsPersistGuard dmsPersistGuard ;

}

#endif // SDB_DMS_PERSIST_GUARD_HPP_
