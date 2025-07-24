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

   Source File Name = rtnCoordList.hpp

   Descriptive Name = Runtime Coord Commands

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
#include "rtnCoordList.hpp"
#include "rtnCoordDef.hpp"
#include "catDef.hpp"
#include "authDef.hpp"
#include "msgMessage.hpp"
#include "pmd.hpp"
#include "rtnCB.hpp"
#include "rtnCoordQuery.hpp"

using namespace bson ;

namespace engine
{

   #define RTN_COORD_EMPTY_AGGR_CONTEXT      NULL

   /*
      rtnCoordListTransCurIntr implement
   */
   void rtnCoordListTransCurIntr::_preSet( pmdEDUCB *cb,
                                           rtnCoordCtrlParam &ctrlParam )
   {
      ctrlParam._role[ SDB_ROLE_CATALOG ] = 0 ;
      ctrlParam._emptyFilterSel = NODE_SEL_PRIMARY ;

      ctrlParam._useSpecialNode = TRUE ;
      DpsTransNodeMap *pMap = cb->getTransNodeLst() ;
      if ( pMap )
      {
         DpsTransNodeMap::iterator it = pMap->begin() ;
         while( it != pMap->end() )
         {
            ctrlParam._specialNodes.insert( it->second.value ) ;
            ++it ;
         }
      }
   }

   /*
      rtnCoordListTransIntr implement
   */
   void rtnCoordListTransIntr::_preSet( pmdEDUCB *cb,
                                        rtnCoordCtrlParam &ctrlParam )
   {
      ctrlParam._role[ SDB_ROLE_CATALOG ] = 0 ;
      ctrlParam._emptyFilterSel = NODE_SEL_PRIMARY ;
   }

   /*
      rtnCoordListTransCur implement
   */
   const CHAR* rtnCoordListTransCur::getIntrCMDName()
   {
      return COORD_CMD_LISTTRANSCURINTR ;
   }

   const CHAR* rtnCoordListTransCur::getInnerAggrContent()
   {
      return RTN_COORD_EMPTY_AGGR_CONTEXT ;
   }

   /*
      rtnCoordListTrans implement
   */
   const CHAR* rtnCoordListTrans::getIntrCMDName()
   {
      return COORD_CMD_LISTTRANSINTR ;
   }

   const CHAR* rtnCoordListTrans::getInnerAggrContent()
   {
      return RTN_COORD_EMPTY_AGGR_CONTEXT ;
   }

   /*
      rtnCoordListBackupIntr implement
   */
   void rtnCoordListBackupIntr::_preSet( pmdEDUCB *cb,
                                         rtnCoordCtrlParam &ctrlParam )
   {
      ctrlParam._filterID = FILTER_ID_HINT ;
      ctrlParam._emptyFilterSel = NODE_SEL_ALL ;
      ctrlParam._role[ SDB_ROLE_DATA ] = 1 ;
      ctrlParam._role[ SDB_ROLE_CATALOG ] = 1 ;
   }

   /*
      rtnCoordListBackup implement
   */
   const CHAR* rtnCoordListBackup::getIntrCMDName()
   {
      return COORD_CMD_LISTBACKUPINTR ;
   }

   const CHAR* rtnCoordListBackup::getInnerAggrContent()
   {
      return RTN_COORD_EMPTY_AGGR_CONTEXT ;
   }

   /*
      rtnCoordCMDListGroups implement
   */
   INT32 rtnCoordCMDListGroups::_preProcess( rtnQueryOptions &queryOpt,
                                             string &clName,
                                             BSONObj &outSelector )
   {
      clName = CAT_NODE_INFO_COLLECTION ;
      return SDB_OK ;
   }

   /*
      rtnCoordCMDListCollectionSpace implement
   */
   INT32 rtnCoordCMDListCollectionSpace::_preProcess( rtnQueryOptions &queryOpt,
                                                      string &clName,
                                                      BSONObj &outSelector )
   {
      BSONObjBuilder builder ;
      clName = CAT_COLLECTION_SPACE_COLLECTION ;
      builder.appendNull( CAT_COLLECTION_SPACE_NAME ) ;
      outSelector = queryOpt._selector ;
      queryOpt._selector = builder.obj() ;
      return SDB_OK ;
   }

