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

   Source File Name = dpsWriteContext.hpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef DPS_WRITE_CONTEXT_HPP_
#define DPS_WRITE_CONTEXT_HPP_

#include "dpsPageMeta.hpp"
#include "dpsRequest.hpp"

namespace engine
{
   class dpsWriteContext : public SDBObject
   {
      public:
         dpsWriteContext(const dpsWriteRequest *req,
                         const dpsWriteOptions *o);
         ~dpsWriteContext() = default;

      public:
         OSS_INLINE const dpsWriteRequest *getReq()const {return _req;}
         OSS_INLINE const dpsWriteOptions *getOptions()const {return _o;}
         OSS_INLINE UINT32 getElementDataSize()const {return _req->getElements().getSize();}
         OSS_INLINE BOOLEAN isDummyRecordFilled()const {return 0 < _dummyRecord._length;}
         OSS_INLINE dpsLogRecordHeader &getDummmyRecord() {return _dummyRecord;}
         OSS_INLINE dpsPageMeta &getDummyPageMeta() {return _dummyPageMeta;}

         OSS_INLINE BOOLEAN isIrreversible()const {return _irreversible;}
         OSS_INLINE dpsLogRecordHeader &getRecord() {return _record;}
         OSS_INLINE const dpsLogRecordHeader &getRecord()const {return _record;}
         OSS_INLINE dpsPageMeta &getPageMeta() {return _pageMeta;}
         
      private:
         void _init();

      private:
         const dpsWriteRequest *_req = nullptr;
         const dpsWriteOptions *_o = nullptr;
         BOOLEAN _irreversible = FALSE;
         UINT32 _alignedRecordSize = 0;
         dpsLogRecordHeader _dummyRecord;
         dpsPageMeta _dummyPageMeta;
         dpsLogRecordHeader _record;
         dpsPageMeta _pageMeta;
   };//class dpsWriteContext
} // namespace engine


#endif//DPS_WRITE_CONTEXT_HPP_