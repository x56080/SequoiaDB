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

   Source File Name = lobChunkHandler.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef VESSEL_LOB_CHUNK_HANDLER_H_
#define VESSEL_LOB_CHUNK_HANDLER_H_

#include "vessel/requestHandler.h"
#include "dmsLobDef.hpp"

namespace engine
{
namespace vessel
{
   class listLobChunkCursor;

   class lobChunkHandler : public requestHandler
   {
      public:
         lobChunkHandler(){}
         virtual ~lobChunkHandler(){}

      public:
         INT32 insert(const globalCollectionId &gcid,
                      const bson::OID &oid,
                      UINT32 chunkId,
                      UINT32 offset,
                      UINT32 size,
                      const CHAR *data);

         INT32 read(const globalCollectionId &gcid,
                    const bson::OID &oid,
                    UINT32 chunkId,
                    UINT32 offset,
                    UINT32 size,
                    CHAR *data,
                    UINT32 &readSize);

         INT32 remove(const globalCollectionId &gcid,
                      const bson::OID &oid,
                      UINT32 chunkId);

         INT32 update(const globalCollectionId &gcid,
                      const bson::OID &oid,
                      UINT32 chunkId,
                      UINT32 offset,
                      UINT32 size,
                      const CHAR *data,
                      BOOLEAN createIfNotExists);

         INT32 truncate(const globalCollectionId &gcid,
                        const bson::OID &oid,
                        UINT32 chunkId,
                        UINT32 size,
                        UINT32 &tsize);

         INT32 list(listLobChunkCursor *cursor);

         INT32 test(const globalCollectionId &gcid,
                    const bson::OID &oid,
                    UINT32 chunkId,
                    dmsLobChunkProfile *profile);
   };//class lobChunkHandler
} // namespace vessel

} // namespace engine


#endif//VESSEL_LOB_CHUNK_HANDLER_H_