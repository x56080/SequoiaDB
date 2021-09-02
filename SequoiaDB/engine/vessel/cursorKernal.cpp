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

   Source File Name = cursorKernal.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/cursorKernal.h"
#include "ossMem.hpp"
#include "pdTrace.hpp"
#include "ossLikely.hpp"
#include "vessel/vesselImpl.h"
#include "vessel/ISession.h"

namespace engine
{
namespace vessel
{
   UINT32 CURSOR_FLAG_IS_OPEN = 0x01;
   UINT32 CURSOR_FLAG_HIT_THE_END = 0x02;

   cursorKernal::~cursorKernal()
   {
      _mb.release();
   }

   BOOLEAN cursorKernal::isOpen()const
   {
      return OSS_BIT_TEST(_flags, CURSOR_FLAG_IS_OPEN);
   }

   INT32 cursorKernal::open(vesselImpl *db,
                            IQueryFilter *filter,
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

      OSS_BIT_SET(_flags, CURSOR_FLAG_IS_OPEN);
      if (NULL != o)
      {
         _options = *o;
      }
      _db = db;
      _filter = filter;

      if (0 < _options.initBufSize)
      {
         rc = _mb.reserve(_options.initBufSize);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to reserve memory:%d", rc);
            goto error;
         }
      }  
   done:
      return rc;
   error:
      close();
      goto done;
   }

   void cursorKernal::close()
   {
      _options = cursorOptions();
      _flags = 0;
      _mb.release();
      _read = 0;
      _db = NULL;
      _filter = NULL;
   
      return;
   }

   INT32 cursorKernal::getNext(ISession *session, slice &content)
   {
      INT32 rc = SDB_OK;
      UINT32 size = 0;

      if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }

      if (!hasMoreDataToFetch())
      {
         if (hitTheEnd())
         {
            rc = SDB_VESSEL_EOC;
            goto error;
         }

         _pushedThisLoop = 0;
         _read = 0;
         _mb.resize(0);
         rc = _db->pushMoreToCursor(session, this);
         if (SDB_OK != rc)
         {
            goto error;
         }

         if (!hasMoreDataToFetch())
         {
            if (!hitTheEnd())
            {
               PD_LOG(PDERROR, "pushed nothing but flag not hit the end");
               rc = SDB_VESSEL_INTERNAL_ERR;
            }
            else
            {
               rc = SDB_VESSEL_EOC;
            }

            goto error;
         }
      }
      
      SDB_ASSERT((_read + sizeof(UINT32)) < _mb.getSize(), "impossible");
      size = *((const UINT32 *)((ossValuePtr)(_mb.getBuffer()) + _read));
      _read += sizeof(UINT32);

      SDB_ASSERT((_read + size) <= _mb.getSize(), "impossible");
      content.reset(size, (const CHAR *)(_mb.getBuffer()) + _read);
      _read += size;
   done:
      return rc;
   error:
      goto done;
   }

   BOOLEAN cursorKernal::isWaitingMorePushing()const
   {
      return !hitTheEnd() && (_pushedThisLoop < getStepLengthInLoop());
             
   }

   BOOLEAN cursorKernal::hitTheEnd()const
   {
      return (0 != OSS_BIT_TEST(_flags, CURSOR_FLAG_HIT_THE_END));
   }

   INT32 cursorKernal::pushData(UINT32 len, const CHAR *data)
   {
      INT32 rc = SDB_OK;
      UINT32 oldSize = 0;

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
      else if (!isWaitingMorePushing())
      {
         rc = SDB_VESSEL_CURSOR_NO_SPACE;
         goto error;
      }

      rc = allocateSpaceForPushing(len);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to allocate space for pushing:%d", rc);
         goto error;
      }

      oldSize = _mb.getSize();
      rc = _mb.append(sizeof(UINT32), &len);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to append size:%d", rc);
         goto error;
      }

      rc = _mb.append(len, data);
      if (SDB_OK != rc)
      {
         _mb.resize(oldSize);
         PD_LOG(PDERROR, "failed to append data:%d", rc);
         goto error;
      }

      ++_pushedThisLoop;
   done:
      return rc;
   error:
      goto done;
   }

   INT32 cursorKernal::pushDataFragments(std::initializer_list<slice> il)
   {
      INT32 rc = SDB_OK;
      UINT32 len = 0;
      UINT32 oldSize = 0;

      if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (!isWaitingMorePushing())
      {
         rc = SDB_VESSEL_CURSOR_NO_SPACE;
         goto error;
      }

      for (auto i = il.begin(); i != il.end(); ++i)
      {
         if (!i->isValid())
         {
            SDB_ASSERT(FALSE, "invalid fragment");
            rc = SDB_INVALIDARG;
            goto error;
         }
         len += i->len();
      }

      rc = allocateSpaceForPushing(len);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to allocate space for pushing:%d", rc);
         goto error;
      }

      oldSize = _mb.getSize();
      rc = _mb.append(sizeof(UINT32), &len);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to append size:%d", rc);
         goto error;
      }

      for (auto i = il.begin(); i != il.end(); ++i)
      {
         rc = _mb.append(i->len(), i->data());
         if (SDB_OK != rc)
         {
            _mb.resize(oldSize);
            PD_LOG(PDERROR, "failed to append data:%d", rc);
            goto error;
         }
      }
      
      ++_pushedThisLoop;
   done:
      return rc;
   error:
      goto done;
   }

   INT32 cursorKernal::allocateSpaceForPushing(UINT32 dataLen)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isOpen(), "must be open");

      UINT32 needSize = getRealBufSizeOfSlice(dataLen);
      UINT32 extendingSize = 0;
      
      if (needSize <= _mb.getFreeCapacity())
      {
         goto done;
      }

      extendingSize = needSize - _mb.getFreeCapacity();
      if (_options.maxBufSize < (_mb.getCapacity() + extendingSize))
      {
         /// max buf size is too small
         rc = SDB_VESSEL_OUT_OF_RESOURCE;
         goto error;
      }
      else if (!_mb.isEmpty())
      {
         /// Extend buffer only at the first pushing of current loop.
         rc = SDB_VESSEL_CURSOR_NO_SPACE;
         goto error;
      }

      rc = _mb.reserve(extendingSize);
      if (SDB_OK != rc)
      {
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   void cursorKernal::pushEnd()
   {
      if (OSS_LIKELY(isOpen()))
      {
         OSS_BIT_SET(_flags, CURSOR_FLAG_HIT_THE_END);
      }
      return;
   }
}//namespace vessel
}//namespace engine