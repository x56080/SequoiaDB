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

   Source File Name = ICursor.cpp

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

#include "vessel/ICursor.h"
#include "vessel/cursorObject.h"

namespace engine
{
namespace vessel
{
   ICursor::~ICursor()
   {
      SAFE_OSS_DELETE(_cursor);
   }

   CURSOR_TYPE ICursor::getType()const
   {
      if (OSS_LIKELY(NULL != _cursor))
      {
         return _cursor->getType();
      }
      return CURSOR_TYPE_INVALID;
   }


   BOOLEAN ICursor::isOpen()const
   {
      return NULL != _cursor && _cursor->isOpen();   
   }

   INT32 ICursor::setOpenedCursorObj(cursorObject *obj)
   {
      INT32 rc = SDB_OK;
      if (OSS_UNLIKELY(NULL != _cursor))
      {
         _cursor->close();
         SDB_OSS_DEL _cursor;
         _cursor = NULL;
      }

      if (OSS_UNLIKELY(NULL == obj ||
                       !obj->isOpen()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      _cursor = obj;
   done:
      return rc;
   error:
      goto done;
   }
   
   INT32 ICursor::close()
   {
      INT32 rc = SDB_INVALIDARG;
      if (NULL != _cursor)
      {
         rc = _cursor->close();
         SDB_OSS_DEL(_cursor);
         _cursor = NULL;
      }
      return rc;
   }

   INT32 ICursor::getNext(ISession *session, slice &content)
   {
      if (OSS_LIKELY(NULL != _cursor))
      {
         return _cursor->getNext(session, content);
      }
      return SDB_INVALIDARG;
   }

}//namespace vessel
}//namespace engine