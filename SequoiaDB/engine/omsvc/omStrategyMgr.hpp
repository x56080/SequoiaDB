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

   Source File Name = omStrategyMgr.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          01/18/2016  Li Jianhua  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef OM_STRATEGY_MGR_HPP_
#define OM_STRATEGY_MGR_HPP_

#include "pmd.hpp"
#include "pmdEDU.hpp"
#include "dmsCB.hpp"
#include "rtnCB.hpp"
#include "ossLatch.hpp"
#include "omStrategyDef.hpp"
#include <string>

namespace engine
{
   class omStrategyMgr : public SDBObject
   {
   public:

      INT32 init( pmdEDUCB *cb ) ;

      INT32 queryTasks( SINT64 &contextID, pmdEDUCB *cb ) ;

      INT32 insertTask( omTaskStrategyInfo &strategyInfo, pmdEDUCB *cb ) ;

      INT32 updateTaskNiceById( INT32 newNice, INT64 _id, pmdEDUCB *cb ) ;

      INT32 addTaskIpsById( const std::set<std::string> &ips,
                            INT64 _id, pmdEDUCB *cb ) ;

      INT32 delTaskIpsById( const std::set<std::string> &ips,
                            INT64 _id, pmdEDUCB *cb ) ;

      INT32 delTaskById( INT64 _id, pmdEDUCB *cb ) ;

      INT32 getTaskStrategy( const std::string &taskName,
                             const std::string &userName,
                             const std::string &IP ) ;

   private:

      INT32 getFieldMaxValue( const CHAR *pFieldName, INT64 &value,
                              INT64 defaultVal, pmdEDUCB * cb ) ;

      INT32 incRecordRuleID( INT64 beginID, pmdEDUCB * cb ) ;

      INT32 decRecordRuleID( INT64 beginID, pmdEDUCB * cb ) ;

      INT64 incRuleID() ;

      INT64 decRuleID() ;

      INT32 checkTaskStrategyInfo( omTaskStrategyInfo &strategyInfo,
                                   pmdEDUCB * cb ) ;

      INT32 getTaskID( const std::string &taskName, INT64 &taskID,
                       pmdEDUCB * cb ) ;

      INT32 getATaskStrategyRecord( bson::BSONObj &recordObj,
                                    const bson::BSONObj &selector,
                                    const bson::BSONObj &matcher,
                                    const bson::BSONObj &orderBy,
                                    pmdEDUCB * cb ) ;

      void rollbackRuleID() ;

      void clearRuleIDIncVal() ;

   public:

      ~omStrategyMgr(){}

      static omStrategyMgr &getInstance() ;

   private:

      omStrategyMgr() ;

      omStrategyMgr( const omStrategyMgr & others ) ;

   private:
      ossSpinXLatch                 m_mutex ;
      INT64                         m_curTaskID ;
      INT64                         m_curRuleID ;
      bson::BSONObj                 m_emptyObj ;
      pmdKRCB *                     m_pKrCB ;
      SDB_DMSCB *                   m_pDmsCB ;
      SDB_RTNCB *                   m_pRtnCB ;
      INT32                         m_ruleIDIncVal ;
   };


#define omStrategyMgrInst           omStrategyMgr::getInstance()
}

#endif