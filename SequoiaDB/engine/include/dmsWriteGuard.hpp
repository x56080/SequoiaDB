/*******************************************************************************


   Copyright (C) 2011-2018 SequoiaDB Ltd.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU Affero General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.

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
#include "ossRWMutex.hpp"
#include "dmsMetadata.hpp"

namespace engine
{

   // forward declaration
   class _pmdEDUCB ;
   class _dmsStorageBase ;
   class _dmsMBContext ;

   /*
      _dmsDataWriteGuard define
    */
   class _dmsDataWriteGuard : public SDBObject
   {
   public:
      _dmsDataWriteGuard() ;
      _dmsDataWriteGuard( _dmsStorageBase *su,
                          _dmsMBContext *mbContext,
                          _pmdEDUCB *cb,
                          BOOLEAN isEnabled = TRUE ) ;
      ~_dmsDataWriteGuard() ;

      void beforeWrite() ;
      void afterWrite() ;

      INT32 begin( _dmsStorageBase *su,
                   _dmsMBContext *mbContext,
                   _pmdEDUCB *cb,
                   BOOLEAN isEnabled = TRUE ) ;
      INT32 begin() ;
      INT32 commit() ;
      INT32 abort( BOOLEAN isForced = FALSE ) ;

      BOOLEAN isEnabled() const
      {
         return _isEnabled ;
      }

   protected:
      _dmsStorageBase *_su = nullptr ;
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
                  dmsIndexBuildLockPtr &lockPtr ) ;
      void releaseAll() ;

      INT32 begin( _pmdEDUCB *cb, BOOLEAN isEnabled = TRUE ) ;
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
                        _pmdEDUCB *cb,
                        BOOLEAN isEnabled = TRUE ) ;
      ~_dmsPersistGuard() ;

      BOOLEAN isGuardEnabled() const
      {
         return _isEnabled ;
      }

      BOOLEAN useAtomicAbort() const
      {
         return _isEnabled &&
                _persistUnit != nullptr &&
                _persistUnit->useAtomicAbort() ;
      }

      INT32 begin( IStorageService *service,
                   _pmdEDUCB *cb,
                   BOOLEAN isEnabled = TRUE ) ;
      INT32 begin() ;
      INT32 commit() ;
      INT32 abort( BOOLEAN isForced = FALSE ) ;

   protected:
      IStorageService *_service ;
      IPersistUnit *_persistUnit ;
      _pmdEDUCB *_eduCB ;
      BOOLEAN _isEnabled ;
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
                      _dmsStorageBase *su,
                      _dmsMBContext *mbContext,
                      _pmdEDUCB *cb,
                      BOOLEAN isDataWriteGuardEnabled = TRUE,
                      BOOLEAN isIndexWriteGuardEnabled = TRUE,
                      BOOLEAN isPersistGuardEnabled = TRUE ) ;

      ~_dmsWriteGuard() = default ;

      INT32 begin( IStorageService *service,
                   _dmsStorageBase *su,
                   _dmsMBContext *mbContext,
                   _pmdEDUCB *cb,
                   BOOLEAN isDataWriteGuardEnabled = TRUE,
                   BOOLEAN isIndexWriteGuardEnabled = TRUE,
                   BOOLEAN isPersistGuardEnabled = TRUE ) ;
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
