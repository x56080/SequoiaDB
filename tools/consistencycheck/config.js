/****************************************************************
@decription:   Defined constants
@author:       FangJiabin 2024-11-20
****************************************************************/

// 收集数据节点所有集合的索引信息可以配置多进程收集，该参数是配置进程数。当集合数小于等于进程数时，只会开启一个进程收集索引信息。
const COLLECT_DATA_INDEX_INFO_THREAD_NUM = 12 ;

/*

无法升级索引或者本地集合处理策略

DROP_STRATEGY : 删除索引/集合
RE_CREATE_STRATEGY : 重建索引
IGNORE_STRATEGY : 忽略

*/
const DROP_STRATEGY = 1 ;
const RE_CREATE_STRATEGY = 2 ;
const IGNORE_STRATEGY = 3 ;

/*
   丢失索引处理策略：
   1 表示删除丢失索引
   2 表示重建丢失索引
   3 表示忽略该索引
*/
const DEALWITH_MISS_INDEX = RE_CREATE_STRATEGY ;
/*
   冲突索引处理策略：
   1 表示删除冲突索引
   2 表示重建冲突索引
   3 表示忽略该索引
*/
const DEALWITH_CONFLICT_INDEX = RE_CREATE_STRATEGY ;
/*
   本地集合处理策略：
   1 表示删除本地集合
   3 表示忽略该本地集合
*/
const DEALWITH_LOCAL_CL = DROP_STRATEGY ;

/*

******以下是内部使用的配置，不能更改******

*/
// 无需升级索引
const IDX_TYPE_CONSISTENT    = "Consistent" ;
const IDX_TYPE_STANDALONE    = "Standalone" ;
// 无法升级索引
const IDX_TYPE_MISSING       = "Missing" ;
const IDX_TYPE_CONFLICT      = "Conflict" ;
const IDX_TYPE_INVALID_SHARD = "ResidualShardIndex" ;
const IDX_TYPE_INVALID_ID    = "ResidualIdIndex" ;
const IDX_TYPE_LOCAL_IDX     = "Invalid CL" ;

// 无效集合类型
const INVALID_CL_TYPE_LOCAL = "Local" ;
const INVALID_CL_TYPE_RESIDUAL = "Residual" ;

const MAX_INVALID_CL_TYPE_LENGTH = 11 ; // sizeof(FIELD_INVALID_TYPE)

const MAX_UNIQUEID_LENGTH = 13 ;

const MAX_RECORD_OR_LOB_COUNT_LENGTH = 11 ;

const MAX_CLFULLNAME_LENGTH = 64 ;
const MAX_INDEX_NAME_LENGTH = 64 ;
const MAX_INDEX_KEY_LENGTH = 64 ;

const NO_NEED_MAX_INDEX_TYPE_LENGTH = 10 ;
const MAX_INDEX_TYPE_LENGTH = 18 ;

const MAX_INDEX_ATTR_DESC_LENGTH = 32 ;

const MAX_GROUP_NAME_LENGTH = 20 ;
const MAX_NODE_NAME_LENGTH = 30 ;

const FIELD_ID          = "ID" ;
const FIELD_COLLECTION  = "Collection" ;
const FIELD_INDEXNAME   = "IndexName" ;
const FIELD_INDEXTYPE   = "IndexType" ;
const FIELD_INDEXATTR   = "IndexAttr" ;
const FIELD_INDEXKEY    = "IndexKey" ;
const FIELD_REASON      = "Reason" ;
const FIELD_RESULT      = "Result" ;
const FIELD_RESULTCODE  = "ResultCode" ;
const FIELD_GROUPNAME   = "GroupName" ;
const FIELD_NODENAME    = "NodeName" ;
const FIELD_INVALID_TYPE = "InvalidType" ;
const FIELD_UNIQUEID = "UniqueID" ;
const FIELD_RECORD_COUNT = "RecordCount" ;
const FIELD_LOB_COUNT = "LobCount" ;

const MASK_CLATTR_NOIDIDX = 0x02 ;

