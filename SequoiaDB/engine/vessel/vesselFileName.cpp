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

   Source File Name = vesselFileName.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/vesselFileName.h"
#include "ossMemPool.hpp"
#include "utilStr.hpp"
#include "ossLikely.hpp"
#include "utilStr.hpp"

namespace engine
{
namespace vessel
{
   const UINT32 FILE_NAME_FORMAT_MAX_COLUMNS = 5;
   const UINT32 FILE_NAME_FORMAT_MIN_COLUMNS = 3;

   vesselFileName::vesselFileName(const vesselFileName &o):
   _space(o._space),
   _fileType(o._fileType),
   _spaceType(o._spaceType),
   _sequence(o._sequence),
   _shadowSuffix(o._shadowSuffix)
   {
      ossMemcpy(_name, o._name, sizeof(_name));
   }


   vesselFileName::~vesselFileName()
   {

   }

   vesselFileName &vesselFileName::operator=(const vesselFileName &o)
   {
      _space = o._space;
      _fileType = o._fileType;
      _spaceType = o._spaceType;
      _sequence = o._sequence;
      _shadowSuffix = o._shadowSuffix;
      ossMemcpy(_name, o._name, sizeof(_name));
      return *this;
   }

   BOOLEAN vesselFileName::operator==(const vesselFileName &o)const
   {
      return isValid() && o.isValid() &&
             _space == o._space &&
             _fileType == o._fileType &&
             _spaceType == o._spaceType &&
             _sequence == o._sequence &&
             _shadowSuffix == o._shadowSuffix;
   }

   void vesselFileName::reset()
   {
      _space = INVALID_SPACE_ID;
      _fileType = INVALID_FILE_TYPE;
      _spaceType = INVALID_SPACE_TYPE;
      _sequence = 0;
      _shadowSuffix = INVALID_FILE_SHADOW_SUFFIX;
      ossMemset(_name, 0, sizeof(_name));
      return;
   }

   BOOLEAN vesselFileName::extract(const strSlice &fileName,
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
      if (fileName.strLen() <= FILE_NAME_PREFIX_LEN ||
          MAX_FILE_NAME_LEN < fileName.strLen())
      {
         goto done;
      }
      else if (0 != ossStrncmp(FILE_NAME_PREFIX, fileName.str(), FILE_NAME_PREFIX_LEN))
      {
         goto done;
      }

      columns = utilStrSplit(fileName.str(), ".");
      if (columns.size() < FILE_NAME_FORMAT_MIN_COLUMNS ||
          maxColumnSize < columns.size())
      {
         goto done;
      }
      else if (!parseDirName(strSlice(columns.at(0).c_str(), columns.at(0).size()), &_space))
      {
         goto done;
      }
      else if (!utilStrIsDigit(columns.at(1).c_str()))
      {
         goto done;
      }
      else if (!parseFileType(columns.at(2).c_str(), _fileType, NULL))
      {
         goto done;
      }
      else if (!ossStrcmp(columns.at(1).c_str(), ""))
      {
         goto done;
      }
      
      _sequence = ossAtoll(columns.at(1).c_str());
      ossMemcpy(_name, fileName.str(), fileName.strLen());
      if ( _name[fileName.strLen() - 1] == '.')
      {
         _name[fileName.strLen() - 1] = '\0' ;
      }

      if (FILE_NAME_FORMAT_MIN_COLUMNS == columns.size())
      {
         r = TRUE;
      }
      else if (FILE_NAME_FORMAT_MAX_COLUMNS == columns.size())
      {
         if (!parseSpaceType(columns.at(3).c_str(), _spaceType, NULL))
         {
            goto done;
         }

         SDB_ASSERT(shadowSuffixCompatible, "must be compatible");
         _shadowSuffix = ::engine::vessel::getShadowSuffixType(columns.at(4).c_str());
         if (INVALID_FILE_SHADOW_SUFFIX == _shadowSuffix)
         {
            goto done;
         }
         r = TRUE;
      }
      else /// 4 == columns.size()
      {
         SPACE_TYPE st = INVALID_SPACE_TYPE;

         if (parseSpaceType(columns.at(3).c_str(), st, NULL))
         {
            _spaceType = st;
            r = TRUE;
            goto done;
         }

         if (!shadowSuffixCompatible)
         {
            goto done;
         }
         /// if the 4th column is not cluster, it must be valid shadow suffix.
         _shadowSuffix = ::engine::vessel::getShadowSuffixType(columns.at(3).c_str());
         if (INVALID_FILE_SHADOW_SUFFIX == _shadowSuffix)
         {
            goto done;
         }

         r = TRUE;
      }

   done:
      if (!r)
      {
         reset();
      }
      return r;
   }

