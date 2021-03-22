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

   Source File Name = storageFileName.cpp

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

#include "vessel/storageFileName.h"
#include "ossLikely.hpp"
#include "utilStr.hpp"
#include "vessel/storageFileDef.h"
#include "vessel/vesselDef.h"
#include "vessel/storageFileUtil.h"
#include <vector>

namespace engine
{
namespace vessel
{
   const UINT32 LEN_2 = 2;
   const UINT32 LEN_3 = 3;

   storageFileName::storageFileName()
   :_space(INVALID_SPACE_ID),
    _type(INVALID_SPACE_TYPE),
    _sequence(0)
    {
       ossMemset(_name, 0, sizeof(_name));
    }

   storageFileName::~storageFileName()
   {
      
   }

   INT32 storageFileName::build(SPACE_TYPE type, SPACE_ID space, UINT32 sequence)
   {
      INT32 rc = SDB_OK;
      if (OSS_UNLIKELY(INVALID_SPACE_TYPE == type))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      reset();

      switch (type)
      {
      case SPACE_TYPE_RECORD_M:
      {
         ossSnprintf(_name, SU_FILE_NAME_LEN + 1, "%s%d.%s",
                     SU_FILE_NAME_PREFIX, space, SU_FILE_NAME_META_SUFFIX);
         break;
      }
      case SPACE_TYPE_RECORD_D:
      {
         ossSnprintf(_name, SU_FILE_NAME_LEN + 1, "%s%d.%d",
                     SU_FILE_NAME_PREFIX, space, sequence);
         break;
      }
      case SPACE_TYPE_IDX_M:
      {
         ossSnprintf(_name, SU_FILE_NAME_LEN + 1, "%s%d.%s.%s",
                     SU_FILE_NAME_PREFIX, space, SU_FILE_NAME_IDX_SUFFIX,
                     SU_FILE_NAME_META_SUFFIX);
         break;
      }
      case SPACE_TYPE_IDX_D:
      {
         ossSnprintf(_name, SU_FILE_NAME_LEN + 1, "%s%d.%s.%d",
                     SU_FILE_NAME_PREFIX, space, SU_FILE_NAME_IDX_SUFFIX,
                     sequence);
         break;
      }
      case SPACE_TYPE_NAME:
      {
         ossSnprintf(_name, SU_FILE_NAME_LEN + 1, "%s%d.%s",
                     SU_FILE_NAME_PREFIX, space, SU_FILE_NAME_CSNAME_SUFFIX);
         break;
      }
      default:
         rc = SDB_INVALIDARG;
         PD_LOG(PDERROR, "unknown space type%d", type);
         goto error;
      }

      _space = space;
      _type = type;
      _sequence = sequence;
      
   done:
      return rc;
   error:
      reset();
      goto done;
   }

   void storageFileName::reset()
   {
      _type = INVALID_SPACE_TYPE;
      _sequence = 0;
      _space = INVALID_SPACE_ID;
      ossMemset(_name, 0, sizeof(_name));
   }

   ///space_0.0
   ///space_0.meta
   ///space_0.idx.0
   ///space_0.idx.meta
   ///space_0.lob.meta
   INT32 storageFileName::extract(const CHAR *fileName, const CHAR *prefix)
   {
      /// consider about regex?
      INT32 rc = SDB_OK;
      reset();
      std::vector<string> columns;
      strSlice prefixSlice;

      if (OSS_UNLIKELY(NULL == fileName || NULL == prefix))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      if (OSS_UNLIKELY(SU_FILE_NAME_LEN < ossStrlen(fileName)))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      ossStrcpy(_name, fileName);

      columns = utilStrSplit(fileName, ".");
      if (LEN_2 != columns.size() && LEN_3 != columns.size())
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      prefixSlice.reset(columns.at(0).c_str(), columns.at(0).size());

      if (0 != ossStrcmp(prefixSlice.str(), prefix))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      if (!parseStorageUnitDir(prefixSlice, &_space))
      {
         PD_LOG(PDERROR, "failed to extract space name:%d", rc);
         goto error;
      }

      if (LEN_2 == columns.size())
      {
         if (utilStrIsDigit(columns.at(1)))
         {
            _type = SPACE_TYPE_RECORD_D;
            _sequence = ossAtoi(prefixSlice.str());
         }
         else if (0 == columns.at(1).compare(SU_FILE_NAME_META_SUFFIX))
         {
            _type = SPACE_TYPE_RECORD_M;
            _sequence = 0;
         }
         else if (0 == columns.at(1).compare(SU_FILE_NAME_CSNAME_SUFFIX))
         {
            _type = SPACE_TYPE_NAME;
            _sequence = 0;
         }
         else
         {
            rc = SDB_INVALIDARG;
            goto error;
         }
         
      }
      else if (LEN_3 == columns.size())
      {
         const std::string &str = columns.at(1);
         if (0 == str.compare(SU_FILE_NAME_IDX_SUFFIX))
         {
            const std::string &suffix = columns.at(2);
            if (utilStrIsDigit(suffix))
            {
               _type = SPACE_TYPE_IDX_D;
               _sequence = ossAtoi(suffix.c_str());
            }
            else if (0 == suffix.compare(SU_FILE_NAME_META_SUFFIX))
            {
               _type = SPACE_TYPE_IDX_M;
               _sequence = 0;
            }
            else
            {
               rc = SDB_INVALIDARG;
               goto error;
            }
         }
         else if (0 == str.compare(SU_FILE_NAME_LOB_SUFFIX))
         {
            const std::string &suffix = columns.at(2);
            if (0 == suffix.compare(SU_FILE_NAME_META_SUFFIX))
            {
               _type = SPACE_TYPE_LOB_M;
               _sequence = 0;
            }
            else
            {
               rc = SDB_INVALIDARG;
               goto  error;
            }
         }
         else
         {
            rc = SDB_INVALIDARG;
            goto error;
         }  
      }
      else
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

   done:
      return rc;
   error:
      reset();
      goto done;
   }
}//namespace vessel
}//namepsace engine