const SUGGEST_INFO =  "For indexes that cannot be upgraded, you need to intervene.\n" +
                      "  For missing index on data nodes, you can choose one of the following\n" +
                      "  options:\n" +
                      "    Option 1\n" +
                      "      Description: Create missing indexes to become consistent index.\n" +
                      "      Operation:   Connect to coord node to create index.\n" +
                      "      Influence:   It may takes a long time to create index.\n" +
                      "    Option 2\n" +
                      "      Description: Make existing indexes become standalone index.\n" +
                      "      Operation:   Specify the data node which exists index to create\n" +
                      "                   standalone index, and UniqueID will generate for it.\n" +
                      "      Influence:   Generate UniqueID only at data node.\n" +
                      "\n" +
                      "  For conflicting index on data nodes, you can choose one of the\n" +
                      "  following options:\n" +
                      "    Option 1\n" +
                      "      Description: Drop conflicting indexes to become consistent index.\n" +
                      "      Operation:   Connect to data node with conflicting index to drop\n" +
                      "                   index, then connect to coord node to create index.\n" +
                      "      Influence:   It may takes a long time to create index.\n" +
                      "    Option 2\n" +
                      "      Description: Make existing indexes become standalone index.\n" +
                      "      Operation:   Specify the data node which exists index to create\n" +
                      "                   standalone index, and UniqueID will generate for it.\n" +
                      "      Influence:   Generate UniqueID only at data node.\n" +
                      "\n" +
                      "  For local collection's indexes, you can do nothing.\n\n" ;

const TMP_CS_UPGRADE_INDEX = "tmp_cs_upgrade_index_4d0df605f351abc6" ;
const TMP_CL_CACHE_INFO = "tmp_cl_cache_info" ;
const TMP_CL_GROUP_NODE_INFO = "tmp_cl_group_node_info" ;
const TMP_CL_CATA_CLUSTER_CL_INFO = "tmp_cl_cata_cluster_cl_info" ;
const TMP_CL_CATA_MAIN_CL_INFO = "tmp_cl_cata_main_cl_info" ;
const TMP_CL_DATA_CLUSTER_CL_INFO = "tmp_cl_data_cluster_cl_info" ;
const TMP_CL_LOCAL_CL_INFO = "tmp_cl_local_cl_info" ;
const TMP_CL_DATA_INDEX_INFO = "tmp_cl_data_index_info" ;
const TMP_CL_CATA_INDEX_INFO = "tmp_cl_cata_index_info" ;
// data index info group by ClFullName,GroupName,IndexName
const TMP_CL_DATA_INDEX_CHECK_INFO_TMP = "tmp_cl_data_index_check_info_tmp" ;
// data index info group by ClFullName,GroupName,IndexDef
const TMP_CL_DATA_INDEX_CHECK_INFO = "tmp_cl_data_index_check_info" ;
const TMP_CL_MAIN_CL_INDEX_CHECK_INFO = "tmp_cl_main_cl_index_check_info" ;
const TMP_CL_NO_NEED_UPGRADE_INDEX_INFO = "tmp_cl_no_need_upgrade_index_info" ;
const TMP_CL_CANNOT_UPGRADE_INDEX_INFO = "tmp_cl_cannot_upgrade_index_info" ;
const TMP_CL_INVALID_CL_INFO = "tmp_cl_invalid_cl_info" ;

const TMP_CL_FULL_CACHE_INFO = TMP_CS_UPGRADE_INDEX + "." + TMP_CL_CACHE_INFO ;
const TMP_CL_FULL_GROUP_NODE_INFO = TMP_CS_UPGRADE_INDEX + "." + TMP_CL_GROUP_NODE_INFO ;
const TMP_CL_FULL_CATA_CLUSTER_CL_INFO = TMP_CS_UPGRADE_INDEX + "." + TMP_CL_CATA_CLUSTER_CL_INFO ;
const TMP_CL_FULL_CATA_MAIN_CL_INFO = TMP_CS_UPGRADE_INDEX + "." + TMP_CL_CATA_MAIN_CL_INFO ;
const TMP_CL_FULL_DATA_CLUSTER_CL_INFO = TMP_CS_UPGRADE_INDEX + "." + TMP_CL_DATA_CLUSTER_CL_INFO ;
const TMP_CL_FULL_LOCAL_CL_INFO = TMP_CS_UPGRADE_INDEX + "." + TMP_CL_LOCAL_CL_INFO ;
const TMP_CL_FULL_DATA_INDEX_INFO = TMP_CS_UPGRADE_INDEX + "." + TMP_CL_DATA_INDEX_INFO ;
const TMP_CL_FULL_CATA_INDEX_INFO = TMP_CS_UPGRADE_INDEX + "." + TMP_CL_CATA_INDEX_INFO ;
const TMP_CL_FULL_DATA_INDEX_CHECK_INFO_TMP = TMP_CS_UPGRADE_INDEX + "." + TMP_CL_DATA_INDEX_CHECK_INFO_TMP ;
const TMP_CL_FULL_DATA_INDEX_CHECK_INFO = TMP_CS_UPGRADE_INDEX + "." + TMP_CL_DATA_INDEX_CHECK_INFO ;
const TMP_CL_FULL_MAIN_CL_INDEX_CHECK_INFO = TMP_CS_UPGRADE_INDEX + "." + TMP_CL_MAIN_CL_INDEX_CHECK_INFO ;
const TMP_CL_FULL_NO_NEED_UPGRADE_INDEX_INFO = TMP_CS_UPGRADE_INDEX + "." + TMP_CL_NO_NEED_UPGRADE_INDEX_INFO ;
const TMP_CL_FULL_CANNOT_UPGRADE_INDEX_INFO = TMP_CS_UPGRADE_INDEX + "." + TMP_CL_CANNOT_UPGRADE_INDEX_INFO ;
const TMP_CL_FULL_INVALID_CL_INFO = TMP_CS_UPGRADE_INDEX + "." + TMP_CL_INVALID_CL_INFO ;

