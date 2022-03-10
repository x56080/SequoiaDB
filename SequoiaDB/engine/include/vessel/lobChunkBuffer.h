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

   Source File Name = lobChunkBuffer.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_LOB_CHUNK_BUFFER_H_
#define VESSEL_LOB_CHUNK_BUFFER_H_

#include "vessel/lobChunkKey.h"
#include <memory> //c++11

namespace engine
{
namespace vessel
{
   class lobChunkBuffer : public SDBObject
   {
      public:
         lobChunkBuffer();
         ~lobChunkBuffer();
         lobChunkBuffer(const lobChunkBuffer &) = delete;
         lobChunkBuffer &operator=(const lobChunkBuffer &) = delete;

      private:
         lobChunkKey _key;
         UINT32 _hash = 0;
   };//class lobChunkBuffer

   typedef class std::shared_ptr<lobChunkBuffer> lobChunkBufferPtr;
} // namespace vessel

} // namespace engine


#endif//VESSEL_LOB_CHUNK_BUFFER_H_