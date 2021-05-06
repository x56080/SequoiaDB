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

namespace engine
{
namespace vessel
{
   vesselFileName::vesselFileName():
   _space(INVALID_SPACE_ID),
   _type(INVALID_FILE_TYPE),
   _sequence(0)
   {
      ossMemset(_name, 0, sizeof(_name));
   }

   vesselFileName::vesselFileName(const vesselFileName &o):
   _space(o._space),
   _type(o._type),
   _sequence(o._sequence)
   {
      ossMemcpy(_name, o._name, sizeof(_name));
   }


   vesselFileName::~vesselFileName()
   {

   }

   vesselFileName &vesselFileName::operator=(const vesselFileName &o)
   {
      _space = o._space;
      _type = o._type;
      _sequence = o._sequence;
      ossMemcpy(_name, o._name, sizeof(_name));
      return *this;
   }

   void vesselFileName::reset()
   {
      _space = INVALID_SPACE_ID;
      _type = INVALID_FILE_TYPE;
      _sequence = 0;
      ossMemset(_name, 0, sizeof(_name));
      return;
   }

   INT32 vesselFileName::extract(const strSlice &fileName,
                                 SPACE_ID sid)
   {
      BOOLEAN r = FALSE;
      std::vector<std::string> columns;
      UINT32 space = 0;
      reset();
      if (fileName.strLen() <= (FILE_NAME_PREFIX_LEN + 1) ||
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
          FILE_NAME_FORMAT_MAX_COLUMNS < columns.size())
      {
         goto done;
      }
      else if (0 != columns.at(0).compare(FILE_NAME_PREFIX))
      {
         goto done;
      }
      else if (!utilStrIsDigit(columns.at(1).c_str()))
      {
         goto done;
      }
      else if (!parseFileSuffix(columns.at(2).c_str(), _type))
      {
         goto done;
      }
      else if ((FILE_NAME_FORMAT_MIN_COLUMNS + 1) == columns.size())
      {
         if (utilStrIsDigit(columns.at(3).c_str()))
         {
            _sequence = ossAtoll(columns.at(3).c_str());
         }
         else
         {
            goto done;
         }
      }

      space = ossAtoi(columns.at(1).c_str());
      if (MAX_SPACE_ID < space)
      {
         goto done;
      }
      else if (INVALID_SPACE_ID != sid &&
               space != (UINT32)(sid))
      {
         goto done;
      }
      _space = (SPACE_ID)space;

      ossMemcpy(_name, fileName.str(), fileName.strLen());
      r = TRUE;

   done:
      if (!r)
      {
         reset();
      }
      return r;
   }

   BOOLEAN vesselFileName::build(SPACE_ID sid,
                                 FILE_TYPE type)
   {
      BOOLEAN r = FALSE;
      reset();
      const CHAR *suffix = NULL;
      if (OSS_UNLIKELY(FILE_TYPE_SUFFIX_ARR_SIZE <= type))
      {
         goto done;
      }
      else if (OSS_UNLIKELY(INVALID_SPACE_ID == sid ||
                            MAX_SPACE_ID < sid))
      {
         goto done;
      }

      suffix = FILE_TYPE_SUFFIX_ARRAY[type];
      ossSnprintf(_name, MAX_FILE_NAME_LEN + 1, "%s.%d.%s",
                  FILE_NAME_PREFIX, sid, suffix);
      _space = sid;
      _type = type;
      _sequence = 0;
      r = TRUE;
   done:
      return r;
   }

   BOOLEAN vesselFileName::build(SPACE_ID sid,
                                 FILE_TYPE type,
                                 UINT64 sequence)
   {
      BOOLEAN r = FALSE;
      reset();
      const CHAR *suffix = NULL;
      if (OSS_UNLIKELY(FILE_TYPE_SUFFIX_ARR_SIZE <= type))
      {
         goto done;
      }
      else if (OSS_UNLIKELY(INVALID_SPACE_ID == sid ||
                            MAX_SPACE_ID < sid))
      {
         goto done;
      }

      suffix = FILE_TYPE_SUFFIX_ARRAY[type];
      ossSnprintf(_name, MAX_FILE_NAME_LEN + 1, "%s.%d.%s.%lld",
                  FILE_NAME_PREFIX, sid, suffix, sequence);
      _space = sid;
      _type = type;
      _sequence = sequence;
      r = TRUE;
   done:
      return r;
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
      ossSnprintf(buf, MAX_SPACE_DIR_LEN + 1, "%s_%d",
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
      if (dirName.strLen() < (FILE_NAME_PREFIX_LEN + 2))
      {
         goto done;
      }
      else if (0 != ossStrncmp(FILE_NAME_PREFIX, dirName.str(), FILE_NAME_PREFIX_LEN))
      {
         goto done;
      }
      else if (!utilStrIsDigit(dirName.str() + FILE_NAME_PREFIX_LEN + 1))
      {
         goto done;
      }

      digit = ossAtoi(dirName.str() + FILE_NAME_PREFIX_LEN + 1);
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
}//namespace vessel
}//namespace engine