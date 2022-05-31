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

   Source File Name = mainDataSpace.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

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