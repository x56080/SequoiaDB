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

   Source File Name = spaceIDLockHelper.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef VESSEL_SPACE_ID_LOCK_HELPER_H_
#define VESSEL_SPACE_ID_LOCK_HELPER_H_

#include "requestContext.h"

namespace engine
{
namespace vessel
{
   class spaceIDLockHelper: public SDBObject
   {
      public:
         OSS_INLINE spaceIDLockHelper(requestContext *context):
         _context(context),
         _locked(FALSE)
         {}

         OSS_INLINE ~spaceIDLockHelper()
         {
            unlock();
         }
      private:
         spaceIDLockHelper(const spaceIDLockHelper &o):
         _context(o._context),
         _locked(o._locked)
         {}

         spaceIDLockHelper &operator=(const spaceIDLockHelper &o)
         {
            _context = o._context;
            _locked = o._locked;
            return *this;
         }
         
      public:
         OSS_INLINE BOOLEAN isLocked()const
         {
            return _locked;
         }

         OSS_INLINE INT32 lock(SPACE_ID sid, OSS_LATCH_MODE mode)
         {
            INT32 rc = SDB_OK;
            if (NULL == _context || isLocked())
            {
               rc = SDB_INVALIDARG;
               goto error;
            }
            rc = _context->lockSpaceID(sid, mode);
            if (SDB_OK != rc)
            {
               goto error;
            }
            _locked = TRUE;
      done:
            return rc;
         error:
            goto done;
         }

         OSS_INLINE void unlock()
         {
            if (isLocked())
            {
               _context->close();
               _locked = FALSE;
            }
            return;
         }
      private:
         requestContext *_context = NULL;
         BOOLEAN _locked = FALSE;
   };//class spaceIDLockHelper
}//namespace vessel
}//namespace engine

#endif//VESSEL_SPACE_ID_LOCK_HELPER_H_