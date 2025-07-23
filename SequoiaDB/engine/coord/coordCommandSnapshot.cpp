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

   Source File Name = coordCommandSnapshot.cpp

   Descriptive Name = Coord Commands

   When/how to use: this program may be used on binary and text-formatted
   versions of runtime component. This file contains code logic for
   common functions for coordinator node.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09-23-2016  XJH Init
   Last Changed =

*******************************************************************************/
#include "coordCommandSnapshot.hpp"
#include "coordSnapshotDef.hpp"
#include "catDef.hpp"
#include "pdTrace.hpp"
#include "coordTrace.hpp"
#include "catGTSDef.hpp"

using namespace bson ;

namespace engine
{

   /*
      _coordCMDSnapshotIntrBase implement
   */
   _coordCMDSnapshotIntrBase::_coordCMDSnapshotIntrBase()
   {
   }

   _coordCMDSnapshotIntrBase::~_coordCMDSnapshotIntrBase()
   {
   }

   /*
      _coordCMDSnapshotCurIntrBase implement
   */
   _coordCMDSnapshotCurIntrBase::_coordCMDSnapshotCurIntrBase()
   {
   }

   _coordCMDSnapshotCurIntrBase::~_coordCMDSnapshotCurIntrBase()
   {
   }

   /*
      _coordCmdSnapshotReset implement
   */
   COORD_IMPLEMENT_CMD_AUTO_REGISTER( _coordCmdSnapshotReset,
                                      CMD_NAME_SNAPSHOT_RESET,
                                      TRUE ) ;
   _coordCmdSnapshotReset::_coordCmdSnapshotReset()
   {
   }

   _coordCmdSnapshotReset::~_coordCmdSnapshotReset()
   {
   }

   void _coordCmdSnapshotReset::_preSet( pmdEDUCB *cb,
                                         coordCtrlParam &ctrlParam )
   {
      // catalog / data has been set with default 1
      ctrlParam._role[ SDB_ROLE_COORD ] = 1 ;
   }

   /*
      _coordSnapshotTransCurIntr implement
   */
   COORD_IMPLEMENT_CMD_AUTO_REGISTER( _coordSnapshotTransCurIntr,
                                      CMD_NAME_SNAPSHOT_TRANSCUR_INTR,
                                      TRUE ) ;
   _coordSnapshotTransCurIntr::_coordSnapshotTransCurIntr()
   {
   }

   _coordSnapshotTransCurIntr::~_coordSnapshotTransCurIntr()
   {
   }

   void _coordSnapshotTransCurIntr::_preSet( pmdEDUCB *cb,
                                             coordCtrlParam &ctrlParam )
   {
      ctrlParam._role[ SDB_ROLE_CATALOG ] = 0 ;
      ctrlParam._emptyFilterSel = NODE_SEL_PRIMARY ;

      ctrlParam._useSpecialNode = TRUE ;
      _groupSession.getPropSite()->dumpTransNode( ctrlParam._specialNodes ) ;
   }

   /*
      _coordSnapshotTransIntr implement
   */
   COORD_IMPLEMENT_CMD_AUTO_REGISTER( _coordSnapshotTransIntr,
                                      CMD_NAME_SNAPSHOT_TRANS_INTR,
                                      TRUE ) ;
   _coordSnapshotTransIntr::_coordSnapshotTransIntr()
   {
   }

   _coordSnapshotTransIntr::~_coordSnapshotTransIntr()
   {
   }

   void _coordSnapshotTransIntr::_preSet( pmdEDUCB *cb,
                                          coordCtrlParam &ctrlParam )
   {
      ctrlParam._role[ SDB_ROLE_CATALOG ] = 0 ;
      ctrlParam._emptyFilterSel = NODE_SEL_PRIMARY ;
   }

   /*
      _coordSnapshotTransCur implement
   */
   COORD_IMPLEMENT_CMD_AUTO_REGISTER( _coordSnapshotTransCur,
                                      CMD_NAME_SNAPSHOT_TRANSACTIONS_CUR,
                                      TRUE ) ;
   _coordSnapshotTransCur::_coordSnapshotTransCur()
   {
   }

   _coordSnapshotTransCur::~_coordSnapshotTransCur()
   {
   }

