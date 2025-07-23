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

   Source File Name = sdb_fdw_util.h

   Descriptive Name = N/A

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who         Description
   ====== =========== =========== =============================================
          23/07/2025  fangjiabin  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef SDB_FDW_UTIL_H__
#define SDB_FDW_UTIL_H__

#include "client.h"
#include "fmgr.h"
#include "catalog/pg_foreign_server.h"
#include "catalog/pg_foreign_table.h"
#include "nodes/relation.h"
#include "sdb_fdw.h"

#define SDB_FIELD_COMMA              ","
#define SDB_FIELD_COMMA_CHR          ','
#define SDB_FIELD_SEMICOLON          ":"
#define SDB_FIELD_SEMICOLON_CHR      ':'


typedef enum
{
   SDB_VAR_VAR = 0,
   SDB_PARAM_VAR ,
   SDB_VAR_CONST ,
   SDB_UNSUPPORT_ARG_TYPE
} EnumSdbArgType;

typedef struct
{
   bool     nonempty;      /* True if lists are not all empty */
   /* Lists of RestrictInfos, one per index column */
   List     *indexclauses[INDEX_MAX_KEYS];
} sdbIndexClauseSet;

void sdbGetIndexEqclause( PlannerInfo *root, RelOptInfo *baserel, Oid tableID,
                          sdbIndexInfo *indexInfo,
                          sdbIndexClauseSet *clauseset ) ;

void sdbMatchJoinClausesToIndex( PlannerInfo *root, RelOptInfo *rel,
                                 Oid tableID, sdbIndexInfo *index,
                                 sdbIndexClauseSet *clauseset ) ;

void sdbMatchClauseToIndex( RelOptInfo *rel, Oid tableID, sdbIndexInfo *index,
                            RestrictInfo *rinfo,
                            sdbIndexClauseSet *clauseset ) ;

bool sdbMatchClauseToIndexcol( RelOptInfo *rel, Oid tableID,
                               sdbIndexInfo *index, int indexcol,
                               RestrictInfo *rinfo ) ;

bool isSortCanPushDown( PlannerInfo *root, Index foreignTableIndex ) ;

INT32 sdbGenerateSortCondition ( Index foreignTableIndex, Oid foreign_id,
                                 List *sort_paths, sdbbson *condition ) ;

void sdbPreprocessLimit(PlannerInfo *root, INT64 *offset, INT64 *limit);

//int sdbGetIndexInfo( SdbExecState *sdbState, sdbIndexInfo *indexInfo ) ;

int sdbGetIndexInfos( SdbExecState *sdbState, sdbIndexInfo *indexInfo,
                      int maxNum, int *indexNum ) ;


sdbConnectionHandle sdbGetConnectionHandle( const char **serverList,
                                            int serverNum,
                                            const char *usr,
                                            const char *passwd,
                                            int useCipher,
                                            const char *token,
                                            const char *cipherfile,
                                            const char *preference_instance,
                                            const char *preference_instance_mode,
                                            int session_timeout,
                                            const char *transaction ) ;

sdbCollectionHandle sdbGetSdbCollection( sdbConnectionHandle connectionHandle,
      const char *sdbcs, const char *sdbcl ) ;


SdbConnectionPool *sdbGetConnectionPool() ;

int sdbSetConnectionPreference( sdbConnectionHandle hConnection, CHAR *preference_instance,
                                const CHAR *preference_instance_mode, INT32 session_timeout ) ;

BOOLEAN sdbIsInterrupt() ;

void sdbReleaseConnectionFromPool(int index) ;

IndexPath *sdb_build_index_paths(PlannerInfo *root, RelOptInfo *rel,
              sdbIndexInfo *sdbIndex, sdbIndexClauseSet *clauses,
              SdbExecState *fdw_state);

IndexPath *sdb_create_index_path(PlannerInfo *root, RelOptInfo *rel,
                                 IndexOptInfo *index, List *indexclauses,
                                 List *indexclausecols, List *indexorderbys,
                                 List *indexorderbycols, List *pathkeys,
                                 ScanDirection indexscandir, bool indexonly,
                                 Relids required_outer, double loop_count,
                                 SdbExecState *fdw_state);

EnumSdbArgType getArgumentType(List *arguments);

int sdbGenerateRescanCondition(SdbExecState *fdw_state, PlanState *planState,
                               sdbbson *rescanCondition);

void sdbPrintBson( sdbbson *bson, int log_level, const char *label ) ;

void debugClauseInfo( PlannerInfo *root, RelOptInfo *baserel, Oid tableID ) ;

Var *getRealVar( Expr *arg ) ;

/* record cache */
typedef struct
{
   sdbbson *record ;
   UINT64 key ;
   BOOLEAN isUsed ;
} SdbRecordItem ;


#define SDB_MAX_RECORD_SIZE 100
typedef struct
{
   INT32 size ;
   INT32 usedCount ;
   SdbRecordItem recordArray[ SDB_MAX_RECORD_SIZE ] ;
} SdbRecordCache ;

void sdbInitRecordCache() ;
void sdbFiniRecordCache() ;

SdbRecordCache *sdbGetRecordCache() ;
sdbbson *sdbAllocRecord( SdbRecordCache *recordCache, SdbExecState *fdw_state ) ;
sdbbson *sdbGetRecord( SdbRecordCache *recordCache, SdbExecState *fdw_state ) ;
void sdbReleaseRecord( SdbRecordCache *recordCache, SdbExecState *fdw_state ) ;


#ifdef SDB_USE_OWN_POSTGRES
void sdbuseownpostgres() ;
#endif /* SDB_USE_OWN_POSTGRES */



#endif /*SDB_FDW_UTIL_H__*/