   /*
      rtnCoordCMDListCollectionSpaceIntr implement
   */
   INT32 rtnCoordCMDListCollectionSpaceIntr::_preProcess( rtnQueryOptions &queryOpt,
                                                          string &clName,
                                                          BSONObj &outSelector )
   {
      clName = CAT_COLLECTION_SPACE_COLLECTION ;
      return SDB_OK ;
   }

   /*
      rtnCoordCMDListCollection implement
   */
   INT32 rtnCoordCMDListCollection::_preProcess( rtnQueryOptions &queryOpt,
                                                 string & clName,
                                                 BSONObj &outSelector )
   {
      BSONObjBuilder builder ;
      clName = CAT_COLLECTION_INFO_COLLECTION ;
      builder.appendNull( CAT_COLLECTION_NAME ) ;
      outSelector = queryOpt._selector ;
      queryOpt._selector = builder.obj() ;
      return SDB_OK ;
   }

   /*
      rtnCoordCMDListCollectionIntr implement
   */
   INT32 rtnCoordCMDListCollectionIntr::_preProcess( rtnQueryOptions &queryOpt,
                                                     string & clName,
                                                     BSONObj &outSelector )
   {
      clName = CAT_COLLECTION_INFO_COLLECTION ;
      return SDB_OK ;
   }

   /*
      rtnCoordCMDListContexts implement
   */
   const CHAR* rtnCoordCMDListContexts::getIntrCMDName()
   {
      return COORD_CMD_LISTCTXINTR ;
   }

   const CHAR* rtnCoordCMDListContexts::getInnerAggrContent()
   {
      return RTN_COORD_EMPTY_AGGR_CONTEXT ;
   }

   /*
      rtnCoordCMDListContextsCur implement
   */
   const CHAR* rtnCoordCMDListContextsCur::getIntrCMDName()
   {
      return COORD_CMD_LISTCTXCURINTR ;
   }

   const CHAR* rtnCoordCMDListContextsCur::getInnerAggrContent()
   {
      return RTN_COORD_EMPTY_AGGR_CONTEXT ;
   }

   /*
      rtnCoordCMDListSessions implement
   */
   const CHAR* rtnCoordCMDListSessions::getIntrCMDName()
   {
      return COORD_CMD_LISTSESSINTR ;
   }

   const CHAR* rtnCoordCMDListSessions::getInnerAggrContent()
   {
      return RTN_COORD_EMPTY_AGGR_CONTEXT ;
   }

   /*
      rtnCoordCMDListSessionsCur implement
   */
   const CHAR* rtnCoordCMDListSessionsCur::getIntrCMDName()
   {
      return COORD_CMD_LISTSESSCURINTR ;
   }

   const CHAR* rtnCoordCMDListSessionsCur::getInnerAggrContent()
   {
      return RTN_COORD_EMPTY_AGGR_CONTEXT ;
   }

   /*
      rtnCoordCMDListUser implement
   */
   INT32 rtnCoordCMDListUser::_preProcess( rtnQueryOptions &queryOpt,
                                           string & clName,
                                           BSONObj &outSelector )
   {
      BSONObjBuilder builder ;
      clName = AUTH_USR_COLLECTION ;
      if ( queryOpt._selector.isEmpty() )
      {
         builder.appendNull( FIELD_NAME_USER ) ;
      }
      queryOpt._selector = builder.obj() ;
      return SDB_OK ;
   }

   /*
      rtnCoordCmdListTask implement
   */
   INT32 rtnCoordCmdListTask::_preProcess( rtnQueryOptions &queryOpt,
                                           string &clName,
                                           BSONObj &outSelector )
   {
      clName = CAT_TASK_INFO_COLLECTION ;
      return SDB_OK ;
   }

