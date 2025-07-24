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

   Source File Name = listCSCursor.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef VESSEL_LIST_CS_CURSOR_H_
#define VESSEL_LIST_CS_CURSOR_H_

#include "vessel/cursorKernal.h"
#include "ossMemPool.hpp"
#include "dms.hpp"
#include "interface/IRecordFilter.h"

namespace engine
{
namespace vessel
{
   class listCSCursor : public cursorKernal
   {
      public:
         listCSCursor(){}
         virtual ~listCSCursor(){}

      public:
         virtual const CHAR *getName()const override
         {
            return "vessel.listCSCursor";
         }
         virtual slice getDataSlice()const override
         {
            return getRawData();
         }

      public:
         virtual CURSOR_TYPE getType()const
         {
            return CURSOR_TYPE_LIST_COLLECTION_SPACE;
         }

         OSS_INLINE void setFetched(UINT32 lid)
         {
            _fetched = lid;
         }

         OSS_INLINE UINT32 getFetched()const {return _fetched;}

      private:
         UINT32 _fetched = DMS_INVALID_LOGICCSID;
   };//class listCSCursor
}//namespace vessel
}//namespace engine

#endif//VESSEL_LIST_CS_CURSOR_H_