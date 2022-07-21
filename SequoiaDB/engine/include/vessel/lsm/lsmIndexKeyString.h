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

   Source File Name = lsmIndexKeyString.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          02/12/2021  WY  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef VESSEL_LSM_INDEX_KEY_STRING_H_
#define VESSEL_LSM_INDEX_KEY_STRING_H_

#include "vessel/keyString.h"
#include "vessel/globalIndexID.h"
#include "vessel/recordID.h"

namespace engine
{
namespace vessel
{
   class lsmIndexKeyString : public keyString
   {
      public:
         lsmIndexKeyString() = default;
         lsmIndexKeyString(const slice &s):
         keyString(s){}

         lsmIndexKeyString(CHAR *buffer,
                          UINT32 bufferSize,
                          UINT32 ksSize):
         keyString(buffer, bufferSize, ksSize){}

      public:
         recordID getRid()const;
         globalIndexID getGlobalIndexId()const;
   };//class lsmIndexKeyString
} // namespace vessel

} // namespace engine


#endif//VESSEL_LSM_INDEX_KEY_STRING_H_