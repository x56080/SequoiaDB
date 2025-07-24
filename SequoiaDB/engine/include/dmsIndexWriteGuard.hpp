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

   Source File Name = dmsIndexWriteGuard.hpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          11/20/2023  HGM Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef SDB_DMS_INDEX_WRITE_GUARD_HPP_
#define SDB_DMS_INDEX_WRITE_GUARD_HPP_

#include "ossUtil.hpp"
#include "dmsIndexBuildGuard.hpp"

namespace engine
{

   // forward declaration
   class _pmdEDUCB ;
   class _dmsStorageDataCommon ;
   class _dmsMBContext ;

   /*
      _dmsIndexWriteGuard define
    */
   class _dmsIndexWriteGuard : public SDBObject
   {
   public:
      _dmsIndexWriteGuard() ;
      _dmsIndexWriteGuard( _pmdEDUCB *cb,
                           BOOLEAN isEnabled = TRUE ) ;
      ~_dmsIndexWriteGuard() ;

      INT32 lock( const dmsIdxMetadataKey &metadataKey,
                  UINT32 indexID,
                  const ixmIndexCB &indexCB,
                  const dmsRecordID &rid,
                  dmsIndexBuildGuardPtr &guardPtr ) ;

      INT32 begin() ;
      INT32 commit() ;
      INT32 abort( BOOLEAN isForced = FALSE ) ;

      BOOLEAN isEnabled() const
      {
         return _isEnabled ;
      }

      BOOLEAN checkNeedProcess( UINT32 indexID )
      {
         return _processMap.testBit( indexID ) ;
      }

      void setNeedProcess( UINT32 indexID )
      {
         _processMap.setBit( indexID ) ;
      }

      BOOLEAN isSet( const dmsIdxMetadataKey &metadataKey,
                     const dmsRecordID &rid ) ;

   protected:
      _pmdEDUCB *_eduCB ;
      BOOLEAN _isEnabled ;
      dmsRecordID _rid ;
      typedef ossPoolMap<dmsIdxMetadataKey,
                         dmsIndexBuildGuardPtr> _dmsIdxBuildGuardRIDMap ;
      typedef _dmsIdxBuildGuardRIDMap::iterator _dmsRIDIdxBuildGuardMapIter ;
      _dmsIdxBuildGuardRIDMap _guards ;
      _utilStackBitmap<DMS_COLLECTION_MAX_INDEX> _processMap ;
   } ;

   typedef class _dmsIndexWriteGuard dmsIndexWriteGuard ;

}

#endif // SDB_DMS_INDEX_WRITE_GUARD_HPP_
