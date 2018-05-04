/*******************************************************************************


   Copyright (C) 2011-2018 SequoiaDB Ltd.

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

******************************************************************************/
#ifndef AGGRDEF_HPP__
#define AGGRDEF_HPP__

#define AGGR_KEYWORD_PREFIX               '$'
#define AGGR_CL_DEFAULT_ALIAS             "SYS_AGGR_ALIAS"

#define AGGR_KEYWORD_PREFIX_STR           "$"
#define AGGR_GROUP_PARSER_NAME            AGGR_KEYWORD_PREFIX_STR"group"
#define AGGR_MATCH_PARSER_NAME            AGGR_KEYWORD_PREFIX_STR"match"
#define AGGR_SKIP_PARSER_NAME             AGGR_KEYWORD_PREFIX_STR"skip"
#define AGGR_LIMIT_PARSER_NAME            AGGR_KEYWORD_PREFIX_STR"limit"
#define AGGR_SORT_PARSER_NAME             AGGR_KEYWORD_PREFIX_STR"sort"
#define AGGR_PROJECT_PARSER_NAME          AGGR_KEYWORD_PREFIX_STR"project"

#endif