   const CHAR* _coordSnapshotTransCur::getIntrCMDName()
   {
      return CMD_ADMIN_PREFIX CMD_NAME_SNAPSHOT_TRANSCUR_INTR ;
   }

   const CHAR* _coordSnapshotTransCur::getInnerAggrContent()
   {
      return NULL ;
   }

   /*
      _coordSnapshotTrans implement
   */
   COORD_IMPLEMENT_CMD_AUTO_REGISTER( _coordSnapshotTrans,
                                      CMD_NAME_SNAPSHOT_TRANSACTIONS,
                                      TRUE ) ;
   _coordSnapshotTrans::_coordSnapshotTrans()
   {
   }

   _coordSnapshotTrans::~_coordSnapshotTrans()
   {
   }

   const CHAR* _coordSnapshotTrans::getIntrCMDName()
   {
      return CMD_ADMIN_PREFIX CMD_NAME_SNAPSHOT_TRANS_INTR ;
   }

   const CHAR* _coordSnapshotTrans::getInnerAggrContent()
   {
      return NULL ;
   }

   /*
      _coordCMDSnapshotDataBase implement
   */
   COORD_IMPLEMENT_CMD_AUTO_REGISTER( _coordCMDSnapshotDataBase,
                                      CMD_NAME_SNAPSHOT_DATABASE,
                                      TRUE ) ;
   _coordCMDSnapshotDataBase::_coordCMDSnapshotDataBase()
   {
   }

   _coordCMDSnapshotDataBase::~_coordCMDSnapshotDataBase()
   {
   }

   const CHAR* _coordCMDSnapshotDataBase::getIntrCMDName()
   {
      return CMD_ADMIN_PREFIX CMD_NAME_SNAPSHOT_DATABASE_INTR;
   }

   const CHAR* _coordCMDSnapshotDataBase::getInnerAggrContent()
   {
      return COORD_SNAPSHOTDB_INPUT ;
   }

   /*
      _coordCMDSnapshotDataBaseIntr implement
   */
   COORD_IMPLEMENT_CMD_AUTO_REGISTER( _coordCMDSnapshotDataBaseIntr,
                                      CMD_NAME_SNAPSHOT_DATABASE_INTR,
                                      TRUE ) ;
   _coordCMDSnapshotDataBaseIntr::_coordCMDSnapshotDataBaseIntr()
   {
   }

   _coordCMDSnapshotDataBaseIntr::~_coordCMDSnapshotDataBaseIntr()
   {
   }

   void _coordCMDSnapshotDataBaseIntr::_preSet( pmdEDUCB *cb,
                                                coordCtrlParam &ctrlParam )
   {
      // catalog / data has been selected in constructor of _coordCtrlParam
      ctrlParam._role[ SDB_ROLE_COORD ] = 1 ;
   }

   /*
      _coordCMDSnapshotSystem implement
   */
   COORD_IMPLEMENT_CMD_AUTO_REGISTER( _coordCMDSnapshotSystem,
                                      CMD_NAME_SNAPSHOT_SYSTEM,
                                      TRUE ) ;
   _coordCMDSnapshotSystem::_coordCMDSnapshotSystem()
   {
   }

   _coordCMDSnapshotSystem::~_coordCMDSnapshotSystem()
   {
   }

   const CHAR* _coordCMDSnapshotSystem::getIntrCMDName()
   {
      return CMD_ADMIN_PREFIX CMD_NAME_SNAPSHOT_SYSTEM_INTR;
   }

   const CHAR* _coordCMDSnapshotSystem::getInnerAggrContent()
   {
      return COORD_SNAPSHOTSYS_INPUT ;
   }

   /*
      _coordCMDSnapshotSystemIntr implement
   */
   COORD_IMPLEMENT_CMD_AUTO_REGISTER( _coordCMDSnapshotSystemIntr,
                                      CMD_NAME_SNAPSHOT_SYSTEM_INTR,
                                      TRUE ) ;
   _coordCMDSnapshotSystemIntr::_coordCMDSnapshotSystemIntr()
   {
   }

   _coordCMDSnapshotSystemIntr::~_coordCMDSnapshotSystemIntr()
   {
   }

   /*
      _coordCMDSnapshotHealth implement
   */
   COORD_IMPLEMENT_CMD_AUTO_REGISTER( _coordCMDSnapshotHealth,
                                      CMD_NAME_SNAPSHOT_HEALTH,
                                      TRUE ) ;
   _coordCMDSnapshotHealth::_coordCMDSnapshotHealth()
   {
   }

