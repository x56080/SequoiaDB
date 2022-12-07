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