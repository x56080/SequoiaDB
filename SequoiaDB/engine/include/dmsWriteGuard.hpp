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

   Source File Name = dmsWriteGuard.hpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          11/20/2023  HGM Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef SDB_DMS_WRITE_GUARD_HPP_
#define SDB_DMS_WRITE_GUARD_HPP_

#include "ossUtil.hpp"
#include "interface/IStorageService.hpp"
#include "dms.hpp"
#include "ixm.hpp"
#include "ossRWMutex.hpp"
#include "dmsMetadata.hpp"
#include "dmsPersistUnit.hpp"
#include "pmdDummySession.hpp"

namespace engine
{

   // forward declaration
   class _pmdEDUCB ;
   class _dmsStorageDataCommon ;
   class _dmsMBContext ;

   /*
      _dmsDataWriteGuard define
    */
   class _dmsDataWriteGuard : public SDBObject
   {
   public:
      _dmsDataWriteGuard() ;
      _dmsDataWriteGuard( _dmsStorageDataCommon *su,
                          _dmsMBContext *mbContext,
                          _pmdEDUCB *cb,
                          BOOLEAN isEnabled = TRUE ) ;
      ~_dmsDataWriteGuard() ;

      void beforeWrite() ;
      void afterWrite() ;

      INT32 begin() ;
      INT32 commit() ;
      INT32 abort( BOOLEAN isForced = FALSE ) ;

      BOOLEAN isEnabled() const
      {
         return _isEnabled ;
      }

   protected:
      _dmsStorageDataCommon *_su = nullptr ;
      _dmsMBStatInfo *_mbStat = nullptr ;
      UINT16 _mbID ;
      _pmdEDUCB *_eduCB ;
      BOOLEAN _isEnabled ;
      BOOLEAN _isInWrite ;
   } ;

   typedef class _dmsDataWriteGuard dmsDataWriteGuard ;

   /*
      dmsIndexBuildLockPtr define
    */
   typedef std::shared_ptr<ossRWMutex> dmsIndexBuildLockPtr ;
   typedef ossPoolMap<dmsIdxMetadataKey, dmsIndexBuildLockPtr> dmsIdxBuildLockMap ;
   typedef dmsIdxBuildLockMap::iterator dmsIdxBuildLockMapIter ;
   /*
      _dmsIndexWriteGuard define
    */
   class _dmsIndexWriteGuard : public SDBObject
   {
   public:
      _dmsIndexWriteGuard() ;
      _dmsIndexWriteGuard( _pmdEDUCB *cb, BOOLEAN isEnabled = TRUE ) ;
      ~_dmsIndexWriteGuard() ;

      INT32 lock( const dmsIdxMetadataKey &metadataKey,
                  const ixmIndexCB &indexCB,
                  const dmsRecordID &rid,
                  dmsIndexBuildLockPtr &lockPtr,
                  BOOLEAN &needProcess ) ;
      void releaseAll() ;

      INT32 begin() ;
      INT32 commit() ;
      INT32 abort( BOOLEAN isForced = FALSE ) ;

      BOOLEAN isEnabled() const
      {
         return _isEnabled ;
      }

   protected:
      _pmdEDUCB *_eduCB ;
      BOOLEAN _isEnabled ;
      dmsIdxBuildLockMap _locks ;
   } ;

   typedef class _dmsIndexWriteGuard dmsIndexWriteGuard ;

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

      BOOLEAN useAtomicAbort() const
      {
         return _isEnabled &&
                _persistUnit != nullptr &&
                _persistUnit->useAtomicAbort() ;
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

   /*
      _dmsWriteGuard define
    */
   class _dmsWriteGuard : public SDBObject
   {
   public:
      _dmsWriteGuard() = default ;
      _dmsWriteGuard( IStorageService *service,
                      _dmsStorageDataCommon *su,
                      _dmsMBContext *mbContext,
                      _pmdEDUCB *cb,
                      BOOLEAN isDataWriteGuardEnabled = TRUE,
                      BOOLEAN isIndexWriteGuardEnabled = TRUE,
                      BOOLEAN isPersistGuardEnabled = TRUE ) ;

      ~_dmsWriteGuard() = default ;

      INT32 begin() ;
      INT32 commit() ;
      INT32 abort( BOOLEAN isForced = FALSE ) ;

      dmsDataWriteGuard &getDataWriteGuard()
      {
         return _dataGuard ;
      }

      dmsIndexWriteGuard &getIndexWriteGuard()
      {
         return _indexGuard ;
      }

      dmsPersistGuard &getPersistGuard()
      {
         return _persistGuard ;
      }

   protected:
      dmsDataWriteGuard _dataGuard ;
      dmsIndexWriteGuard _indexGuard ;
      dmsPersistGuard _persistGuard ;
   } ;

   typedef class _dmsWriteGuard dmsWriteGuard ;

}

#endif // SDB_DMS_WRITE_GUARD_HPP_
