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

   Source File Name = dpsRequest.hpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef DPS_REQUEST_HPP_
#define DPS_REQUEST_HPP_

#include "dpsLogRecord.hpp"
#include "utilFragAllocator.hpp"
#include "utilSlice.hpp"

namespace engine
{
   struct dpsWriteOptions
   {
      OSS_INLINE BOOLEAN hasTransTime()const
      {
         return DPS_INVALID_TRANS_TIME != transTime;
      }

      INT32 csid = -1;
      INT32 clid = -1;
      INT32 extentPos = -1;
      UINT64 transTime = DPS_INVALID_TRANS_TIME;
      BOOLEAN notify = FALSE;
      BOOLEAN transEnabled = FALSE;
      BOOLEAN flushAtOnce = FALSE;
      IExecutor *executor = nullptr;
   };//struct dpsWriteOptions

   class dpsWriteRequest : public SDBObject
   {
      friend class dpsWriteReqBuilder;
      public:
         dpsWriteRequest() = default;
         ~dpsWriteRequest() = default;
         dpsWriteRequest(const dpsWriteRequest &) = delete;
         dpsWriteRequest &operator=(const dpsWriteRequest &) = delete;
         dpsWriteRequest(dpsWriteRequest &&);
         dpsWriteRequest &operator=(dpsWriteRequest &&);

      public:
         OSS_INLINE DPS_LOG_TYPE getType()const {return _type;}
         OSS_INLINE UINT16 getFlags()const {return _flags;}
         OSS_INLINE UINT32 getElementDataSize()const {return _totalDataSize;}
         OSS_INLINE UINT32 getElementNum()const {return _elementNum;}
         const utilSlice &getElement(UINT32 pos)const;
         BOOLEAN seek(DPS_TAG tag, utilSlice &data)const;
         void reset();

      private:
         DPS_LOG_TYPE _type = LOG_TYPE_DUMMY;
         UINT16 _flags = 0;
         UINT32 _totalDataSize = 0;
         UINT32 _elementNum = 0;
         utilSlice *_elements = nullptr;
         utilFragAllocator::repertory _rep;
   };//class dpsWriteRequest

   struct dpsSearchOptions
   {
      BOOLEAN searchMem = TRUE;
      BOOLEAN searchFile = TRUE;
      BOOLEAN onlyHeader = FALSE;
      INT32 limits = 1;
      INT32 maxTime = -1;
      INT32 maxSize = 5242880;
   };//struct dpsSearchOptions
} // namespace engine


#endif//DPS_WRITE_REQUEST_HPP_