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

   Source File Name = vesselFileName.h

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

#ifndef VESSEL_VESSEL_FILE_NAME_H_
#define VESSEL_VESSEL_FILE_NAME_H_

#include "vessel/vesselFileDef.h"
#include "vessel/vesselIdDef.h"
#include "vessel/strSlice.h"

namespace engine
{
namespace vessel
{
   class vesselFileName : public SDBObject
   {
      public:
         vesselFileName();
         vesselFileName(const vesselFileName &);
         ~vesselFileName();
         vesselFileName &operator=(const vesselFileName &);

      public:
         OSS_INLINE BOOLEAN isValid()const
         {
            return INVALID_SPACE_ID != _space;
         }

         OSS_INLINE FILE_TYPE getType()const
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

         OSS_INLINE UINT64 getSequence()const
         {
            return _sequence;
         }

         void reset();
         /// if sid set as valid value, "extract" will also validate
         /// space id in file name.
         BOOLEAN extract(const strSlice &fileName, SPACE_ID sid=INVALID_SPACE_ID);
         BOOLEAN build(SPACE_ID sid, FILE_TYPE type, UINT64 sequence);
         /// "build" with out sequence will set sequence as 0,
         /// but ignore sequence in file name.
         BOOLEAN build(SPACE_ID sid, FILE_TYPE type);
         static BOOLEAN parseDirName(const strSlice &dirName, SPACE_ID *sid);
         static BOOLEAN buildDirName(SPACE_ID sid, UINT32 bufLen, CHAR *buf);
      private:
         CHAR _name[MAX_FILE_NAME_LEN + 1];
         SPACE_ID _space;
         FILE_TYPE _type;
         UINT64 _sequence;
   };//class vesselFileName
} // namespace vessel
} // namespace engine

#endif // VESSEL_VESSEL_FILE_NAME_H_