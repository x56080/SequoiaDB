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

   Source File Name = storageUnitManifest.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_STORAGE_UNIT_MANIFEST_H_
#define VESSEL_STORAGE_UNIT_MANIFEST_H_

#include "vessel/storageFileDef.h"

namespace engine
{
namespace vessel
{
#pragma pack(4)
   struct storageUnitManifest : public SDBObject
   {
      storageUnitManifest(){}
      ~storageUnitManifest(){}

      storageUnitManifest(const storageUnitManifest &o):
      sid(o.sid),
      flags(o.flags),
      secretValue(o.secretValue),
      dataArgs(o.dataArgs),
      idxArgs(o.idxArgs),
      lobArgs(o.lobArgs){}

      storageUnitManifest &operator=(const storageUnitManifest &o)
      {
         sid = o.sid;
         flags = o.flags;
         secretValue = o.secretValue;
         dataArgs = o.dataArgs;
         idxArgs = o.idxArgs;
         lobArgs = o.lobArgs;
         return *this;
      }

      BOOLEAN isValid()const;
      void reset()
      {
         sid = INVALID_SPACE_ID;
         flags = 0;
         secretValue = 0;
         dataArgs.reset();
         idxArgs.reset();
         lobArgs.reset();
      }
      

      SPACE_ID sid = INVALID_SPACE_ID;
      UINT16 flags = 0;
      UINT32 secretValue = 0;

      storageCoreArgs dataArgs;
      storageCoreArgs idxArgs;
      storageCoreArgs lobArgs;
   };//struct storageUnitManifest

   struct storageFileManifest : public SDBObject
   {
      storageFileManifest(){}
      ~storageFileManifest(){}
      
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
   };//struct storageFileManifest

#pragma pack()
} // namespace vessel

} // namespace engine


#endif//VESSEL_STORAGE_UNIT_MANIFEST_H_