   _coordCMDSnapshotHealth::~_coordCMDSnapshotHealth()
   {
   }

   const CHAR* _coordCMDSnapshotHealth::getIntrCMDName()
   {
      return CMD_ADMIN_PREFIX CMD_NAME_SNAPSHOT_HEALTH_INTR ;
   }

   const CHAR* _coordCMDSnapshotHealth::getInnerAggrContent()
   {
      return NULL ;
   }

   /*
      _coordCMDSnapshotHealthIntr implement
   */
   COORD_IMPLEMENT_CMD_AUTO_REGISTER( _coordCMDSnapshotHealthIntr,
                                      CMD_NAME_SNAPSHOT_HEALTH_INTR,
                                      TRUE ) ;
   _coordCMDSnapshotHealthIntr::_coordCMDSnapshotHealthIntr()
   {
   }

   _coordCMDSnapshotHealthIntr::~_coordCMDSnapshotHealthIntr()
   {
   }

   void _coordCMDSnapshotHealthIntr::_preSet( pmdEDUCB *cb,
                                              coordCtrlParam &ctrlParam )
   {
      // catalog / data has been selected in constructor of _coordCtrlParam
      ctrlParam._role[ SDB_ROLE_COORD ] = 1 ;
   }

   /*
      _coordCMDSnapshotCollections implement
   */
   COORD_IMPLEMENT_CMD_AUTO_REGISTER( _coordCMDSnapshotCollections,
                                      CMD_NAME_SNAPSHOT_COLLECTIONS,
                                      TRUE ) ;
   _coordCMDSnapshotCollections::_coordCMDSnapshotCollections()
   {
   }

   _coordCMDSnapshotCollections::~_coordCMDSnapshotCollections()
   {
   }

   const CHAR* _coordCMDSnapshotCollections::getIntrCMDName()
   {
      return CMD_ADMIN_PREFIX CMD_NAME_SNAPSHOT_COLLECTION_INTR;
   }

   const CHAR* _coordCMDSnapshotCollections::getInnerAggrContent()
   {
      return COORD_SNAPSHOTCL_INPUT ;
   }

   /*
      _coordCMDSnapshotCLIntr implement
   */
   COORD_IMPLEMENT_CMD_AUTO_REGISTER( _coordCMDSnapshotCLIntr,
                                      CMD_NAME_SNAPSHOT_COLLECTION_INTR,
                                      TRUE ) ;
   _coordCMDSnapshotCLIntr::_coordCMDSnapshotCLIntr()
   {
   }

   _coordCMDSnapshotCLIntr::~_coordCMDSnapshotCLIntr()
   {
   }

   /*
      _coordCMDSnapshotSpaces implement
   */
   COORD_IMPLEMENT_CMD_AUTO_REGISTER( _coordCMDSnapshotSpaces,
                                      CMD_NAME_SNAPSHOT_COLLECTIONSPACES,
                                      TRUE ) ;
   _coordCMDSnapshotSpaces::_coordCMDSnapshotSpaces()
   {
   }

   _coordCMDSnapshotSpaces::~_coordCMDSnapshotSpaces()
   {
   }

   const CHAR* _coordCMDSnapshotSpaces::getIntrCMDName()
   {
      return CMD_ADMIN_PREFIX CMD_NAME_SNAPSHOT_SPACE_INTR;
   }

   const CHAR* _coordCMDSnapshotSpaces::getInnerAggrContent()
   {
      return COORD_SNAPSHOTCS_INPUT ;
   }

   /*
      _coordCMDSnapshotCSIntr implement
   */
   COORD_IMPLEMENT_CMD_AUTO_REGISTER( _coordCMDSnapshotCSIntr,
                                      CMD_NAME_SNAPSHOT_SPACE_INTR,
                                      TRUE ) ;
   _coordCMDSnapshotCSIntr::_coordCMDSnapshotCSIntr()
   {
   }

   _coordCMDSnapshotCSIntr::~_coordCMDSnapshotCSIntr()
   {
   }

