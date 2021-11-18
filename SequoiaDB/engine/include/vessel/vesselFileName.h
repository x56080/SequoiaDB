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
#include "ossMemPool.hpp"

namespace engine
{
namespace vessel
{
   /// space dir name: _cs_<space id>
   /// simple file name: _cs_<space id>.<user defined suffix>   eg: _cs_100.csname
   /// file name: _cs_<space id>.<sequence>.<file type suffix>.[space type suffix].[shadow suffix]

   class vesselFileName : public SDBObject
   {
      public:
         vesselFileName(){}
         vesselFileName(const vesselFileName &);
         ~vesselFileName();
         vesselFileName &operator=(const vesselFileName &);
         BOOLEAN operator==(const vesselFileName &)const;

      public:
         OSS_INLINE BOOLEAN isValid()const
         {
            return INVALID_SPACE_ID != _space;
         }

         OSS_INLINE FILE_TYPE getFileType()const
         {
            return _fileType;
         }

         OSS_INLINE const CHAR *getFileName()const
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

         OSS_INLINE UINT32 getShadowSuffix()const
         {
            return _shadowSuffix;
         }
         OSS_INLINE BOOLEAN hasShadowSuffix()const
         {
            return INVALID_FILE_SHADOW_SUFFIX != _shadowSuffix;
         }
         OSS_INLINE SPACE_TYPE getSpaceType()const
         {
            return _spaceType;
         }

         void reset();

         /// Will return false when parsing filename with
         /// shadow suffix and "shadowSuffixCompatible" is false.
         BOOLEAN extract(const strSlice &fileName,
                         BOOLEAN shadowSuffixCompatible=FALSE);

         /// sequence will always included in filename
         BOOLEAN build(SPACE_ID sid,
                       FILE_TYPE type,
                       SPACE_TYPE spaceType = INVALID_SPACE_TYPE,
                       UINT64 sequence = 0,
                       UINT32 shadowSuffix = INVALID_FILE_SHADOW_SUFFIX);

         void rebuildWithOutShadowSuffix();
         
         static BOOLEAN parseDirName(const strSlice &dirName, SPACE_ID *sid);
         static BOOLEAN buildDirName(SPACE_ID sid, UINT32 bufLen, CHAR *buf);
         static BOOLEAN buildSimpleName(SPACE_ID sid,
                                        const strSlice &suffix,
                                        UINT32 bufferSize,
                                        CHAR *buffer);
      private:
         CHAR _name[MAX_FILE_NAME_LEN + 1] = {};
         SPACE_ID _space = INVALID_SPACE_ID;
         FILE_TYPE _fileType = INVALID_FILE_TYPE;
         SPACE_TYPE _spaceType = INVALID_SPACE_TYPE;
         UINT64 _sequence = 0;
         UINT32 _shadowSuffix = INVALID_FILE_SHADOW_SUFFIX;
   };//class vesselFileName

   typedef ossPoolList<vesselFileName> FILE_NAME_LIST; 
} // namespace vessel
} // namespace engine

#endif // VESSEL_VESSEL_FILE_NAME_H_