   BOOLEAN vesselFileName::build(SPACE_ID sid,
                                 FILE_TYPE fileType,
                                 SPACE_TYPE spaceType,
                                 UINT64 sequence,
                                 UINT32 shadowSuffix)
   {
      BOOLEAN r = FALSE;
      fileDescriptor fd;
      spaceTypeDescriptor sd;
      INT32 size = 0;

      reset();
      if (OSS_UNLIKELY(INVALID_SPACE_ID == sid ||
                       MAX_SPACE_ID < sid))
      {
         goto done;
      }
      else if (OSS_UNLIKELY(INVALID_FILE_TYPE == fileType))
      {
         goto done;
      }
      else if (OSS_UNLIKELY(INVALID_SPACE_TYPE == spaceType))
      {
         goto done;
      }

      if (OSS_UNLIKELY(!getFileDescriptor(fileType, fd)))
      {
         PD_LOG(PDERROR, "failed to get file descriptor of type[%d]", fileType);
         goto done;
      }

      if (OSS_UNLIKELY(!getSpaceTypeDescriptor(spaceType, sd)))
      {
         PD_LOG(PDERROR, "failed to get space descriptor of type[%d]", spaceType);
         goto done;
      }

      size = ossSnprintf(_name, MAX_FILE_NAME_LEN + 1, "%s%d.%lld.%s",
                         FILE_NAME_PREFIX, sid, sequence, fd.getSuffix());
      SDB_ASSERT(0 < size, "impossible");

      if (INVALID_SPACE_TYPE != spaceType)
      {
         INT32 tmpSize = 0;
         tmpSize = ossSnprintf(_name + size, MAX_FILE_NAME_LEN + 1 - size,
                               ".%s", sd.getSuffix());
         SDB_ASSERT(0 < tmpSize, "impossible");
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
      _space = sid;
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

   void vesselFileName::rebuildWithOutShadowSuffix()
   {
      BOOLEAN r = FALSE;
      if (!isValid())
      {
         goto done;
      }
      else if (!hasShadowSuffix())
      {
         goto done;
      }

      r = build(_space, _fileType, _spaceType, _sequence);
      SDB_ASSERT(r, "must be ok");

   done:
      return;
   }

   BOOLEAN vesselFileName::buildDirName(SPACE_ID sid, UINT32 bufLen, CHAR *buf)
   {
      BOOLEAN r = FALSE;
      if (INVALID_SPACE_ID == sid ||
          MAX_SPACE_ID < sid)
      {
         goto done;
      }
      else if (bufLen < (MAX_SPACE_DIR_LEN + 1) ||
               NULL == buf)
      {
         goto done;
      }

      ossMemset(buf, 0, MAX_SPACE_DIR_LEN + 1);
      ossSnprintf(buf, MAX_SPACE_DIR_LEN + 1, "%s%d",
                   FILE_NAME_PREFIX, sid);
      r = TRUE;
   done:
      return r;
   }

   BOOLEAN vesselFileName::parseDirName(const strSlice &dirName, SPACE_ID *sid)
   {
      BOOLEAN r = FALSE;
      UINT32 digit = 0;
      /// _vessel_<space id>
      if (dirName.strLen() <= FILE_NAME_PREFIX_LEN)
      {
         goto done;
      }
      else if (0 != ossStrncmp(FILE_NAME_PREFIX, dirName.str(), FILE_NAME_PREFIX_LEN))
      {
         goto done;
      }
      else if (!utilStrIsDigit(dirName.str() + FILE_NAME_PREFIX_LEN))
      {
         goto done;
      }

      digit = ossAtoi(dirName.str() + FILE_NAME_PREFIX_LEN);
      if (MAX_SPACE_ID < digit)
      {
         goto done;
      }

      r = TRUE;
      if (NULL != sid)
      {
         *sid = (SPACE_ID)digit;
      }
   done:
      return r;
   }

   BOOLEAN vesselFileName::buildSimpleName(SPACE_ID sid,
                                           const strSlice &suffix,
                                           UINT32 bufferSize,
                                           CHAR *buffer)
   {
      BOOLEAN r = FALSE;
      UINT32 minBufSize = 0;

      if (OSS_UNLIKELY(INVALID_SPACE_ID == sid ||
                       MAX_SPACE_ID < sid ||
                       suffix.empty() ||
                       NULL == buffer))
      {
         goto done;
      }
      
      /// prefix size + sid size(always according to max size) + suffix len + '.' + '\0'
      minBufSize = FILE_NAME_PREFIX_LEN + suffix.strLen() + 7;
      if (bufferSize < minBufSize)
      {
         goto done;
      }

      ossSnprintf(buffer, bufferSize, "%s%d.%s",
                  FILE_NAME_PREFIX, sid, suffix.str());

      r = TRUE;
   done:
      return r;
   }
}//namespace vessel
}//namespace engine