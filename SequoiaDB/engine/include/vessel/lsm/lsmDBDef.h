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

   Source File Name = lsmDBDef.h

   Descriptive Name = 

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          04/20/2022  LYC  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef VESSEL_LSM_DB_DEF_H_
#define VESSEL_LSM_DB_DEF_H_

#include "oss.hpp"
namespace engine
{
namespace vessel
{
   enum LSM_CF_ID : INT32
   {
      LSM_CF_INVALID = -1,
      LSM_CF_DEFAULT = 0,
      LSM_CF_HYBRID_INDEX = 1,
      LSM_CF_INDEX_META = 2,
      LSM_CF_LOBM = 3,
      LSM_CF_MAX = LSM_CF_LOBM
   };

   // The first column family name must be 'default'.
   constexpr CHAR *LSM_DEFAULT_CF_NAME = "default";
   constexpr CHAR *LSM_HYBRID_INDEX_CF_NAME = "sdb.hybridIndex";
   constexpr CHAR *LSM_LOBM_CF_NAME = "sdb.lobm";
   constexpr CHAR *LSM_INDEX_META_CF_NAME = "sdb.indexm";
} // namespace vessel
} // namespace engine

#endif // VESSEL_LSM_DB_DEF_H_