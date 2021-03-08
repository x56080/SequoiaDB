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

   Source File Name = spaceIDLockHelper.h

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
               _context->unlockSpaceID();
               _locked = FALSE;
            }
            return;
         }
      private:
         requestContext *_context;
         BOOLEAN _locked;
   };//class spaceIDLockHelper
}//namespace vessel
}//namespace engine

#endif//VESSEL_SPACE_ID_LOCK_HELPER_H_