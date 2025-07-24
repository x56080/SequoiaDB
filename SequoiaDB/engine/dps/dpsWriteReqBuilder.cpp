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

   Source File Name = dpsWriteReqBuilder.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#include "dpsWriteReqBuilder.hpp"

namespace engine
{
   constexpr UINT32 _DEFAULT_BUFFER_SIZE = 8192;

   dpsWriteReqBuilder::dpsWriteReqBuilder():
   _buf(utilBufferBuilder::options(_DEFAULT_BUFFER_SIZE))
   {
   }

   dpsWriteReqBuilder::dpsWriteReqBuilder(UINT32 defaultBufSize):
   _buf(utilBufferBuilder::options(defaultBufSize))
   {
      SDB_ASSERT(0 < defaultBufSize, "can not be invalid");
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DPSWREQBUILDER_RESET, "dpsWriteReqBuilder::reset" )
   void dpsWriteReqBuilder::reset()
   {
      PD_TRACE_ENTRY(SDB__DPSWREQBUILDER_RESET);
      _type = LOG_TYPE_DUMMY;
      _flags = 0;
      _elementNum = 0;
      _buf.restart() ;

      PD_TRACE_EXIT(SDB__DPSWREQBUILDER_RESET);
      return;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DPSWREQBUILDER_REAP, "dpsWriteReqBuilder::reap" )
   dpsWriteRequest dpsWriteReqBuilder::reap()
   {
      PD_TRACE_ENTRY(SDB__DPSWREQBUILDER_REAP);
      dpsWriteRequest req;

      req._type = _type;
      req._flags = _flags;

      if (0 < _elementNum)
      {
         _appendEndTag() ;
         UINT32 size = _buf.getSize();
         INT32 num = static_cast<INT32>(_elementNum);
         req._body = std::move(dpsRecordElements(_buf.release(), size, num));
      }
      
      reset();

      PD_TRACE_EXIT(SDB__DPSWREQBUILDER_REAP);
      return std::move(req);
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DPSWREQBUILDER_APPEND, "dpsWriteReqBuilder::append" )
   INT32 dpsWriteReqBuilder::append(DPS_TAG tag, UINT32 size, const void *data)
   {
      INT32 rc = SDB_OK;
      PD_TRACE_ENTRY(SDB__DPSWREQBUILDER_APPEND);

      if (OSS_UNLIKELY(DPS_INVALID_TAG == tag ||
                       0 == size ||
                       nullptr == data))
      {
         SDB_ASSERT(FALSE, "invalid arg");
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(DPS_MERGE_BLOCK_MAX_DATA == _elementNum))
      {
         SDB_ASSERT(FALSE, "out of max element cout");
         rc = SDB_DPS_CORRUPTED_LOG;
         goto error;
      }
      else
      {
      #if defined (_DEBUG)
         SDB_ASSERT(!_isTagDuplicated(tag), "duplicated tag");
      #endif//_DEBUG
         
         dpsRecordEle e(tag, size);
         rc = _buf.reserve(_getElementBufferSize(size));
         if (OSS_UNLIKELY(SDB_OK != rc))
         {
            PD_LOG(PDERROR, "failed to reserve buffer size:%d", rc);
            goto error;
         }

         rc = _buf.appendObj(e);
         SDB_ASSERT(SDB_OK == rc, "can not be failed");
         rc = _buf.append(size, data);
         SDB_ASSERT(SDB_OK == rc, "can not be failed");
         ++_elementNum;

      }
   done:
      PD_TRACE_EXITRC(SDB__DPSWREQBUILDER_APPEND, rc);
      return rc;
   error:
      /// no need to free buffer here, it managed by allocator.
      goto done;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DPSWREQBUILDER__ISTAGDUP, "dpsWriteReqBuilder::_isTagDuplicated" )
   BOOLEAN dpsWriteReqBuilder::_isTagDuplicated(DPS_TAG tag)const
   {
      PD_TRACE_ENTRY(SDB__DPSWREQBUILDER__ISTAGDUP);
      SDB_ASSERT(DPS_INVALID_TAG != tag, "can not be invalid");

      dpsRecordElements elements(_buf.getBuf().get(), _buf.getSize()) ;

      PD_TRACE_EXIT(SDB__DPSWREQBUILDER__ISTAGDUP);
      return elements.contains(tag);
   }

   dpsTrivialElement dpsWriteReqBuilder::startToBuildTsElement(DPS_TAG tag)
   {
      SDB_ASSERT(DPS_INVALID_TAG != tag, "can not be invalid");
      return std::move(dpsTrivialElement(&_buf, tag));
   }

   dpsRecordElements dpsWriteReqBuilder::peekElements() const
   {
      return std::move(dpsRecordElements(_buf.getBuf().get(),
                                         _buf.getSize(),
                                         (INT32)_elementNum));
   }

   void dpsWriteReqBuilder::_appendEndTag()
   {
      SDB_ASSERT( 0 < _elementNum, "one element appended at least" ) ;
      /// we always reserve one more byte to save end tag when appending element.
      INT32 rc = _buf.appendUint8(DPS_INVALID_TAG);
      SDB_ASSERT( SDB_OK == rc, "can not be failed");
   }
} // namespace engine
