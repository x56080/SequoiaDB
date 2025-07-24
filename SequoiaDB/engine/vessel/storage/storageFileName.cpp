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

   Source File Name = storageFileName.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#include "vessel/storageFileName.h"
#include "ossMemPool.hpp"
#include "utilStr.hpp"
#include "ossLikely.hpp"
#include "utilStr.hpp"

namespace engine
{
namespace vessel
{
   constexpr UINT32 FILE_NAME_FORMAT_MAX_COLUMNS = 4;
   constexpr UINT32 FILE_NAME_FORMAT_MIN_COLUMNS = 3;

   storageFileName::storageFileName(const storageFileName &o):
   _fileType(o._fileType),
   _spaceType(o._spaceType),
   _sequence(o._sequence),
   _shadowSuffix(o._shadowSuffix)
   {
      ossMemcpy(_name, o._name, sizeof(_name));
   }


   storageFileName::~storageFileName()
   {

   }

   storageFileName &storageFileName::operator=(const storageFileName &o)
   {
      _fileType = o._fileType;
      _spaceType = o._spaceType;
      _sequence = o._sequence;
      _shadowSuffix = o._shadowSuffix;
      ossMemcpy(_name, o._name, sizeof(_name));
      return *this;
   }

   BOOLEAN storageFileName::operator==(const storageFileName &o)const
   {
      return _fileType == o._fileType &&
             _spaceType == o._spaceType &&
             _sequence == o._sequence &&
             _shadowSuffix == o._shadowSuffix;
   }

   void storageFileName::reset()
   {
      _fileType = INVALID_FILE_TYPE;
      _spaceType = INVALID_SPACE_TYPE;
      _sequence = 0;
      _shadowSuffix = INVALID_FILE_SHADOW_SUFFIX;
      ossMemset(_name, 0, sizeof(_name));
      return;
   }

   BOOLEAN storageFileName::extract(const strSlice &fileName,
                                    BOOLEAN shadowSuffixCompatible)
   {
      BOOLEAN r = FALSE;
      std::vector<std::string> columns;
      UINT32 maxColumnSize = FILE_NAME_FORMAT_MAX_COLUMNS;
      if (!shadowSuffixCompatible)
      {
         --maxColumnSize;
         SDB_ASSERT(FILE_NAME_FORMAT_MIN_COLUMNS <= maxColumnSize, "impossible");
      }

      reset();
      if (MAX_FILE_NAME_LEN < fileName.strLen())
      {
         goto done;
      }

      columns = utilStrSplit(fileName.str(), ".");
      if (columns.size() < FILE_NAME_FORMAT_MIN_COLUMNS ||
          maxColumnSize < columns.size())
      {
         goto done;
      }
      else if (!parseSpaceType(columns.at(0).c_str(), _spaceType, NULL))
      {
         goto done;
      }
      else if (!parseFileType(columns.at(1).c_str(), _fileType, NULL))
      {
         goto done;
      }
      else if (!utilStrIsDigit(columns.at(2).c_str()))
      {
         goto done;
      }
      
      _sequence = ossAtoi(columns.at(2).c_str());
      ossMemcpy(_name, fileName.str(), fileName.strLen());
      if ( _name[fileName.strLen() - 1] == '.')
      {
         _name[fileName.strLen() - 1] = '\0' ;
      }

      if (FILE_NAME_FORMAT_MAX_COLUMNS == columns.size())
      {
         SDB_ASSERT(shadowSuffixCompatible, "must be compatible");
         _shadowSuffix = ::engine::vessel::getShadowSuffixType(columns.at(3).c_str());
         if (INVALID_FILE_SHADOW_SUFFIX == _shadowSuffix)
         {
            goto done;
         }
      }
      r = TRUE;

   done:
      if (!r)
      {
         reset();
      }
      return r;
   }

   BOOLEAN storageFileName::build(FILE_TYPE fileType,
                                  SPACE_TYPE spaceType,
                                  UINT32 sequence,
                                  UINT16 shadowSuffix)
   {
      BOOLEAN r = FALSE;
      fileTypeDescriptor fd;
      spaceTypeDescriptor sd;
      INT32 size = 0;

      reset();
      if (OSS_UNLIKELY(INVALID_FILE_TYPE == fileType))
      {
         goto done;
      }
      else if (OSS_UNLIKELY(INVALID_SPACE_TYPE == spaceType))
      {
         goto done;
      }

      if (OSS_UNLIKELY(!getFileTypeDescriptor(fileType, fd)))
      {
         PD_LOG(PDERROR, "failed to get file descriptor of type[%d]", fileType);
         goto done;
      }

      if (OSS_UNLIKELY(!getSpaceTypeDescriptor(spaceType, sd)))
      {
         PD_LOG(PDERROR, "failed to get space descriptor of type[%d]", spaceType);
         goto done;
      }

      size = ossSnprintf(_name, MAX_FILE_NAME_LEN + 1, "%s", sd.getTypeName());

      if (INVALID_FILE_TYPE != fileType)
      {
         INT32 tmpSize = 0;
         tmpSize = ossSnprintf(_name + size, MAX_FILE_NAME_LEN + 1, ".%s.%06d",
                               fd.getTypeName(), sequence);
         SDB_ASSERT(0 < size, "impossible");
         size += tmpSize;
      }

      if (INVALID_FILE_SHADOW_SUFFIX != shadowSuffix)
      {
         INT32 tmpSize = 0;
         strSlice s;
         if (!::engine::vessel::getShadowSuffix(shadowSuffix, s))
         {
            goto done;
         }
         tmpSize = ossSnprintf(_name + size, MAX_FILE_NAME_LEN + 1 - size,
                               ".%s", s.str());
         SDB_ASSERT(0 < tmpSize, "impossible");
         size += tmpSize;
      }
      _fileType = fileType;
      _spaceType = spaceType;
      _sequence = sequence;
      _shadowSuffix = shadowSuffix;
      r = TRUE;
   done:
      if (!r)
      {
         reset();
      }
      return r;
   }

   void storageFileName::rebuildWithOutShadowSuffix()
   {
      BOOLEAN r = FALSE;
      if (!hasShadowSuffix())
      {
         goto done;
      }

      r = build(_spaceType, _fileType, _sequence);
      SDB_ASSERT(r, "must be ok");

   done:
      return;
   }

}//namespace vessel
}//namespace engine