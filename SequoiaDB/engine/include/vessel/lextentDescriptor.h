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

   Source File Name = lextentDescriptor.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef VESSEL_LEXTENT_DESCRIPTOR_H_
#define VESSEL_LEXTENT_DESCRIPTOR_H_

#include "vessel/pageIdentifier.h"
#include "vessel/vesselIdDef.h"

namespace engine
{
namespace vessel
{
#pragma pack(4)
   struct lextentDescriptor
   {
      OSS_INLINE BOOLEAN isValid()const
      {
         return 0 < pcnt &&
                  INVALID_PAGE_ID != pid &&
                  INVALID_PAGE_SNAPSHOT_VERSION != psv;
      }

      OSS_INLINE void reset()
      {
         flags = 0;
         pcnt = 0;
         pid = INVALID_PAGE_ID;
         psv = INVALID_PAGE_SNAPSHOT_VERSION;
         size = 0;
         return;
      }

      OSS_INLINE UINT32 getCapacity(UINT32 pageSize)const
      {
         return pcnt * pageSize;
      }
      
      UINT16 flags = 0;
      UINT16 pcnt = 0;
      PAGE_ID pid = INVALID_PAGE_ID;
      PAGE_SNAPSHOT_VERION psv = INVALID_PAGE_SNAPSHOT_VERSION;
      UINT32 size = 0;
   };//class lextentDescriptor

   constexpr UINT32 LEXTENT_DESC_SIZE = sizeof(lextentDescriptor);
#pragma pack()
} // namespace vessel

} // namespace engine


#endif//VESSEL_LEXTENT_DESCRIPTOR_H_