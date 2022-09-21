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

   Source File Name = dpsWriteRequest.hpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef DPS_WRITE_REQUEST_HPP_
#define DPS_WRITE_REQUEST_HPP_

#include "dpsLogRecord.hpp"

namespace engine
{
   class dpsWriteRequest : public SDBObject
   {
      friend class dpsWriteReqBuilder;
      public:
         dpsWriteRequest() = default;
         ~dpsWriteRequest();
         dpsWriteRequest(const dpsWriteRequest &) = delete;
         dpsWriteRequest &operator=(const dpsWriteRequest &) = delete;
         dpsWriteRequest(dpsWriteRequest &&);
         dpsWriteRequest &operator=(dpsWriteRequest &&);

      public:
         OSS_INLINE DPS_LOG_TYPE getType()const {return _type;}
         OSS_INLINE UINT16 getFlags()const {return _flags;}
         OSS_INLINE UINT32 getBufferSize()const {return _bufferSize;}
         OSS_INLINE UINT32 getElementNum()const {return _elementNum;}
         OSS_INLINE const CHAR *getElementsBuffer()const {return _buffer;}
         OSS_INLINE BOOLEAN isOwned()const {return _buffer == _bufferOwned;}
         void reset();

      private:
         DPS_LOG_TYPE _type = LOG_TYPE_DUMMY;
         UINT16 _flags = 0;
         UINT32 _elementNum = 0;
         UINT32 _bufferSize = 0;/// it is not the real buffer size, it is element data size.
         const CHAR *_buffer = nullptr;
         CHAR *_bufferOwned = nullptr;
   };//class dpsWriteRequest
} // namespace engine


#endif//DPS_WRITE_REQUEST_HPP_