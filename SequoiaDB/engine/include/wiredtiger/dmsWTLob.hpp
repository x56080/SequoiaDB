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

   Source File Name = dmsWTIndex.hpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          01/08/2024  ZHY Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef DMS_WT_LOB_HPP_
#define DMS_WT_LOB_HPP_

#include "dmsMetadata.hpp"
#include "interface/ILob.hpp"
#include "wiredtiger/dmsWTStoreHolder.hpp"

namespace engine
{
namespace wiredtiger
{
   class _dmsWTLob : public ILob, public _dmsWTStoreHolder
   {
   public:
      _dmsWTLob( dmsWTStorageEngine &engine, const dmsWTStore &store, ICollection *collPtr )
      : dmsWTStoreHolder( engine, store ), _collPtr( collPtr )
      {
      }

      static INT32 buildLobURI( utilCSUniqueID csUID,
                                utilCLInnerID clInnerID,
                                UINT32 clLID,
                                ossPoolString &uri );

      virtual ICollection *getCollPtr() override;
      virtual UINT64 fetchSnapshotID() override;

      virtual INT32 write( const dmsLobRecord &record, IExecutor *executor ) override;
      virtual INT32 update( const dmsLobRecord &record,
                            IExecutor *executor,
                            updatedInfo *info ) override;
      virtual INT32 writeOrUpdate( const dmsLobRecord &record,
                                   IExecutor *executor,
                                   updatedInfo *info ) override;
      virtual INT32 remove( const dmsLobRecord &record, IExecutor *executor ) override;
      virtual INT32 read( const dmsLobRecord &record,
                          IExecutor *executor,
                          void *buf,
                          UINT32 &readLen ) const override;
      virtual INT32 list( IExecutor *executor, unique_ptr< ILobCursor > &cursor ) override;
      virtual INT32 truncate( IExecutor *executor ) override;
      virtual INT32 compact( IExecutor *executor ) override;
      virtual INT32 validate( IExecutor *executor ) override;

   private:
      ICollection *_collPtr = nullptr;
   };
   typedef _dmsWTLob dmsWTLob;
} // namespace wiredtiger
} // namespace engine
#endif