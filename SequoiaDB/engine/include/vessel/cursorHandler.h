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

   Source File Name = cursorHandler.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_CURSOR_HANDLER_H_
#define VESSEL_CURSOR_HANDLER_H_

#include "vessel/vesselDef.h"
#include "vessel/vesselOptions.h"
#include "ossLikely.hpp"

namespace engine
{
namespace vessel
{
   class vessel;
   class IQueryFilter;
   class cursorKernal;
   class ISession;

   class cursorHandler : public SDBObject
   {
      public:
         cursorHandler();
         cursorHandler(cursorKernal *c);
         cursorHandler(const cursorHandler &o);
         cursorHandler &operator=(const cursorHandler &o);

         ~cursorHandler();

      public: /// normal api.
         CURSOR_TYPE getType()const;

         BOOLEAN isOpen()const;
         
         INT32 close();

         ///return SDB_VESSEL_END_OF_CURSOR when hit the end.
         INT32 getNext(ISession *session, slice &content);

      private:
         cursorKernal *_cursor;
   };//class cursorHandler
}//namespace vessel
}//namespace engine

#endif//VESSEL_CURSOR_HANDLER_H_