   /*
      _coordCMDSnapshotContexts implement
   */
   COORD_IMPLEMENT_CMD_AUTO_REGISTER( _coordCMDSnapshotContexts,
                                      CMD_NAME_SNAPSHOT_CONTEXTS,
                                      TRUE ) ;
   _coordCMDSnapshotContexts::_coordCMDSnapshotContexts()
   {
   }

   _coordCMDSnapshotContexts::~_coordCMDSnapshotContexts()
   {
   }

   const CHAR* _coordCMDSnapshotContexts::getIntrCMDName()
   {
      return CMD_ADMIN_PREFIX CMD_NAME_SNAPSHOT_CONTEXT_INTR;
   }

   const CHAR* _coordCMDSnapshotContexts::getInnerAggrContent()
   {
      return COORD_SNAPSHOTCONTEXTS_INPUT ;
   }

   /*
      _coordCMDSnapshotContextIntr implement
   */
   COORD_IMPLEMENT_CMD_AUTO_REGISTER( _coordCMDSnapshotContextIntr,
                                      CMD_NAME_SNAPSHOT_CONTEXT_INTR,
                                      TRUE ) ;
   _coordCMDSnapshotContextIntr::_coordCMDSnapshotContextIntr()
   {
   }

   _coordCMDSnapshotContextIntr::~_coordCMDSnapshotContextIntr()
   {
   }

   /*
      _coordCMDSnapshotContextsCur implement
   */
   COORD_IMPLEMENT_CMD_AUTO_REGISTER( _coordCMDSnapshotContextsCur,
                                      CMD_NAME_SNAPSHOT_CONTEXTS_CURRENT,
                                      TRUE ) ;
   _coordCMDSnapshotContextsCur::_coordCMDSnapshotContextsCur()
   {
   }

   _coordCMDSnapshotContextsCur::~_coordCMDSnapshotContextsCur()
   {
   }

   const CHAR* _coordCMDSnapshotContextsCur::getIntrCMDName()
   {
      return CMD_ADMIN_PREFIX CMD_NAME_SNAPSHOT_CONTEXTCUR_INTR;
   }

   const CHAR* _coordCMDSnapshotContextsCur::getInnerAggrContent()
   {
      return COORD_SNAPSHOTCONTEXTSCUR_INPUT ;
   }

   /*
      _coordCMDSnapshotContextCurIntr implement
   */
   COORD_IMPLEMENT_CMD_AUTO_REGISTER( _coordCMDSnapshotContextCurIntr,
                                      CMD_NAME_SNAPSHOT_CONTEXTCUR_INTR,
                                      TRUE ) ;
   _coordCMDSnapshotContextCurIntr::_coordCMDSnapshotContextCurIntr()
   {
   }

   _coordCMDSnapshotContextCurIntr::~_coordCMDSnapshotContextCurIntr()
   {
   }

   /*
      _coordCMDSnapshotSessions implement
   */
   COORD_IMPLEMENT_CMD_AUTO_REGISTER( _coordCMDSnapshotSessions,
                                      CMD_NAME_SNAPSHOT_SESSIONS,
                                      TRUE ) ;
   _coordCMDSnapshotSessions::_coordCMDSnapshotSessions()
   {
   }

   _coordCMDSnapshotSessions::~_coordCMDSnapshotSessions()
   {
   }

   const CHAR* _coordCMDSnapshotSessions::getIntrCMDName()
   {
      return CMD_ADMIN_PREFIX CMD_NAME_SNAPSHOT_SESSION_INTR;
   }

   const CHAR* _coordCMDSnapshotSessions::getInnerAggrContent()
   {
      return COORD_SNAPSHOTSESS_INPUT ;
   }

   /*
      _coordCMDSnapshotSessionIntr implement
   */
   COORD_IMPLEMENT_CMD_AUTO_REGISTER( _coordCMDSnapshotSessionIntr,
                                      CMD_NAME_SNAPSHOT_SESSION_INTR,
                                      TRUE ) ;
   _coordCMDSnapshotSessionIntr::_coordCMDSnapshotSessionIntr()
   {
   }

   _coordCMDSnapshotSessionIntr::~_coordCMDSnapshotSessionIntr()
   {
   }

   /*
      _coordCMDSnapshotSessionsCur implement
   */
   COORD_IMPLEMENT_CMD_AUTO_REGISTER( _coordCMDSnapshotSessionsCur,
                                      CMD_NAME_SNAPSHOT_SESSIONS_CURRENT,
                                      TRUE ) ;
   _coordCMDSnapshotSessionsCur::_coordCMDSnapshotSessionsCur()
   {
   }

