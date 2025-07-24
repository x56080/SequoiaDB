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

   Source File Name = dpsRequest.hpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef DPS_REQUEST_HPP_
#define DPS_REQUEST_HPP_

#include "dpsLogRecord.hpp"
#include "dpsRecordElements.hpp"

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
      BOOLEAN compress = TRUE ;
   };//struct dpsWriteOptions

   class dpsWriteRequest : public SDBObject
   {
      friend class dpsWriteReqBuilder;
      public:
         dpsWriteRequest() = default;
         ~dpsWriteRequest() = default;
         dpsWriteRequest(const dpsWriteRequest &) = delete;
         dpsWriteRequest &operator=(const dpsWriteRequest &) = delete;
         dpsWriteRequest(dpsWriteRequest &&) noexcept;
         dpsWriteRequest &operator=(dpsWriteRequest &&) noexcept;

      public:
         OSS_INLINE DPS_LOG_TYPE getType()const {return _type;}
         OSS_INLINE UINT16 getFlags()const {return _flags;}
         OSS_INLINE UINT32 getElementDataSize() const { return _body.getSize(); }
         const dpsRecordElements &getElements() const { return _body ; }
         BOOLEAN seek(DPS_TAG tag, utilSlice &value) const;
         void reset();

      private:
         DPS_LOG_TYPE _type = LOG_TYPE_DUMMY;
         UINT16 _flags = 0;
         dpsRecordElements _body ;
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