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

   Source File Name = dpsWriteContext.hpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef DPS_WRITE_CONTEXT_HPP_
#define DPS_WRITE_CONTEXT_HPP_

#include "dpsPageMeta.hpp"
#include "dpsRequest.hpp"

namespace engine
{
   class dpsWriteContext : public SDBObject
   {
      public:
         dpsWriteContext(IExecutor *executor,
                         const dpsWriteRequest *req,
                         const dpsWriteOptions *o);
         ~dpsWriteContext() = default;

      public:
         OSS_INLINE const dpsWriteRequest *getReq()const {return _req;}
         OSS_INLINE const dpsWriteOptions *getOptions()const {return _o;}
         OSS_INLINE UINT32 getOriginalEleSize()const {return _req->getElements().getSize();}
         OSS_INLINE BOOLEAN isDummyRecordFilled()const {return 0 < _dummyRecord._length;}
         OSS_INLINE dpsLogRecordHeader &getDummmyRecord() {return _dummyRecord;}
         OSS_INLINE dpsPageMeta &getDummyPageMeta() {return _dummyPageMeta;}

         OSS_INLINE BOOLEAN isIrreversible()const {return _irreversible;}
         OSS_INLINE dpsLogRecordHeader &getRecord() {return _record;}
         OSS_INLINE const dpsLogRecordHeader &getRecord()const {return _record;}
         OSS_INLINE dpsPageMeta &getPageMeta() {return _pageMeta;}
         OSS_INLINE const dpsPageMeta &getPageMeta() const {return _pageMeta;}
         OSS_INLINE void setCompressedRecord( utilUniqueBuffer &&b )
         {
            _compressedRecord = std::move( b ) ;
         }

      public:
         UINT32 getRecordBodySizeAuto() const ;
         utilSlice getRecordBodyData() const ;
         
      private:
         void _init();

      private:
         IExecutor *_executor = nullptr;
         const dpsWriteRequest *_req = nullptr;
         const dpsWriteOptions *_o = nullptr;
         BOOLEAN _irreversible = FALSE;
         dpsLogRecordHeader _dummyRecord;
         dpsPageMeta _dummyPageMeta;
         dpsLogRecordHeader _record;
         dpsPageMeta _pageMeta;
         utilUniqueBuffer _compressedRecord ;
   };//class dpsWriteContext
} // namespace engine


#endif//DPS_WRITE_CONTEXT_HPP_