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

   Source File Name = storageFileManifest.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_STORAGE_FILE_MANIFEST_H_
#define VESSEL_STORAGE_FILE_MANIFEST_H_

#include "vessel/vesselIdDef.h"
#include "vessel/vesselFileDef.h"
#include "vessel/storageFileDef.h"

namespace engine
{
namespace vessel
{
   struct storageFileManifest : public SDBObject
   {
      storageFileManifest &operator=(const storageFileManifest &o)
      {
         sid = o.sid;
         stype = o.stype;
         ftype = o.ftype;
         secretValue = o.secretValue;
         args = o.args;
         return *this;
      }

      OSS_INLINE BOOLEAN isValid()const
      {
         return INVALID_SPACE_ID != sid &&
                INVALID_SPACE_TYPE != stype &&
                INVALID_FILE_TYPE != ftype &&
                args.isValid();
      }

      void reset()
      {
         sid = INVALID_SPACE_ID;
         stype = INVALID_SPACE_TYPE;
         ftype = INVALID_FILE_TYPE;
         secretValue = 0;
         args.reset();
      }

      SPACE_ID sid = INVALID_SPACE_ID;
      SPACE_TYPE stype = INVALID_SPACE_TYPE;
      SPACE_TYPE ftype = INVALID_FILE_TYPE;
      UINT32 secretValue = 0;
      storageCoreArgs args;
   };//class storageFileManifest
} // namespace vessel

} // namespace engine


#endif//VESSEL_STORAGE_FILE_MANIFEST_H_