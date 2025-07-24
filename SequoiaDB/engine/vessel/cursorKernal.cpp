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

   Source File Name = cursorKernal.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#include "vessel/cursorKernal.h"
#include "ossMem.hpp"
#include "pdTrace.hpp"
#include "ossLikely.hpp"
#include "vessel/vesselImpl.h"

namespace engine
{
namespace vessel
{
   constexpr UINT32 CURSOR_FLAG_IS_OPEN = 0x01;
   constexpr UINT32 CURSOR_FLAG_HIT_THE_END = 0x02;

   cursorKernal::~cursorKernal()
   {
      _close();
   }

   BOOLEAN cursorKernal::isOpen()const
   {
      return 0 != OSS_BIT_TEST(_flags, CURSOR_FLAG_IS_OPEN);
   }

   BOOLEAN cursorKernal::isClosed()const
   {
      return !isOpen();
   }

   void cursorKernal::close()
   {
      _close();
   }

   INT32 cursorKernal::fetchNext(IExecutor *executor)
   {
      INT32 rc = SDB_OK;
      if (OSS_UNLIKELY(NULL == executor))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }

      if (hasMoreInBatch())
      {
         ++_pos;
      }
      else if (hitTheEnd())
      {
         rc = SDB_DMS_EOC;
         goto error;
      }
      else
      {
         _pos = -1;
         _batch.clearRows();
         UINT32 step = _options.stepSize;
         if (_options.hasRowCountLimit())
         {
            SDB_ASSERT(_fetched < _options.rowCountLimit, "impossible");
            INT64 maxRow = _options.rowCountLimit - _fetched;
            if (maxRow < (INT64)step)
            {
               step = (UINT32)maxRow;
            }
         }

         _batch.setRowLimit(step);
         rc = _db->pushMoreToCursor(executor, this);
         if (SDB_OK != rc)
         {
            goto error;
         }

         if (_batch.isEmpty())
         {
            if (!hitTheEnd())
            {
               PD_LOG(PDERROR, "pushed nothing but flag not hit the end");
               rc = SDB_VESSEL_INTERNAL_ERR;
            }
            else
            {
               rc = SDB_DMS_EOC;
            }

            goto error;
         }
         else
         {
            _pos = 0;
         }
      }
   done:
      return rc;
   error:
      goto done;
   }

   slice cursorKernal::getRawData()const
   {
      SDB_ASSERT(isOpen(), "can not be closed");
      SDB_ASSERT(_pos < (INT32)_batch.getRowCount(), "out of bound");
      return _batch.getRow(_pos);
   }

   INT32 cursorKernal::open(vesselImpl *db,
                            const cursorOptions *o)
   {
      INT32 rc = SDB_OK;

      if (OSS_UNLIKELY(NULL == db))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      if (OSS_UNLIKELY(isOpen()))
      {
         SDB_ASSERT(FALSE, "do not reopen");
         close();
      }

      if (NULL != o &&
          !o->isValid())
      {
         SDB_ASSERT(FALSE, "invalid options");
         rc = SDB_INVALIDARG;
         goto error;
      }

      OSS_BIT_SET(_flags, CURSOR_FLAG_IS_OPEN);
      if (NULL != o)
      {
         _options = *o;
      }
      _db = db;
      _batch.setBufferSizeLimit(_options.bufferSizeLimit);
      _batch.setDefaultBlockSize(_options.defaultBufferSize);
   done:
      return rc;
   error:
      close();
      goto done;
   }

   void cursorKernal::_close()
   {
      _options = cursorOptions();
      _flags = 0;
      _fetched = 0;
      _batch.fini();
      _pos = -1;
      return;
   }

   BOOLEAN cursorKernal::noMorePushThisLoop()const
   {
      SDB_ASSERT(isOpen(), "can not be invalid");
      return hitTheEnd() ||
             !_batch.isFreeToPush(0);
   }

   BOOLEAN cursorKernal::hitTheEnd()const
   {
      return 0 != OSS_BIT_TEST(_flags, CURSOR_FLAG_HIT_THE_END);
   }

   INT32 cursorKernal::pushData(UINT32 len, const CHAR *data)
   {
      INT32 rc = SDB_OK;

      if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (OSS_UNLIKELY(0 == len || NULL == data))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (hitTheEnd())
      {
         rc = SDB_VESSEL_CURSOR_NO_SPACE;
         goto error;
      }

      rc = _batch.pushRow(slice(len, data));
      if (SDB_VESSEL_ROW_BATCH_LIMITS == rc)
      {
         rc = SDB_VESSEL_CURSOR_NO_SPACE;
         goto error;
      }
      else if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to push data into batch:%d", rc);
         goto error;
      }

      ++_fetched;
      if (_options.hasRowCountLimit() &&
          _fetched == _options.rowCountLimit)
      {
         setEOC();
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 cursorKernal::pushDataFragments(std::initializer_list<slice> il)
   {
      INT32 rc = SDB_OK;

      if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (OSS_UNLIKELY(0 == il.size()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (hitTheEnd())
      {
         rc = SDB_VESSEL_CURSOR_NO_SPACE;
         goto error;
      }

      rc = _batch.pushRowFragments(il);
      if (SDB_VESSEL_ROW_BATCH_LIMITS == rc)
      {
         rc = SDB_VESSEL_CURSOR_NO_SPACE;
         goto error;
      }
      else if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to push data into batch:%d", rc);
         goto error;
      }

      ++_fetched;
      if (_options.hasRowCountLimit() &&
          _fetched == _options.rowCountLimit)
      {
         setEOC();
      }
   done:
      return rc;
   error:
      goto done;
   }

   void cursorKernal::setEOC()
   {
      SDB_ASSERT(0 != OSS_BIT_TEST(_flags, CURSOR_FLAG_IS_OPEN),
                 "must be open");
      OSS_BIT_SET(_flags, CURSOR_FLAG_HIT_THE_END);
      return;
   }
}//namespace vessel
}//namespace engine