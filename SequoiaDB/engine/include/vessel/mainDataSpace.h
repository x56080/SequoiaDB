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

   Source File Name = mainDataSpace.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef VESSEL_MAIN_DATA_SPACE_H_
#define VESSEL_MAIN_DATA_SPACE_H_

#include "vessel/logicalPageSpace.h"
#include "vessel/csMetaBlockPage.h"

namespace engine
{
namespace vessel
{
   class fsmFile;

   class mainDataSpace : public logicalPageSpace
   {
      public:
         mainDataSpace(const storageUnitManifest *manifest);
         virtual ~mainDataSpace();

      public:
         virtual SPACE_TYPE getSpaceType()const override
         {
            return SPACE_TYPE_MAIN_DATA;
         }

      public:
         fsmFile *getFsmFile()
         {
            return _fsm;
         }
      private:
         virtual UINT32 _getReservedLpidUnits()const override
         {
            return 1;
         }

         virtual INT32 _onCreationFinished() override;
         virtual INT32 _onOpenFinished(const storageFileLoader &loader) override;
         virtual void _onClosingStarted() override;
         virtual void _onDestroyStarted() override;

      private:
         fsmFile *_fsm = NULL;
   };//class mainDataSpace
}//namespace vessel
}//namespace engine

#endif//VESSEL_MAIN_DATA_SPACE_H_