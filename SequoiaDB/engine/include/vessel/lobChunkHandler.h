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

   Source File Name = lobChunkHandler.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_LOB_CHUNK_HANDLER_H_
#define VESSEL_LOB_CHUNK_HANDLER_H_

#include "vessel/requestHandler.h"

namespace engine
{
namespace vessel
{
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

   };//class lobChunkHandler
} // namespace vessel

} // namespace engine


#endif//VESSEL_LOB_CHUNK_HANDLER_H_