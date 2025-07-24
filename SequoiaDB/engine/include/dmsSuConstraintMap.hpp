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

   Source File Name = dmsSuConstraintMap.hpp

   Descriptive Name = Data Management Service SU Constraint Map

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          12/14/2022  ZHY Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef DMS_SU_CONSTRAINT_MAP_HPP__
#define DMS_SU_CONSTRAINT_MAP_HPP__

#include "dmsSuDescriptor.hpp"
#include "utilStringView.hpp"
#include <mutex>
#include <atomic>

namespace engine
{
   class _dmsSuConstraintMap : public SDBObject
   {
      public:
         class context : public SDBObject
         {
            public:
               context( _dmsSuConstraintMap *cm ) : _cm( cm ) {}
               virtual ~context(){};

               virtual void abort() = 0;

            protected:
               _dmsSuConstraintMap *_cm = nullptr;
         };
         using CONTEXT_PTR = std::unique_ptr< context >;

         class contextCreate : public _dmsSuConstraintMap::context
         {
            public:
               contextCreate( _dmsSuConstraintMap *cm,
                              std::unique_lock< std::mutex > &&lock,
                              utilCSUniqueID csUID )
               : context( cm ), _lock( std::move( lock ) ), _csUID( csUID )
               {
               }
               virtual ~contextCreate();

               virtual void abort() override;

            public:
               void commit( const DMS_SU_DESCRIPTOR &desc );

            private:
               void _release();

            private:
               std::unique_lock< std::mutex > _lock;
               utilCSUniqueID _csUID = UTIL_UNIQUEID_NULL;
         };
         using CONTEXT_CREATE = std::unique_ptr< contextCreate >;

         class contextDrop : public _dmsSuConstraintMap::context
         {
            public:
               contextDrop( _dmsSuConstraintMap *cm,
                            std::unique_lock< std::mutex > &&lock,
                            const DMS_SU_DESCRIPTOR &desc )
               : context( cm ), _lock( std::move( lock ) ), _desc( desc )
               {
                  SDB_ASSERT( desc && desc->isValid(), "must be valid" );
               }
               virtual ~contextDrop();

               virtual void abort() override;

            public:
               const DMS_SU_DESCRIPTOR &getDescriptor() const;

               void commit();

            private:
               void _release();

            private:
               std::unique_lock< std::mutex > _lock;
               DMS_SU_DESCRIPTOR _desc;
         };
         using CONTEXT_DROP = std::unique_ptr< contextDrop >;

         class contextRename : public _dmsSuConstraintMap::context
         {
            public:
               contextRename( _dmsSuConstraintMap *cm,
                              std::unique_lock< std::mutex > &&lock1,
                              std::unique_lock< std::mutex > &&lock2,
                              utilCSUniqueID csUID,
                              const DMS_SU_DESCRIPTOR &oldDesc )
               : context( cm )
               , _lock1( std::move( lock1 ) )
               , _lock2( std::move( lock2 ) )
               , _csUID( csUID )
               , _oldDesc( oldDesc )
               {
               }
               virtual ~contextRename();

               virtual void abort() override;

            public:
               const DMS_SU_DESCRIPTOR &getOldDescriptor() const;

               void commit( const DMS_SU_DESCRIPTOR &newDesc );

            private:
               void _release();

            private:
               std::unique_lock< std::mutex > _lock1;
               std::unique_lock< std::mutex > _lock2;
               utilCSUniqueID _csUID = UTIL_UNIQUEID_NULL;
               DMS_SU_DESCRIPTOR _oldDesc = nullptr;
         };
         using CONTEXT_RENAME = std::unique_ptr< contextRename >;

         class contextChangeUniqueID : public _dmsSuConstraintMap::context
         {
            public:
               contextChangeUniqueID( _dmsSuConstraintMap *cm,
                                      std::unique_lock< std::mutex > &&lock,
                                      utilCSUniqueID csUID );

               virtual ~contextChangeUniqueID();

               virtual void abort() override;

            public:
               const DMS_SU_DESCRIPTOR &getOldDescriptor() const;

               void commit( const DMS_SU_DESCRIPTOR &newDesc );

            private:
               void _release();

            private:
               std::unique_lock< std::mutex > _lock;
               utilCSUniqueID _csUID = UTIL_UNIQUEID_NULL;
               DMS_SU_DESCRIPTOR _oldDesc = nullptr;
         };
         using CONTEXT_CHANGE_UNIQUE_ID = std::unique_ptr< contextChangeUniqueID >;

      public:
         DMS_SU_DESCRIPTOR getSuDescriptor( const utilStringView &name );
         DMS_SU_DESCRIPTOR getSuDescriptor( const CHAR *name );
         DMS_SU_DESCRIPTOR getSuDescriptor( utilCSUniqueID csUID );

         INT32 addSuDescriptor( const DMS_SU_DESCRIPTOR & );

         void removeSuDescriptor( const CHAR *name );
         void removeSuDescriptor( utilCSUniqueID csUID );

         INT32 prepareToCreate( const CHAR *name, utilCSUniqueID csUID, CONTEXT_CREATE & );
         INT32 prepareToDrop( const CHAR *name, CONTEXT_DROP & );
         INT32 prepareToRename( const CHAR *oldName, const CHAR *newName, CONTEXT_RENAME & );
         INT32 prepareToChangeUniqueID( const CHAR *name,
                                        utilCSUniqueID newCSUID,
                                        CONTEXT_CHANGE_UNIQUE_ID & );

         void removeTempUniqueID( utilCSUniqueID csUID );

         void clear();

         UINT32 getNullCSUniqueID() const;

      private:
         INT32 _insertTempUniqueID( utilCSUniqueID csUID );

         // If collection space name or unique id is used, exist is true
         INT32 _testSuDescriptor( const CHAR *name, utilCSUniqueID csUID, BOOLEAN &exist );

         INT32 _upsert( const DMS_SU_DESCRIPTOR &dms);
         void _remove( const CHAR *name );
         void _remove( utilCSUniqueID csUID );

         std::mutex &_getHashLatch( const CHAR *name );
         UINT64 _getHashLatchPos( const CHAR *name );
         void _getOrderedLatches( const CHAR *name1,
                                  const CHAR *name2,
                                  std::mutex **latchSmaller,
                                  std::mutex **latchLarger );

         void _nullCSUniqueIDCntInc();
         void _nullCSUniqueIDCntDec();

      private:
         ossPoolMap< utilStringView, DMS_SU_DESCRIPTOR > _nameToDesc;
         ossPoolMap< utilCSUniqueID, DMS_SU_DESCRIPTOR > _UIDToDesc;
         ossSpinSLatch _mapLatch;

         ossPoolSet< utilCSUniqueID > _tempUniqueIDs;
         ossSpinXLatch _setLatch;

         static constexpr UINT32 HASH_LATCH_NUMBER = 32;
         static constexpr UINT64 HASH_SEED = 5;
         std::mutex _hashLatches[ HASH_LATCH_NUMBER ];

         std::atomic< UINT32 > _nullCSUniqueIDCnt;
   };
   using dmsSuConstraintMap = _dmsSuConstraintMap;

} // namespace engine

#endif