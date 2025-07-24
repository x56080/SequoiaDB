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

   Source File Name = omStrategyMgr.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          01/18/2016  Li Jianhua  Initial Draft

   Last Changed =

*******************************************************************************/
#include "omStrategyMgr.hpp"
#include "omDef.hpp"
#include "catCommon.hpp"
#include "rtn.hpp"
#include "../util/fromjson.hpp"

using namespace bson ;
namespace engine
{

   omStrategyMgr::omStrategyMgr()
   : m_curTaskID(1),
     m_curRuleID(1)
   {
      m_pKrCB = NULL ;
      m_pDmsCB = NULL ;
      m_pRtnCB = NULL ;
   }

   omStrategyMgr::omStrategyMgr( const omStrategyMgr & others )
   {
      m_curTaskID = others.m_curTaskID ;
      m_curRuleID = others.m_curRuleID ;
      m_pKrCB = others.m_pKrCB ;
      m_pDmsCB = others.m_pDmsCB ;
      m_pRtnCB = others.m_pRtnCB ;
   }

   omStrategyMgr & omStrategyMgr::getInstance()
   {
      static omStrategyMgr _omStrategyMgr ;
      return _omStrategyMgr ;
   }

   INT32 omStrategyMgr::init( pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      BSONObj indexDef ;
      INT64 valTmp ;

      m_pKrCB  = pmdGetKRCB() ;
      m_pDmsCB = m_pKrCB->getDMSCB() ;
      m_pRtnCB = m_pKrCB->getRTNCB() ;

      rc = catTestAndCreateCL( OM_CS_STRATEGY_CL_BUSINESS_TASK_PRO,
                               cb, m_pDmsCB, NULL, TRUE ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to create collection(name:%s, rc:%d)",
                   OM_CS_STRATEGY_CL_BUSINESS_TASK_PRO, rc ) ;

      rc = fromjson ( OM_CS_STRATEGY_CL_BUSINESSTASKPROIDX1, indexDef ) ;
      PD_RC_CHECK ( rc, PDERROR, "Failed to build index object, rc = %d",
                    rc ) ;
      rc = catTestAndCreateIndex( OM_CS_STRATEGY_CL_BUSINESS_TASK_PRO,
                                  indexDef, cb, m_pDmsCB, NULL, TRUE ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to create index(cl:%s, index:%s, rc:%d)",
                   OM_CS_STRATEGY_CL_BUSINESS_TASK_PRO, OM_CS_STRATEGY_CL_BUSINESSTASKPROIDX1,
                   rc ) ;

      rc = getFieldMaxValue( OM_REST_FIELD_TASK_ID, valTmp, 0, cb ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to get max task-id(rc=%d)!", rc ) ;
      m_curTaskID = valTmp + 1 ;

      rc = getFieldMaxValue( OM_REST_FIELD_RULE_ID, valTmp, 0, cb ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to get max _id(rc=%d)!", rc ) ;
      m_curRuleID = valTmp + 1 ;
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 omStrategyMgr::checkTaskStrategyInfo( omTaskStrategyInfo &strategyInfo,
                                               pmdEDUCB * cb )
   {
      INT32 rc = SDB_OK ;
      INT64 taskID = 0 ;
      PD_CHECK( !strategyInfo.taskName.empty(), SDB_INVALIDARG, error, PDERROR,
                "TaskName can't be empty!" ) ;
      rc = getTaskID( strategyInfo.taskName, taskID, cb ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to get taskID(rc=%d)!", rc ) ;
      strategyInfo.setTaskID( taskID ) ;

      if ( strategyInfo._id >= m_curRuleID
         || strategyInfo._id <= 0 )
      {
         strategyInfo._id = incRuleID() ;
      }

      if ( strategyInfo.nice < OM_TASK_STRATEGY_NICE_MIN )
      {
         strategyInfo.nice = OM_TASK_STRATEGY_NICE_MIN ;
      }
      else if ( strategyInfo.nice > OM_TASK_STRATEGY_NICE_MAX )
      {
         strategyInfo.nice = OM_TASK_STRATEGY_NICE_MAX ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 omStrategyMgr::incRecordRuleID( INT64 beginID, pmdEDUCB * cb )
   {
      INT32 rc = SDB_OK ;
      SDB_ASSERT( beginID < m_curRuleID, "RuleID error!" ) ;
      if ( m_curRuleID - 1 == beginID )
      {
         // it is the last one and not need to inc
         goto done ;
      }
      {
      BSONObj rule ;
      BSONObj matcher ;
      rule = BSON("$inc" << BSON( OM_REST_FIELD_RULE_ID << 1) ) ;
      matcher = BSON( OM_REST_FIELD_RULE_ID << BSON( "$gte" << beginID )) ;
      rc = rtnUpdate( OM_CS_STRATEGY_CL_BUSINESS_TASK_PRO,
                      matcher, rule, m_emptyObj, 0, cb ) ;
      PD_RC_CHECK( rc, PDERROR, "Update failed(rc=%d)!", rc ) ;
      }
      incRuleID();
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 omStrategyMgr::decRecordRuleID( INT64 beginID, pmdEDUCB * cb )
   {
      INT32 rc = SDB_OK ;
      SDB_ASSERT( beginID < m_curRuleID, "RuleID error!" ) ;
      if ( m_curRuleID - 1 == beginID )
      {
         // it is the last one and not need to dec
         goto done ;
      }
      {
      BSONObj rule ;
      BSONObj matcher ;
      rule = BSON("$inc" << BSON( OM_REST_FIELD_RULE_ID << -1) ) ;
      matcher = BSON( OM_REST_FIELD_RULE_ID << BSON( "$gt" << beginID )) ;
      rc = rtnUpdate( OM_CS_STRATEGY_CL_BUSINESS_TASK_PRO,
                      matcher, rule, m_emptyObj, 0, cb ) ;
      PD_RC_CHECK( rc, PDERROR, "Update failed(rc=%d)!", rc ) ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 omStrategyMgr::getFieldMaxValue( const CHAR *pFieldName, INT64 &value,
                                          INT64 defaultVal, pmdEDUCB * cb )
   {
      INT32 rc = SDB_OK ;
      BSONObj recordObj ;
      BSONObj orderBy = BSON( pFieldName << -1 ) ;
      BSONElement fieldTmp ;
      rc = getATaskStrategyRecord( recordObj, m_emptyObj,
                                   m_emptyObj, orderBy, cb ) ;
      if ( rc != SDB_OK )
      {
         if ( SDB_DMS_EOC == rc )
         {
            value = defaultVal ;
            rc = SDB_OK ;
            goto done ;
         }
         PD_LOG( PDERROR, "Failed to get task-strategy record(rc = %d)!", rc ) ;
         goto error ;
      }

      fieldTmp = recordObj.getField( pFieldName ) ;
      if ( fieldTmp.eoo() )
      {
         value = defaultVal ;
         goto done ;
      }
      PD_CHECK( fieldTmp.isNumber(), SDB_SYS, error, PDERROR,
                "Type of the field(%s) is not number!", pFieldName ) ;

      value = fieldTmp.numberLong() ;
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 omStrategyMgr::getTaskID( const std::string &taskName, INT64 &taskID,
                                   pmdEDUCB * cb )
   {
      INT32 rc = SDB_OK ;
      BSONObj recordObj ;
      BSONObj matcher = BSON( OM_REST_FIELD_TASK_NAME << taskName ) ;
      BSONElement fieldTmp ;
      rc = getATaskStrategyRecord( recordObj, m_emptyObj,
                                   matcher, m_emptyObj, cb ) ;
      if ( rc != SDB_OK )
      {
         if ( SDB_DMS_EOC == rc )
         {
            taskID = m_curTaskID++ ;
            rc = SDB_OK ;
            goto done ;
         }
         PD_LOG( PDERROR, "Failed to get task-strategy record(rc = %d)!", rc ) ;
         goto error ;
      }

      fieldTmp = recordObj.getField( OM_REST_FIELD_TASK_ID ) ;
      PD_CHECK( fieldTmp.isNumber(), SDB_SYS, error, PDERROR,
                "Type of the field(%s) is not number!",
                OM_REST_FIELD_TASK_NAME ) ;

      taskID = fieldTmp.numberLong() ;
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 omStrategyMgr::getATaskStrategyRecord( bson::BSONObj &recordObj,
                                                const bson::BSONObj &selector,
                                                const bson::BSONObj &matcher,
                                                const bson::BSONObj &orderBy,
                                                pmdEDUCB * cb )
   {
      INT32 rc = SDB_OK ;
      SINT64 contextID = -1 ;
      rtnContextBuf buffObj ;
      rc = rtnQuery( OM_CS_STRATEGY_CL_BUSINESS_TASK_PRO,
                     selector, matcher, orderBy, m_emptyObj,
                     0, cb, 0, 1, m_pDmsCB, m_pRtnCB, contextID ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Query failed(rc=%d)!", rc ) ;
      rc = rtnGetMore( contextID, 1, buffObj, cb, m_pRtnCB ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Getmore failed(rc=%d)!", rc ) ;
      rc = buffObj.nextObj( recordObj ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to get the record(rc=%d)!", rc ) ;
   done:
      if ( rc != SDB_DMS_EOC && contextID != -1 )
      {
         rtnKillContexts(1, &contextID, cb, m_pRtnCB ) ;
      }
      return rc ;
   error:
      goto done ;
   }

   void omStrategyMgr::rollbackRuleID()
   {
      m_curRuleID -= m_ruleIDIncVal ;
      clearRuleIDIncVal() ;
   }

   void omStrategyMgr::clearRuleIDIncVal()
   {
      m_ruleIDIncVal = 0 ;
   }

   INT32 omStrategyMgr::queryTasks( SINT64 & contextID, pmdEDUCB * cb )
   {
      INT32 rc = SDB_OK ;
      static BSONObj orderBy = BSON( OM_REST_FIELD_RULE_ID << 1 ) ;
      rc = rtnQuery( OM_CS_STRATEGY_CL_BUSINESS_TASK_PRO,
                     m_emptyObj, m_emptyObj, orderBy, m_emptyObj,
                     0, cb, 0, -1, m_pDmsCB, m_pRtnCB, contextID ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Query the cl(%s) failed!",
                   OM_CS_STRATEGY_CL_BUSINESS_TASK_PRO ) ;
   done:
      return rc ;
   error:
      goto done ;
   }

   INT64 omStrategyMgr::incRuleID()
   {
      m_ruleIDIncVal++ ;
      return m_curRuleID++ ;
   }

   INT64 omStrategyMgr::decRuleID()
   {
      m_ruleIDIncVal-- ;
      return m_curRuleID-- ;
   }

   INT32 omStrategyMgr::insertTask( omTaskStrategyInfo &strategyInfo, pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      BSONObj obj ;
      {
      ossScopedLock lock( &m_mutex ) ;
      clearRuleIDIncVal() ;

      rc = checkTaskStrategyInfo( strategyInfo, cb ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Check strategy info failed(rc=%d)!",
                   rc ) ;

      rc = incRecordRuleID( strategyInfo._id, cb ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to inc ruleID(rc=%d)!", rc ) ;
      
      rc = strategyInfo.toBSON( obj ) ;
      PD_CHECK( SDB_OK ==rc, rc, rollback, PDERROR,
                "Failed to generate the bson-obj(rc=%d)!",
                rc ) ;

      rc = rtnInsert( OM_CS_STRATEGY_CL_BUSINESS_TASK_PRO,
                      obj, 1, 0, cb );
      PD_CHECK( SDB_OK ==rc, rc, rollback, PDERROR,
                "Insert failed(rc=%d)!", rc ) ;
      }
   done:
      return rc ;
   rollback:
      decRecordRuleID( strategyInfo._id, cb ) ;
   error:
      rollbackRuleID() ;
      goto done ;
   }

   INT32 omStrategyMgr::updateTaskNiceById( INT32 newNice, INT64 _id, pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;

      if ( newNice < OM_TASK_STRATEGY_NICE_MIN )
      {
         newNice = OM_TASK_STRATEGY_NICE_MIN ;
      }
      else if ( newNice > OM_TASK_STRATEGY_NICE_MAX )
      {
         newNice = OM_TASK_STRATEGY_NICE_MAX ;
      }

      BSONObj rule = BSON( "$set" << BSON( OM_REST_FIELD_NICE << newNice )) ;
      BSONObj matcher = BSON( OM_REST_FIELD_RULE_ID << _id ) ;
      ossScopedLock lock( &m_mutex ) ;
      rc = rtnUpdate( OM_CS_STRATEGY_CL_BUSINESS_TASK_PRO,
                      matcher, rule, m_emptyObj, 0, cb ) ;
      PD_RC_CHECK( rc, PDERROR, "Update failed(rc=%d)!", rc ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 omStrategyMgr::addTaskIpsById( const std::set<std::string> &ips,
                                        INT64 _id, pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      if ( ips.size() == 0 )
      {
         goto done ;
      }
      {
      BSONArrayBuilder arrBuilder ;
      std::set<std::string>::iterator iter = ips.begin() ;
      while( iter != ips.end() )
      {
         arrBuilder.append( *iter ) ;
         ++iter ;
      }
      BSONObj rule = BSON( "$addtoset"
                           << BSON( OM_REST_FIELD_IPS << arrBuilder.arr() ) ) ;
      BSONObj matcher = BSON( OM_REST_FIELD_RULE_ID << _id ) ;
      ossScopedLock lock( &m_mutex ) ;
      rc = rtnUpdate( OM_CS_STRATEGY_CL_BUSINESS_TASK_PRO,
                      matcher, rule, m_emptyObj, 0, cb ) ;
      PD_RC_CHECK( rc, PDERROR, "Update failed(rc=%d)!", rc ) ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 omStrategyMgr::delTaskIpsById( const std::set<std::string> &ips,
                                        INT64 _id, pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      if ( ips.size() == 0 )
      {
         goto done ;
      }
      {
      BSONArrayBuilder arrBuilder ;
      std::set<std::string>::iterator iter = ips.begin() ;
      while( iter != ips.end() )
      {
         arrBuilder.append( *iter ) ;
         ++iter ;
      }
      BSONObj rule = BSON( "$pull_all"
                           << BSON( OM_REST_FIELD_IPS << arrBuilder.arr() ) ) ;
      BSONObj matcher = BSON( OM_REST_FIELD_RULE_ID << _id ) ;
      ossScopedLock lock( &m_mutex ) ;
      rc = rtnUpdate( OM_CS_STRATEGY_CL_BUSINESS_TASK_PRO,
                      matcher, rule, m_emptyObj, 0, cb ) ;
      PD_RC_CHECK( rc, PDERROR, "Update failed(rc=%d)!", rc ) ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 omStrategyMgr::delTaskById( INT64 _id, pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      ossScopedLock lock( &m_mutex ) ;
      clearRuleIDIncVal() ;
      if ( _id >= m_curRuleID || _id <= 0 )
      {
         goto done ;
      }
      {
      BSONObj deletor = BSON( OM_REST_FIELD_RULE_ID << _id ) ;
      rc = rtnDelete( OM_CS_STRATEGY_CL_BUSINESS_TASK_PRO, deletor,
                      m_emptyObj, 0, cb ) ;
      PD_RC_CHECK( rc, PDERROR, "Delete failed(rc=%d)!", rc ) ;

      rc = decRecordRuleID( _id, cb ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to dec ruleID(rc=%d)!", rc ) ;
      }
      decRuleID() ;
   done:
      return rc ;
   error:
      rollbackRuleID() ;
      goto done ;
   }

   INT32 omStrategyMgr::getTaskStrategy( const std::string &taskName,
                                         const std::string &userName,
                                         const std::string &IP )
   {
      INT32 rc = SDB_OK ;
      SDB_ASSERT( FALSE, "TODO!" ) ;
      // TODO: match: taskname, username, ip
      // TODO: match: taskName, username
      // TODO: match: taskName
      // TODO: default
   done:
      return rc ;
   error:
      goto done ;
   }
}
