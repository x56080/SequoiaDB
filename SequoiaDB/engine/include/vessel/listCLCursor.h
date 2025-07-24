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

   Source File Name = listCLCursor.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef VESSEL_LIST_CL_CURSOR_H_
#define VESSEL_LIST_CL_CURSOR_H_

#include "vessel/cursorKernal.h"
#include "ossMemPool.hpp"
#include "dms.hpp"
#include "vessel/objectIdentifier.h"

namespace engine
{
namespace vessel
{
   class listCLCursor : public cursorKernal
   {
      public:
         listCLCursor(){}
         virtual ~listCLCursor(){}

      public:
         virtual const CHAR *getName()const override
         {
            return "vessel.listCLCursor";
         }
         virtual slice getDataSlice()const override
         {
            return getRawData();
         }

      public:
         virtual CURSOR_TYPE getType()const
         {
            return CURSOR_TYPE_LIST_COLLECTION;
         }

         OSS_INLINE void setCollectionSpace(const collectionSpaceId &id)
         {
            _id = id;
         }

         OSS_INLINE const collectionSpaceId &getIdentifier()const
         {
            return _id;
         }

         OSS_INLINE UINT32 getScanned()const {return _scanned;}
         OSS_INLINE void setScanned(UINT32 lid) {_scanned = lid;}
      private:
         collectionSpaceId _id;
         UINT32 _scanned = DMS_INVALID_LOGICCLID;
   };//class listCLCursor
}//namespace vessel
}//namespace engine

#endif//VESSEL_LIST_CL_CURSOR_H_