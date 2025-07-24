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

   Source File Name = rtnStatisticsDef.hpp

   Descriptive Name =

   When/how to use:

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          11/24/2022  ZHY Initial Draft

   Last Changed =

*******************************************************************************/
#include "oss.hpp"
#include <math.h>
#include <type_traits>

namespace engine
{
   // Fields to create indexes of SYSSTAT collections
   constexpr const CHAR RTN_STAT_COLLECTION_SPACE[] = "CollectionSpace";
   constexpr const CHAR RTN_STAT_COLLECTION[] = "Collection";

   constexpr const CHAR RTN_STAT_IDX_INDEX[] = "Index";
   constexpr const CHAR RTN_STAT_IDX_MCV[] = "MCV";
   constexpr const CHAR RTN_STAT_CREATE_TIME[] = "CreateTime";
   constexpr const CHAR RTN_STAT_IDX_INDEX_PAGES[] = "IndexPages";
   constexpr const CHAR RTN_STAT_IDX_LEVELS[] = "IndexLevels";
   constexpr const CHAR RTN_STAT_IDX_IS_UNIQUE[] = "IsUnique";
   constexpr const CHAR RTN_STAT_IDX_KEY_PATTERN[] = "KeyPattern";
   constexpr const CHAR RTN_CL_STAT_AVG_NUM_FIELDS[] = "AvgNumFields";

   constexpr UINT32 RTN_STAT_DEF_AVG_NUM_FIELDS = 10;
   constexpr UINT32 RTN_STAT_DEF_TOTAL_PAGES = 1;
   constexpr UINT32 RTN_STAT_DEF_IDX_LEVELS = 1;
   constexpr UINT64 RTN_STAT_DEF_TOTAL_RECORDS = 200;
   constexpr UINT64 RTN_STAT_DEF_DATA_SIZE = 400;

   constexpr UINT32 RTN_STAT_FRACTION_SCALE = 10000;

   // Default selectivity of a range predicate
   constexpr FLOAT64 RTN_STAT_PRED_RANGE_DEF_SELECTIVITY = 0.05;

   // Default selectivity of a $et predicate
   constexpr FLOAT64 RTN_STAT_PRED_EQ_DEF_SELECTIVITY = 0.005;

   #define RTN_STAT_ROUND( x, min, max ) ( OSS_MIN( OSS_MAX( ( x ), ( min ) ), ( max ) ) )

   #define RTN_STAT_ROUND_SELECTIVITY( x ) RTN_STAT_ROUND( ( x ), ( 0.0 ), ( 1.0 ) )

   OSS_INLINE FLOAT64 RTN_STAT_ROUND_INT( FLOAT64 x )
   {
      return ( ( ( x ) >= 0.0 ) ? floor( ( x ) + 0.5 ) : ceil( (x)-0.5 ) );
   }

} // namespace engine