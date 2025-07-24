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

   Source File Name = catDef.hpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of runtime component. This file contains code logic for
   common functions for coordinator node.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================

   Last Changed =

*******************************************************************************/
#ifndef CATDEF_HPP__
#define CATDEF_HPP__

#include "msgCatalog.hpp"
#include "fmpDef.hpp"

/*
   SYSCAT CollectionSpace define
*/
#define CAT_SYS_SPACE_NAME                "SYSCAT"
#define CAT_NODE_INFO_COLLECTION          CAT_SYS_SPACE_NAME".SYSNODES"
#define CAT_COLLECTION_SPACE_COLLECTION   CAT_SYS_SPACE_NAME".SYSCOLLECTIONSPACES"
#define CAT_COLLECTION_INFO_COLLECTION    CAT_SYS_SPACE_NAME".SYSCOLLECTIONS"
#define CAT_TASK_INFO_COLLECTION          CAT_SYS_SPACE_NAME".SYSTASKS"
#define CAT_DOMAIN_COLLECTION             CAT_SYS_SPACE_NAME".SYSDOMAINS"
#define CAT_HISTORY_COLLECTION            CAT_SYS_SPACE_NAME".SYSHISTORY"

#define CAT_NODEINFO_GROUPNAMEIDX         "{name:\"SYSIDX1\",key: {"\
                                          CAT_GROUPNAME_NAME":1}, unique: true, enforced: true } "
#define CAT_NODEINFO_GROUPIDIDX           "{name:\"SYSIDX2\",key: {"\
                                          CAT_GROUPID_NAME":1}, unique: true, enforced: true } "
#define CAT_COLLECTION_SPACE_NAMEIDX      "{name:\"SYSIDX1\",key: {"\
                                          CAT_COLLECTION_SPACE_NAME":1}, unique: true, enforced: true } "
#define CAT_COLLECTION_NAMEIDX            "{name:\"SYSIDX1\",key: {"\
                                          CAT_COLLECTION_NAME":1}, unique: true, enforced: true } "
#define CAT_COLLECTION_MAINCLIDX          "{name:\"SYSIDX2\",key: {"\
                                          CAT_MAINCL_NAME":1}} "
#define CAT_TASK_INFO_CLOBJIDX            "{name:\"SYSIDX1\",key: {"\
                                          CAT_TASKID_NAME":1}, unique: true }"
#define CAT_DOMAIN_NAMEIDX                "{name:\"SYSIDX1\",key: {"\
                                          CAT_DOMAINNAME_NAME":1}, unique: true, enforced: true } "
#define CAT_HISTORY_BUCKETID_IDX          "{name:\"SYSIDX1\",key: {"\
                                          FIELD_NAME_BUCKETID":1}, unique: true, enforced: true } "

#define CAT_MATCHER_HOST_NAME             CAT_HOST_FIELD_NAME
#define CAT_MATCHER_NODEID_NAME           CAT_GROUP_NAME"."CAT_NODEID_NAME
#define CAT_MATCHER_SERVICE_NAME          CAT_SERVICE_FIELD_NAME"."CAT_SERVICE_NAME_FIELD_NAME

#define READ_BUFFER_SIZE                  8192

#define CAT_VERSION_BEGIN                 1

#define CAT_HASH_LOW_BOUND                0

#define CAT_SHARDING_PARTITION_DEFAULT    4096       // 2^12
#define CAT_SHARDING_PARTITION_MIN        8          // 2^3
#define CAT_SHARDING_PARTITION_MAX        1048576    // 2^20

#define CAT_BUCKET_SIZE                   ( 200000 )
#define FIELD_NAME_BUCKETID               "BucketID"

#define CAT_SYS_DOMAIN_NAME               SYS_PREFIX"DOMAIN"

/*
   SYSPROCEDURES CollectionSpace define
*/
#define CAT_PROCEDURES_SPACE_NAME         "SYSPROCEDURES"
#define CAT_PROCEDURES_COLLECTION         CAT_PROCEDURES_SPACE_NAME".STOREPROCEDURES"

#define CAT_PROCEDURES_COLLECTION_INDEX   "{name:\"PROCEDUREIDX1\", key: {"\
                                          FMP_FUNC_NAME":1}, unique: true, enforced: true } "

/*
   SYSINFO CollectionSpace define
*/
#define CAT_SYSINFO_SPACE_NAME            "SYSINFO"
#define CAT_SYSDCBASE_COLLECTION_NAME     CAT_SYSINFO_SPACE_NAME".SYSDCBASE"

#define CAT_DCBASEINFO_TYPE_INDEX         "{name:\"SYSTYPE\",key: {"\
                                          FIELD_NAME_TYPE":1}, unique: true, enforced: true } "

#define CAT_BASE_TYPE_GLOBAL_STR          "GLOBAL"

#define CAT_SYSLOG_COLLECTION_NAME        CAT_SYSINFO_SPACE_NAME".SYSLOG"
#define CAT_SYSLOG_CL_NUM                 ( 5 )
#define CAT_SYSLOG_CL_MAX_COUNT           ( 100000 )

#define CAT_SYSLOG_TYPE_LSNVER            "{name:\"SYSLSNVER\",key: {"\
                                          FIELD_NAME_LSN_VERSION":1}, unique: false } "
#define CAT_SYSLOG_TYPE_LSNOFF            "{name:\"SYSLSNOFF\",key: {"\
                                          FIELD_NAME_LSN_OFFSET":1}, unique: true, enforced: true } "

#endif // CATDEF_HPP__

