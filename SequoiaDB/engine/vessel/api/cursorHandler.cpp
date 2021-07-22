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
   cursorHandler::cursorHandler(cursorKernal *kernal)
   {
      _cursor.reset(kernal);
   }

   cursorHandler::~cursorHandler()
   {
      _cursor.release();
   }

   cursorHandler &cursorHandler::operator=(const cursorHandler &o)
   {
      _cursor = o._cursor;
      return *this;
   }

   CURSOR_TYPE cursorHandler::getType()const
   {
      return  _cursor.isValid() ?
              _cursor.get<cursorKernal>()->getType() :
              CURSOR_TYPE_INVALID;
   }


   BOOLEAN cursorHandler::isOpen()const
   {
      return _cursor.isValid() && _cursor.get<cursorKernal>()->isOpen();
   }
   
   void cursorHandler::close()
   {
      if (_cursor.isValid())
      {
         _cursor.get<cursorKernal>()->close();
      }
      return;
   }

   INT32 cursorHandler::getNext(ISession *session, slice &content)
   {
      if (_cursor.isValid())
      {
         return _cursor.get<cursorKernal>()->getNext(session, content);
      }
      
      return SDB_VESSEL_RESOURCES_NOT_INIT;
   }
}//namespace vessel
}//namespace engine