   _coordCMDSnapshotSessionsCur::~_coordCMDSnapshotSessionsCur()
   {
   }

   const CHAR* _coordCMDSnapshotSessionsCur::getIntrCMDName()
   {
      return CMD_ADMIN_PREFIX CMD_NAME_SNAPSHOT_SESSIONCUR_INTR;
   }

   const CHAR* _coordCMDSnapshotSessionsCur::getInnerAggrContent()
   {
      return COORD_SNAPSHOTSESSCUR_INPUT ;
   }

   /*
      _coordCMDSnapshotSessionCurIntr implement
   */
   COORD_IMPLEMENT_CMD_AUTO_REGISTER( _coordCMDSnapshotSessionCurIntr,
                                      CMD_NAME_SNAPSHOT_SESSIONCUR_INTR,
                                      TRUE ) ;
   _coordCMDSnapshotSessionCurIntr::_coordCMDSnapshotSessionCurIntr()
   {
   }

   _coordCMDSnapshotSessionCurIntr::~_coordCMDSnapshotSessionCurIntr()
   {
   }

   /*
      _coordCMDSnapshotCata implement
   */
   COORD_IMPLEMENT_CMD_AUTO_REGISTER( _coordCMDSnapshotCata,
                                      CMD_NAME_SNAPSHOT_CATA,
                                      TRUE ) ;
   _coordCMDSnapshotCata::_coordCMDSnapshotCata()
   {
   }

   _coordCMDSnapshotCata::~_coordCMDSnapshotCata()
   {
   }

   INT32 _coordCMDSnapshotCata::_preProcess( rtnQueryOptions &queryOpt,
                                             string &clName,
                                             BSONObj &outSelector )
   {
      clName = CAT_COLLECTION_INFO_COLLECTION ;
      return SDB_OK ;
   }

   INT32 _coordCMDSnapshotCata::_processVCS( rtnQueryOptions &queryOpt,
                                             const CHAR *pName,
                                             rtnContext *pContext )
   {
      INT32 rc = SDB_OK ;

      if ( 0 == ossStrcmp( pName,
                           CMD_ADMIN_PREFIX SYS_CL_SESSION_INFO ) )
      {
         pContext->append( BSON( FIELD_NAME_NAME << pName <<
                                 FIELD_NAME_VERSION << 1 ) ) ;
      }
      else
      {
         rc = _coordCMDQueryBase::_processVCS( queryOpt, pName, pContext ) ;
      }

      return rc ;
   }

   /*
      _coordCMDSnapshotCataIntr implement
   */
   COORD_IMPLEMENT_CMD_AUTO_REGISTER( _coordCMDSnapshotCataIntr,
                                      CMD_NAME_SNAPSHOT_CATA_INTR,
                                      TRUE ) ;
   _coordCMDSnapshotCataIntr::_coordCMDSnapshotCataIntr()
   {
   }

   _coordCMDSnapshotCataIntr::~_coordCMDSnapshotCataIntr()
   {
   }

   /*
      _coordCMDSnapshotAccessPlans implement
   */
   COORD_IMPLEMENT_CMD_AUTO_REGISTER( _coordCMDSnapshotAccessPlans,
                                      CMD_NAME_SNAPSHOT_ACCESSPLANS,
                                      TRUE ) ;
   _coordCMDSnapshotAccessPlans::_coordCMDSnapshotAccessPlans ()
   {
   }

   _coordCMDSnapshotAccessPlans::~_coordCMDSnapshotAccessPlans ()
   {
   }

   const CHAR* _coordCMDSnapshotAccessPlans::getIntrCMDName ()
   {
      return CMD_ADMIN_PREFIX CMD_NAME_SNAPSHOT_ACCESSPLANS_INTR ;
   }

   const CHAR* _coordCMDSnapshotAccessPlans::getInnerAggrContent()
   {
      return NULL ;
   }

   /*
      _coordCMDSnapshotAccessPlansIntr implement
   */
   COORD_IMPLEMENT_CMD_AUTO_REGISTER( _coordCMDSnapshotAccessPlansIntr,
                                      CMD_NAME_SNAPSHOT_ACCESSPLANS_INTR,
                                      TRUE ) ;
   _coordCMDSnapshotAccessPlansIntr::_coordCMDSnapshotAccessPlansIntr ()
   {
   }

