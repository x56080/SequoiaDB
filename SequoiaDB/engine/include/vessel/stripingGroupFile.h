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

   Source File Name = strpingGroupFile.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_STRIPING_GROUP_FILE_H_
#define VESSEL_STRIPING_GROUP_FILE_H_

#include "vessel/extentStorageFile.h"

namespace engine
{
namespace vessel
{
   class stripingGroupFile : public extentStorageFile
   {
      public:
         stripingGroupFile();
         virtual ~stripingGroupFile();

      private:
         virtual SPACE_TYPE getSpaceType()const
         {
            return SPACE_TYPE_FSM_SG;
         }
         virtual const CHAR *getMagicChars()const
         {
            return "SDBVSTRI";
         }

         virtual INT32 initUserDefinedHead(const void *userDefinedOptions, CHAR *headBuf);
         virtual INT32 validateUserDefinedHead(const void *head);
         virtual INT32 afterHeadOpen();
   };//class stripingGroupFile
}//namespace vessel
}//namespace engine

#endif//VESSEL_STRIPING_GROUP_FILE_H_