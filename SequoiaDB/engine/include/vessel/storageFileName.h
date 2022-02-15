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

#include "vessel/vesselFileDef.h"
#include "vessel/vesselIdDef.h"
#include "vessel/strSlice.h"
#include "ossMemPool.hpp"

namespace engine
{
namespace vessel
{
   /// space dir name: _cs_<space id> 
   /// storage file name: <space type>.<file type>.<sequence suffix>.[shadow suffix] eg: data.ds.000001
   /// Will not supplement zero if sequence has more than 6 digits.
   class storageFileName : public SDBObject
   {
      public:
         storageFileName(){}
         storageFileName(const storageFileName &);
         ~storageFileName();
         storageFileName &operator=(const storageFileName &);
         BOOLEAN operator==(const storageFileName &)const;

      public:
         OSS_INLINE BOOLEAN isValid()const
         {
            return INVALID_FILE_TYPE != _fileType &&
                   INVALID_SPACE_TYPE != _spaceType; 
         }

         OSS_INLINE FILE_TYPE getFileType()const
         {
            return _fileType;
         }

         OSS_INLINE const CHAR *getFileName()const
         {
            return _name;
         }

         OSS_INLINE UINT32 getSequence()const
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
         BOOLEAN build(FILE_TYPE type,
                       SPACE_TYPE spaceType = INVALID_SPACE_TYPE,
                       UINT32 sequence = 0,
                       UINT32 shadowSuffix = INVALID_FILE_SHADOW_SUFFIX);
         


         void rebuildWithOutShadowSuffix();
         
         static BOOLEAN buildDirName(SPACE_ID sid, UINT32 bufLen, CHAR *buf);
         static BOOLEAN parseDirName(const strSlice &dirName, SPACE_ID *sid);


      private:
         CHAR _name[MAX_FILE_NAME_LEN + 1] = {};
         FILE_TYPE _fileType = INVALID_FILE_TYPE;
         SPACE_TYPE _spaceType = INVALID_SPACE_TYPE;
         UINT32 _sequence = 0;
         UINT32 _shadowSuffix = INVALID_FILE_SHADOW_SUFFIX;
   };//class storageFileName

   typedef ossPoolList<storageFileName> STORAGE_FILE_NAME_LIST; 
} // namespace vessel
} // namespace engine

#endif // VESSEL_STORAGE_FILE_NAME_H_