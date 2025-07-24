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

   Source File Name = ILob.hpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          01/08/2024  ZHY Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef SDB_I_LOB_HPP_
#define SDB_I_LOB_HPP_

#include "sdbInterface.hpp"
#include "interface/ICursor.hpp"
#include "utilPooledObject.hpp"
#include "dmsLobDef.hpp"

namespace engine
{

class ILob : public _utilPooledObject, public std::enable_shared_from_this< ILob >
{
public:
   struct updatedInfo
   {
      BOOLEAN hasUpdated;
      INT64 increasedSize;
   };

public:
   ILob() = default;
   virtual ~ILob() = default;
   ILob( const ILob & ) = delete;
   ILob &operator=( const ILob & ) = delete;

   virtual ICollection *getCollPtr() = 0;
   virtual UINT64 fetchSnapshotID() = 0;

   virtual INT32 write( const dmsLobRecord &record, IExecutor *executor ) = 0;
   virtual INT32 update( const dmsLobRecord &record,
                         IExecutor *executor,
                         updatedInfo *info = nullptr ) = 0;
   virtual INT32 writeOrUpdate( const dmsLobRecord &record,
                                IExecutor *executor,
                                updatedInfo *info = nullptr ) = 0;
   virtual INT32 remove( const dmsLobRecord &record, IExecutor *executor ) = 0;
   virtual INT32 read( const dmsLobRecord &record,
                       IExecutor *executor,
                       void *buf,
                       UINT32 &readLen ) const = 0;
   virtual INT32 list( IExecutor *executor, unique_ptr< ILobCursor > &cursor ) = 0;
   virtual INT32 truncate( IExecutor *executor ) = 0;
   virtual INT32 compact( IExecutor *executor ) = 0;
   virtual INT32 validate( IExecutor *executor ) = 0;
};
} // namespace engine
#endif