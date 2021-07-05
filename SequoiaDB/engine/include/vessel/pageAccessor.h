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

   Source File Name = pageAccessor.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_PAGE_ACCESSOR_H_
#define VESSEL_PAGE_ACCESSOR_H_

#include "vessel/globalPageID.h"
#include "vessel/pageDef.h"

namespace engine
{
namespace vessel
{
   class logRecordContext;
   class runtimePageBuffer;
   class requestContext;

   class pageAccessor : public SDBObject
   {
      public:
         OSS_INLINE pageAccessor(){}
         virtual ~pageAccessor(){}

      public:
         pageAccessor(const pageAccessor &) = delete;
         pageAccessor &operator=(const pageAccessor &) = delete;

      public:
         virtual PAGE_TYPE getPageType()const = 0;

         virtual INT32 copyFrom(requestContext *context,
                                runtimePageBuffer *rpb);

      protected:
         INT32 prepareLog(requestContext *context,
                          runtimePageBuffer *rpb,
                          UINT16 logType,
                          logRecordContext *lrc);

         INT32 prepareLogDone(requestContext *context,
                              logRecordContext *lrc);

         INT32 commitLog(requestContext *context,
                         logRecordContext *lrc);

         void abortLog(requestContext *context,
                       logRecordContext *lrc);
   
   };//class pageAccessor
}//namespace vessel
}//namespace engine

#endif//VESSEL_PAGE_ACCESSOR_H_