const SQL_COMMON_STR = "not Name like '^" + TMP_CS_UPGRADE_INDEX + "\\.'" ;

// 收集数据节点所有集合的索引信息，进程是后台运行。通过循环定时检测进程 PID 是否存在来判断收集信息是否结束。该参数用于配置检测间隔时间，单位 ms
const COLLECT_DATA_INDEX_INFO_SLEEP_TIME = 5000 ;

const PROGRESS_TMP_FILEPATH_PREFIXX = "./getDataIndexInfo_4d0df605f351abc6_"

const _PRINT_PROGRESS_COUNT = 40 ;

// 工具生成的所有临时检查信息会写入临时表中，或从临时表中删除记录，该参数是配置批插或者批删的记录数
const BATCH_NUM = 10000 ;
// 为了使用过多内存或者频繁写文件带来的性能开销，每生成 PRINT_BATCH_NUM 行检查信息就写一次文件
const PRINT_BATCH_NUM = 50 ;
// 为了使用过多内存或者频繁写文件带来的性能开销，每生成 WRITE_JS_FILE_BATCH_NUM 行代码就写一次文件
const WRITE_JS_FILE_BATCH_NUM = 50 ;

const JS_DIR = "./js" ;
// 生成代码文件，升级可以升级的索引
const UPGRADE_INDEX_JS_FILE = JS_DIR + "/upgradeIndexes.js" ;
// 生成代码文件，处理缺失索引，处理策略见 DEALWITH_MISS_INDEX，默认是重建缺失索引
const MISS_INDEX_JS_FILE = JS_DIR + "/missIndexes.js" ;
// 生成代码文件，处理冲突索引，处理策略见 DEALWITH_CONFLICT_INDEX，默认是重建冲突索引
const CONFLICT_INDEX_JS_FILE = JS_DIR + "/conflictIndexes.js" ;
// 生成代码文件，删除非法 $id
const INVALID_ID_INDEX_JS_FILE = JS_DIR + "/invalidIdIndexes.js" ;
// 生成代码文件，处理本地集合，处理策略见 DEALWITH_LOCAL_CL，默认是删除本地集合
const LOCAL_CL_JS_FILE = JS_DIR + "/invalidCls.js" ;

const _CLEAR_STEP_ARR =
[
   "clearTmpCollectionSpace",
   "clearGenerateJsFile"
] ;

const _CHECK_STEP_ARR =
[
   "clearTmpCollectionSpace",
   "clearGenerateJsFile",
   "preCheck1",
   "init",
   "collectNodesInfo",
   "collectCataClusterClInfo",
   "collectCataMainClInfo",
   "collectDataClusterClInfo",
   "collectLocalClInfo",
   "collectCataIndexInfo",
   "collectInvalidClInfos",
   "generateCollectDataIndexInfoPlan",
   "startCollectDataIndexInfoPlan",
   "waitPlanDone",
   "clearTmpFiles",
   "aggregateIndexInfo( aggregate )",
   "aggregateIndexInfo( update UniqueIDCount )",
   "aggregateIndexInfo( aggregate )",
   "checkConflictIndex( check same idx def, diff idx name)",
   "checkConflictIndex( check diff idx def, same idx name)",
   "checkConflictIndex( clear conflict idx infos )",
   "checkMissIndex( collect consistent indexes )",
   "checkMissIndex( collect miss index infos )",
   "checkMissIndex( clear miss index infos )",
   "checkInvalidIdAndShardIdx",
   "checkCataIndexMetaData",
   "checkCanUpgradeAndStandaloneIdx",
   "checkMainClIndexInfo",
   "checkLocalClIndexInfo",
   "get indexes count",
   "writeNoNeedUpgradeIdxReport",
   "writeCanUpgradeIdxReport",
   "writeCannotUpgradeIdxReport",
   "writeLocalClReport"
] ;

const _GENERATE_STEP_ARR =
[
   "preCheck2",
   "checkIfSupportOnlyUpgradeMeta",
   "clearResultFiles",
   "generateCanUpgradeIdxJsScript",
   "generateCannotUpgradeIdxJsScript"
] ;