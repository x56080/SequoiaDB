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

         virtual void commit() = 0;
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
         virtual ~contextCreate()
         {
            if ( UTIL_IS_VALID_CSUNIQUEID( _csUID ) )
            {
               _cm->removeTempUniqueID( _csUID );
            }
         }

         virtual void commit() override
         {
            if ( _desc )
            {
               _cm->addSuDescriptor( _desc );
            }
         }
         virtual void abort() override {}

      public:
         void setDescriptorToAddWhenCommit( const DMS_SU_DESCRIPTOR &desc )
         {
            _desc = desc;
         }

      private:
         std::unique_lock< std::mutex > _lock;
         utilCSUniqueID _csUID = UTIL_UNIQUEID_NULL;
         DMS_SU_DESCRIPTOR _desc = nullptr;
      };
      using CONTEXT_CREATE = std::unique_ptr< contextCreate >;

      class contextDrop : public _dmsSuConstraintMap::context
      {
      public:
         contextDrop( _dmsSuConstraintMap *cm, std::unique_lock< std::mutex > &&lock )
         : context( cm ), _lock( std::move( lock ) )
         {
         }
         virtual ~contextDrop() = default;

         virtual void commit() override
         {
            _cm->removeSuDescriptor( _name.c_str() );
         }
         virtual void abort() override {}

      public:
         void setDescriptorToRemoveWhenCommit( const CHAR *name )
         {
            _name = name;
         }

      private:
         std::unique_lock< std::mutex > _lock;
         ossPoolString _name;
      };
      using CONTEXT_DROP = std::unique_ptr< contextDrop >;

      class contextRename : public _dmsSuConstraintMap::context
      {
      public:
         contextRename( _dmsSuConstraintMap *cm,
                        std::unique_lock< std::mutex > &&lock1,
                        std::unique_lock< std::mutex > &&lock2,
                        utilCSUniqueID csUID )
         : context( cm )
         , _lock1( std::move( lock1 ) )
         , _lock2( std::move( lock2 ) )
         , _csUID( csUID )
         {
         }
         virtual ~contextRename()
         {
            if ( UTIL_IS_VALID_CSUNIQUEID( _csUID ) )
            {
               _cm->removeTempUniqueID( _csUID );
            }
         }

         void setDescriptorToUpdateWhenCommit( const CHAR *oldName,
                                               const DMS_SU_DESCRIPTOR &newDesc )
         {
            _oldName = oldName;
            _newDesc = newDesc;
         }

         virtual void commit() override
         {
            _cm->removeSuDescriptor( _oldName.c_str() );
            _cm->addSuDescriptor( _newDesc );
         }
         virtual void abort() override {}

      private:
         std::unique_lock< std::mutex > _lock1;
         std::unique_lock< std::mutex > _lock2;
         utilCSUniqueID _csUID = UTIL_UNIQUEID_NULL;
         ossPoolString _oldName;
         DMS_SU_DESCRIPTOR _newDesc = nullptr;
      };
      using CONTEXT_RENAME = std::unique_ptr< contextRename >;

   public:
      INT32 getSuDescriptor( const CHAR *name, DMS_SU_DESCRIPTOR & );
      INT32 getSuDescriptor( utilCSUniqueID csUID, DMS_SU_DESCRIPTOR & );

      INT32 addSuDescriptor( const DMS_SU_DESCRIPTOR & );

      void removeSuDescriptor( const CHAR *name );
      void removeSuDescriptor( utilCSUniqueID csUID );

      INT32 prepareToCreate( const CHAR *name, utilCSUniqueID csUID, CONTEXT_CREATE & );
      INT32 prepareToDrop( const CHAR *name, CONTEXT_DROP & );
      INT32 prepareToRename( const CHAR *oldName, const CHAR *newName, CONTEXT_RENAME & );

      void removeTempUniqueID( utilCSUniqueID csUID );

      void clear();

   private:
      INT32 _insertTempUniqueID( utilCSUniqueID csUID );

      INT32 _testSuDescriptor( const CHAR *name, BOOLEAN &exist );

      void _remove( const CHAR *name );
      void _remove( utilCSUniqueID csUID );

      std::mutex &_getHashLatch( const CHAR *name );
      UINT64 _getHashLatchPos( const CHAR *name );

   private:
      ossPoolMap< utilStringView, DMS_SU_DESCRIPTOR > _nameToDesc;
      ossPoolMap< utilCSUniqueID, DMS_SU_DESCRIPTOR > _UIDToDesc;
      ossSpinSLatch _mapLatch;

      ossPoolSet< utilCSUniqueID > _tempUniqueIDs;
      ossSpinXLatch _setLatch;

      static constexpr UINT32 HASH_LATCH_NUMBER = 32;
      static constexpr UINT64 HASH_SEED = 5;
      std::mutex _hashLatches[ HASH_LATCH_NUMBER ];
   };
   using dmsSuConstraintMap = _dmsSuConstraintMap;

} // namespace engine

#endif