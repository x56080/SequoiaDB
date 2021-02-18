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

   Source File Name = storageFileName.h

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains functions for agent processing.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_STORAGE_FILE_NAME_H_
#define VESSEL_STORAGE_FILE_NAME_H_

#include "vessel/vesselDef.h"
#include "vessel/storageFileDef.h"

namespace engine
{
namespace vessel
{
   class storageFileName : public SDBObject
   {
      public:
         storageFileName();
         ~storageFileName();
      public:
         OSS_INLINE BOOLEAN valid()const
         {
            return _type != INVALID_SPACE_TYPE;
         }

         OSS_INLINE SPACE_TYPE getType()const
         {
            return _type;
         }

         OSS_INLINE const CHAR *getName()const
         {
            return _name;
         }

         OSS_INLINE SPACE_ID getSpaceID()const
         {
            return _space;
         }

         OSS_INLINE UINT32 getSequence()const
         {
            return _sequence;
         }

         void reset();
         INT32 extract(const CHAR *fileName, const CHAR *prefix);
         INT32 build(SPACE_TYPE type, SPACE_ID space, UINT32 sequence);
      private:
         CHAR _name[SU_FILE_NAME_LEN + 1];
         SPACE_ID _space;
         SPACE_TYPE _type;
         UINT32 _sequence;
   };//class storageFileName
}//namespace vessel
}//namespace engine

#endif//VESSEL_STORAGE_FILE_NAME_H_