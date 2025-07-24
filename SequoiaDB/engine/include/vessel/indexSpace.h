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

   Source File Name = indexSpace.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef VESSEL_INDEX_SPACE_H_
#define VESSEL_INDEX_SPACE_H_

#include "vessel/logicalPageSpacePte.h"

namespace engine
{
namespace vessel
{  
   class indexSpace : public logicalPageSpacePte
   {
      public:
         indexSpace(const storageUnitManifest *manifest):
         logicalPageSpacePte(manifest){}
         virtual ~indexSpace(){}

      public:
         virtual SPACE_TYPE getSpaceType()const override
         {
            return SPACE_TYPE_IDX;
         }

      protected:
         virtual UINT32 _getSegmentPcntReused()const override {return 128;}

   };//class indexSpace
}//namespace vessel
}//namespace engine

#endif//VESSEL_INDEX_SPACE_H_