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

   Source File Name = aggrDef.hpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains functions for agent processing.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/04/2013  JHL  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef AGGRDEF_HPP__
#define AGGRDEF_HPP__

#define AGGR_KEYWORD_PREFIX               '$'
#define AGGR_CL_DEFAULT_ALIAS             "SYS_AGGR_ALIAS"

#define AGGR_KEYWORD_PREFIX_STR           "$"

// Aggregation operations
#define AGGR_OPR_GROUP_NAME               AGGR_KEYWORD_PREFIX_STR"group"
#define AGGR_OPR_MATCH_NAME               AGGR_KEYWORD_PREFIX_STR"match"
#define AGGR_OPR_SKIP_NAME                AGGR_KEYWORD_PREFIX_STR"skip"
#define AGGR_OPR_LIMIT_NAME               AGGR_KEYWORD_PREFIX_STR"limit"
#define AGGR_OPR_SORT_NAME                AGGR_KEYWORD_PREFIX_STR"sort"
#define AGGR_OPR_PROJECT_NAME             AGGR_KEYWORD_PREFIX_STR"project"
#define AGGR_OPR_UNWIND_NAME              AGGR_KEYWORD_PREFIX_STR"unwind"

#endif
