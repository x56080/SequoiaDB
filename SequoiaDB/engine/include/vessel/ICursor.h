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

   Source File Name = ICursor.h

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

#ifndef VESSEL_I_CURSOR_H_
#define VESSEL_I_CURSOR_H_

#include "vessel/vesselDef.h"
#include "vessel/vesselOptions.h"
#include "ossLikely.hpp"

namespace engine
{
namespace vessel
{
   class vessel;
   class IQueryFilter;
   class cursorObject;
   class ISession;

   class ICursor : public SDBObject
   {
      public:
         OSS_INLINE ICursor():
         _cursor(NULL)
         {

         }

        ~ICursor();

      public: /// normal api.
         CURSOR_TYPE getType()const;

         BOOLEAN isOpen()const;
         
         INT32 close();

         ///return SDB_VESSEL_END_OF_CURSOR when hit the end.
         INT32 getNext(ISession *session, slice &content);


         ///WRANING: callback api. you should not access these functions.
      public:
         OSS_INLINE cursorObject *get()
         {
            return _cursor;
         }

         /// WARNING: ICursor will own obj's memory only after returns ok.
         INT32 setOpenedCursorObj(cursorObject *obj);
      private:
         cursorObject *_cursor;
   };//class ICursor
}//namespace vessel
}//namespace engine

#endif//VESSEL_I_CURSOR_H_