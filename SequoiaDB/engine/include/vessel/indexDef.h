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

   Source File Name = indexDef.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef VESSEL_INDEX_DEF_H_
#define VESSEL_INDEX_DEF_H_

#include "core.hpp"
#include "oss.hpp"

namespace engine
{
namespace vessel
{
   enum INDEX_TYPE : UINT16
   {
      INDEX_TYPE_BTREE = 0,
      INDEX_TYPE_LSM = 1,
      INDEX_TYPE_HYBRID_TREE = 2,
      INDEX_TYPE_INVALID = 65535
   };//enum INDEX_TYPE

   constexpr UINT32 INVALID_LOGICAL_INDEX_ID = (UINT32)(-1);

   constexpr UINT32 MAX_INDEX_COUNT_PER_CL = 64;

   constexpr UINT32 MAX_INDEX_KEY_COLUMNS = 32;

   constexpr UINT32 MAX_INDEX_KEY_SIZE = 4096;

   constexpr UINT32 MAX_INDEX_META_ENTRY_SIZE = 4096;

   constexpr FLOAT32 BTREE_NODE_HIGH_WATER_MARK = 0.8;

   constexpr FLOAT64 ACCEPTABLE_COMPRESSION_RATIO = 0.2;

   enum INDEX_STATUS : UINT16
   {
      INDEX_STATUS_INVALID = 0,
      INDEX_STATUS_BUILDING = 1,
      INDEX_STATUS_NORMAL = 2,
      INDEX_STATUS_TRUNCATING = 3,
      INDEX_STATUS_REMOVING = 4,
      INDEX_STATUS_ABNORMAL = 5,
   };// enum INDEX_STATUS

   enum class INDEX_ITERATOR_TYPE : INT32
   {
      BTREE = 0,
      LSM = 1,
      HYBRID_TREE = 2,
   }; //class INDEX_ITERATOR_TYPE

}//namespace vessel
}//namespace engine

#endif//VESSEL_INDEX_DEF_H_