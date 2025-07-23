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

   Source File Name = clsFSDef.hpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of Replication component. This file contains structure for
   replication control block.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/14/2012  YW  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef CLSFSDEF_HPP_
#define CLSFSDEF_HPP_

#include "core.hpp"

namespace engine
{

   #define CLS_FS_VALIDCLS                "validcls"
   #define CLS_FS_COMMITFLAG              "commitflag"
   #define CLS_FS_COMMITLSN               "commitlsn"
   #define CLS_FS_CS_NAME                 "cs"
   #define CLS_FS_COLLECTION_NAME         "collection"
   #define CLS_FS_COLLECTION_UNIQUEID     "cluniqueid"
   #define CLS_FS_CS_META_NAME            "csmeta"
   #define CLS_FS_PAGE_SIZE               "pagesize"
   #define CLS_FS_KEYOBJ                  "keyobj"
   #define CLS_FS_END_KEYOBJ              "endkeyobj"
   #define CLS_FS_NOMORE                  "nomore"
   #define CLS_FS_SLICE                   "slice"
   #define CLS_FS_INDEXES                 "indexes"
   #define CLS_FS_FULLNAME                "fullname"
   #define CLS_FS_FULLNAMES               "fullnames"
   #define CLS_FS_CSNAME                  "csname"
   #define CLS_FS_CSNAMES                 "csnames"
   #define CLS_FS_CS_UNIQUEID             "csuniqueid"
   #define CLS_FS_NEEDDATA                "needdata"
   #define CLS_FS_ATTRIBUTES              "attributes"
   #define CLS_FS_LOB_PAGE_SIZE           "lobpagesize"
   #define CLS_FS_CS_TYPE                 "cstype"
   #define CLS_FS_COMP_TYPE               "comptype"
   #define CLS_FS_EXT_OPTION              "extoption"
   #define CLS_FS_IDIDX_DEF               "idIdxDef"
   #define CLS_FS_COMP_DICT               "compdict"

   #define CLS_FS_SYNC_SPEED( syncSize, timeSpent ) \
           0 == (timeSpent) ? 0 : \
           ( (FLOAT64)(syncSize) / ( 1024 * 1024 ) / (timeSpent) ) * 1000

   #define CLS_FS_TIME_SPENT( timeSpent ) \
           (timeSpent) / ( 1000 * 60 )

   enum CLS_FS_STATUS
   {
      CLS_FS_STATUS_NONE = 0,
      CLS_FS_STATUS_BEGIN,
      CLS_FS_STATUS_META,
      CLS_FS_STATUS_INDEX,
      CLS_FS_STATUS_NOTIFY_DOC,
      CLS_FS_STATUS_NOTIFY_LOB,
      CLS_FS_STATUS_NOTIFY_LOG,
      CLS_FS_STATUS_END
   } ;
}

#endif

