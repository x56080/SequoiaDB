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

   Source File Name = cursorHandler.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/cursorHandler.h"
#include "vessel/cursorKernal.h"

namespace engine
{
namespace vessel
{
   cursorHandler::cursorHandler():
   _cursor(NULL)
   {}

   cursorHandler::cursorHandler(cursorKernal *c):
   _cursor(c)
   {
      if (OSS_UNLIKELY(NULL != _cursor))
      {
         _cursor->incUsageCount();
      }
   }

   cursorHandler::cursorHandler(const cursorHandler &o):
   _cursor(NULL)
   {
      if (NULL != o._cursor)
      {
         _cursor = o._cursor;
         _cursor->incUsageCount();
      }
   }

   cursorHandler::~cursorHandler()
   {
      if (NULL != _cursor)
      {
         if (0 == _cursor->decUsageCount())
         {
            SDB_OSS_DEL _cursor;
            _cursor = NULL;
         }
      }
   }

   cursorHandler &cursorHandler::operator=(const cursorHandler &o)
   {
      if (NULL != _cursor)
      {
         if (0 == _cursor->decUsageCount())
         {
            SDB_OSS_DEL _cursor;
            _cursor = NULL;
         }
         else
         {
            _cursor = NULL;
         }
      }

      if (NULL != o._cursor)
      {
         _cursor = o._cursor;
         _cursor->incUsageCount();
      }

      return *this;
   }

   CURSOR_TYPE cursorHandler::getType()const
   {
      if (OSS_LIKELY(NULL != _cursor))
      {
         return _cursor->getType();
      }
      return CURSOR_TYPE_INVALID;
   }


   BOOLEAN cursorHandler::isOpen()const
   {
      return NULL != _cursor && _cursor->isOpen();   
   }
   
   void cursorHandler::close()
   {
      if (NULL != _cursor)
      {
         _cursor->close();
      }
      return;
   }

   INT32 cursorHandler::getNext(ISession *session, slice &content)
   {
      if (OSS_LIKELY(NULL != _cursor))
      {
         return _cursor->getNext(session, content);
      }
      return SDB_INVALIDARG;
   }
}//namespace vessel
}//namespace engine