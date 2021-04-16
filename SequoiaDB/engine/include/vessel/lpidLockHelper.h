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

   Source File Name = lpidLockHelper.h

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains functions for agent processing.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_LPID_LOCK_HELPER_H_
#define VESSEL_LPID_LOCK_HELPER_H_

#include "ossLatch.hpp"
#include "vessel/requestContext.h"
#include "vessel/vesselFileDef.h"

namespace engine
{
namespace vessel
{
   class lpidLockHelper : public SDBObject
   {
      public:
         OSS_INLINE lpidLockHelper():
         _context(NULL),
         _type(INVALID_FILE_TYPE),
         _lpid(INVALID_PAGE_ID),
         _mode(SHARED)
         {}

         OSS_INLINE ~lpidLockHelper()
         {
            unlock();
         }

      public:
         OSS_INLINE INT32 lock(requestContext *context,
                               FILE_TYPE type,
                               PAGE_ID lpid,
                               OSS_LATCH_MODE mode)
         {
            INT32 rc = SDB_OK;
            if (OSS_UNLIKELY(NULL == context))
            {
               rc = SDB_INVALIDARG;
               goto error;
            }
            else if (OSS_UNLIKELY(NULL != _context))
            {
               rc = SDB_INVALIDARG;
               goto error;
            }

            rc = context->lockLpid(type, lpid, mode);
            if (SDB_OK != rc)
            {
               goto error;
            }

            _context = context;
            _type = type;
            _lpid = lpid;
            _mode = mode;
         done:
            return rc;
         error:
            goto done;
         }

         OSS_INLINE INT32 unlock()
         {
            INT32 rc = SDB_OK;
            if (NULL == _context)
            {
               rc = SDB_INVALIDARG;
               goto error;
            }

            rc = _context->unlockLpid(_type, _lpid);
            if (SDB_OK != rc)
            {
               goto error;
            }
         done:
            if (NULL != _context)
            {
               _context = NULL;
               _type = INVALID_FILE_TYPE;
               _lpid = INVALID_PAGE_ID;
               _mode = SHARED;
            }
            return rc;
         error:
            goto done;
         }

         OSS_INLINE BOOLEAN isLocked(OSS_LATCH_MODE *mode=NULL)const
         {
            if (NULL != _context)
            {
               if (NULL != mode)
               {
                  *mode = _mode;
               }
               return TRUE;
            }
            return FALSE;
         }

      private:
         requestContext *_context;
         FILE_TYPE _type;
         PAGE_ID _lpid;
         OSS_LATCH_MODE _mode;
   };//class lpidLockHelper
}//namespace vessel
}//namespace engine

#endif//VESSEL_LPID_LOCK_HELPER_H_