   /*
      rtnCoordCMDListProcedures implement
   */
   INT32 rtnCoordCMDListProcedures::_preProcess( rtnQueryOptions &queryOpt,
                                                 string &clName,
                                                 BSONObj &outSelector )
   {
      clName = CAT_PROCEDURES_COLLECTION ;
      return SDB_OK ;
   }

   /*
      rtnCoordCMDListDomains implement
   */
   INT32 rtnCoordCMDListDomains::_preProcess( rtnQueryOptions &queryOpt,
                                              string &clName,
                                              BSONObj &outSelector )
   {
      clName = CAT_DOMAIN_COLLECTION ;
      return SDB_OK ;
   }

   /*
      rtnCoordCMDListCSInDomain implement
   */
   INT32 rtnCoordCMDListCSInDomain::_preProcess( rtnQueryOptions &queryOpt,
                                                 string &clName,
                                                 BSONObj &outSelector )
   {
      clName = CAT_COLLECTION_SPACE_COLLECTION ;
      return SDB_OK ;
   }

   /*
      rtnCoordCMDListCLInDomain implement
   */
   INT32 rtnCoordCMDListCLInDomain::execute( MsgHeader *pMsg,
                                             pmdEDUCB *cb,
                                             INT64 &contextID,
                                             rtnContextBuf *buf )
   {
      INT32 rc = SDB_OK ;
      BSONObj conObj ;
      BSONObj dummy ;
      CHAR *query = NULL ;
      BSONElement domain ;
      rtnQueryOptions queryOptions ;
      vector<BSONObj> replyFromCata ;
      contextID = -1 ;

      rc = msgExtractQuery( (CHAR*)pMsg, NULL, NULL,
                            NULL, NULL, &query,
                            NULL, NULL, NULL );
      if ( rc != SDB_OK )
      {
         PD_LOG ( PDERROR, "failed to parse query request(rc=%d)", rc ) ;
         goto error ;
      }

      try
      {
         conObj = BSONObj( query ) ;
      }
      catch ( std::exception &e )
      {
         PD_LOG( PDERROR, "unexpected err happened: %s", e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      domain = conObj.getField( FIELD_NAME_DOMAIN ) ;
      if ( String != domain.type() )
      {
         PD_LOG( PDERROR, "invalid domain field in object:%s",
                  conObj.toString().c_str() ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      queryOptions._query = BSON( CAT_DOMAIN_NAME << domain.valuestr() ) ;
      queryOptions._fullName = CAT_COLLECTION_SPACE_COLLECTION ;

      rc = queryOnCataAndPushToVec( queryOptions, cb, replyFromCata,
                                    buf ) ; 
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to execute query on catalog:%d", rc ) ;
         goto error ;
      }

      /// build result
      rc = _rebuildListResult( replyFromCata, cb, contextID ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to rebuild list result:%d", rc ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      if ( contextID >= 0 )
      {
         pmdGetKRCB()->getRTNCB()->contextDelete( contextID, cb ) ;
         contextID = -1 ;
      }
      goto done ;
   }

   INT32 rtnCoordCMDListCLInDomain::_rebuildListResult(
                                    const vector<BSONObj> &infoFromCata,
                                    pmdEDUCB *cb,                       
                                    SINT64 &contextID )
   {
      INT32 rc = SDB_OK ;
      rtnContext *context = NULL ;
      SDB_RTNCB *rtnCB = sdbGetRTNCB() ;

      rc = rtnCB->contextNew( RTN_CONTEXT_DUMP,
                              &context,
                              contextID,
                              cb ) ;
      if  ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to create new context:%d", rc ) ;
         goto error ;
      }

      rc = (( rtnContextDump * )context)->open( BSONObj(), BSONObj() ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to open context:%d", rc ) ;
         goto error ;
      }

      for ( vector<BSONObj>::const_iterator itr = infoFromCata.begin();
            itr != infoFromCata.end();
            itr++ )
      {
         BSONElement cl ;
         BSONElement cs = itr->getField( FIELD_NAME_NAME ) ;
         if ( String != cs.type() )
         {
            PD_LOG( PDERROR, "invalid collection space info:%s",
                    itr->toString().c_str() ) ;
            rc = SDB_SYS ;
            goto error ;
         }

         cl = itr->getField( FIELD_NAME_COLLECTION ) ;
         if ( Array != cl.type() )
         {
            PD_LOG( PDERROR, "invalid collection space info:%s",
                    itr->toString().c_str() ) ;
            rc = SDB_SYS ;
            goto error ;
         }
         else
         {
            BSONObjIterator clItr( cl.embeddedObject() ) ;
            while ( clItr.more() )
            {
               stringstream ss ;
               BSONElement clName ;
               BSONElement oneCl = clItr.next() ;
               if ( Object != oneCl.type() )
               {
                  PD_LOG( PDERROR, "invalid collection space info:%s",
                          itr->toString().c_str() ) ;
                  rc = SDB_SYS ;
                  goto error ;
               }

               clName = oneCl.embeddedObject().getField( FIELD_NAME_NAME ) ;
               if ( String != clName.type() )
               {
                  PD_LOG( PDERROR, "invalid collection space info: %s",
                          itr->toString().c_str() ) ;
                  rc = SDB_SYS ;
                  goto error ;
               }

               ss << cs.valuestr() << "." << clName.valuestr() ;
               context->append( BSON( FIELD_NAME_NAME << ss.str() ) ) ;
            }
         }
      }

   done:
      return rc ;
   error:
      if ( -1 != contextID )
      {
         rtnCB->contextDelete ( contextID, cb ) ;
         contextID = -1 ;
      }
      goto done ;
   }

   /*
      rtnCoordCMDListLobs implement
   */
   INT32 rtnCoordCMDListLobs::execute( MsgHeader *pMsg,
                                       pmdEDUCB *cb,
                                       INT64 &contextID,
                                       rtnContextBuf *buf )
   {
      INT32 rc = SDB_OK ;

      CHAR *pQuery = NULL ;
      BSONObj query ;

      rtnContextCoord *context = NULL ;
      rtnCoordQuery queryOpr( isReadonly() ) ;
      rtnQueryConf queryConf ;
      rtnSendOptions sendOpt ;

      SDB_RTNCB *pRtncb = pmdGetKRCB()->getRTNCB() ;
      CoordCB *pCoordcb = pmdGetKRCB()->getCoordCB() ;
      netMultiRouteAgent *pRouteAgent = pCoordcb->getRouteAgent() ;

      contextID = -1 ;

      rc = msgExtractQuery( (CHAR*)pMsg, NULL, NULL,
                            NULL, NULL, &pQuery,
                            NULL, NULL, NULL ) ;

      PD_RC_CHECK( rc, PDERROR, "Snapshot failed, failed to parse query "
                   "request(rc=%d)", rc ) ;

      try
      {
         query = BSONObj( pQuery ) ;
         BSONElement ele = query.getField( FIELD_NAME_COLLECTION ) ;
         if ( String != ele.type() )
         {
            PD_LOG( PDERROR, "invalid obj of list lob:%s",
                    query.toString( FALSE, TRUE ).c_str() ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }
         queryConf._realCLName = ele.valuestr() ;
      }
      catch ( std::exception &e )
      {
         PD_LOG( PDERROR, "unexpected err happened:%s", e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      queryConf._openEmptyContext = TRUE ;
      queryConf._allCataGroups = TRUE ;
      rc = queryOpr.queryOrDoOnCL( pMsg, pRouteAgent, cb, &context,
                                   sendOpt, &queryConf, buf ) ;
      PD_RC_CHECK( rc, PDERROR, "List lobs[%s] on groups failed, rc: %d",
                   queryConf._realCLName.c_str(), rc ) ;

      // set context id
      contextID = context->contextID() ;

   done:
      return rc ;
   error:
      if ( context )
      {
         pRtncb->contextDelete( context->contextID(), cb ) ;
      }
      goto done ;
   }

}