   _coordCMDSnapshotAccessPlansIntr::~_coordCMDSnapshotAccessPlansIntr ()
   {
   }

   /*
      _coordCMDSnapshotConfigs implement
   */
   COORD_IMPLEMENT_CMD_AUTO_REGISTER( _coordCMDSnapshotConfigs,
                                      CMD_NAME_SNAPSHOT_CONFIGS,
                                      TRUE ) ;
   _coordCMDSnapshotConfigs::_coordCMDSnapshotConfigs()
   {
   }

   _coordCMDSnapshotConfigs::~_coordCMDSnapshotConfigs()
   {
   }

   const CHAR* _coordCMDSnapshotConfigs::getIntrCMDName()
   {
      return CMD_ADMIN_PREFIX CMD_NAME_SNAPSHOT_CONFIGS_INTR;
   }

   const CHAR* _coordCMDSnapshotConfigs::getInnerAggrContent()
   {
      return NULL ;
   }

   /*
      _coordCMDSnapshotConfigsIntr implement
   */
   COORD_IMPLEMENT_CMD_AUTO_REGISTER( _coordCMDSnapshotConfigsIntr,
                                      CMD_NAME_SNAPSHOT_CONFIGS_INTR,
                                      TRUE ) ;
   _coordCMDSnapshotConfigsIntr::_coordCMDSnapshotConfigsIntr()
   {
   }

   _coordCMDSnapshotConfigsIntr::~_coordCMDSnapshotConfigsIntr()
   {
   }

   void _coordCMDSnapshotConfigsIntr::_preSet( pmdEDUCB *cb,
                                               coordCtrlParam &ctrlParam )
   {
      // catalog / data has been set with default 1
      ctrlParam._role[ SDB_ROLE_COORD ] = 1 ;
   }

   /*
      _coordCMDSnapshotSvcTasks implement
   */
   COORD_IMPLEMENT_CMD_AUTO_REGISTER( _coordCMDSnapshotSvcTasks,
                                      CMD_NAME_SNAPSHOT_SVCTASKS,
                                      TRUE ) ;

   _coordCMDSnapshotSvcTasks::_coordCMDSnapshotSvcTasks()
   {
   }

   _coordCMDSnapshotSvcTasks::~_coordCMDSnapshotSvcTasks()
   {
   }

   const CHAR* _coordCMDSnapshotSvcTasks::getIntrCMDName()
   {
      return CMD_ADMIN_PREFIX CMD_NAME_SNAPSHOT_SVCTASKS_INTR ;
   }

   const CHAR* _coordCMDSnapshotSvcTasks::getInnerAggrContent()
   {
      return COORD_SNAPSHOTSVCTASKS_INPUT ;
   }

   /*
      _coordCMDSnapshotSvcTasksIntr implement
   */
   COORD_IMPLEMENT_CMD_AUTO_REGISTER( _coordCMDSnapshotSvcTasksIntr,
                                      CMD_NAME_SNAPSHOT_SVCTASKS_INTR,
                                      TRUE ) ;
   _coordCMDSnapshotSvcTasksIntr::_coordCMDSnapshotSvcTasksIntr()
   {
   }

   _coordCMDSnapshotSvcTasksIntr::~_coordCMDSnapshotSvcTasksIntr()
   {
   }

   /*
      _coordCMDSnapshotSequencess implement
   */
   COORD_IMPLEMENT_CMD_AUTO_REGISTER( _coordCMDSnapshotSequences,
                                      CMD_NAME_SNAPSHOT_SEQUENCES,
                                      TRUE ) ;

   _coordCMDSnapshotSequences::_coordCMDSnapshotSequences()
   {
   }

   _coordCMDSnapshotSequences::~_coordCMDSnapshotSequences()
   {
   }

   INT32 _coordCMDSnapshotSequences::_preProcess( rtnQueryOptions &queryOpt,
                                            string & clName,
                                            BSONObj &outSelector )
   {
      BSONObjBuilder builder ;
      clName = GTS_SEQUENCE_COLLECTION_NAME ;
      return SDB_OK ;
   }


}

