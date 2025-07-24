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
#include "ossLatch.hpp"
#include "omStrategyDef.hpp"
#include "omSdbAdaptor.hpp"
#include <string>

using namespace bson ;
using namespace std ;

namespace engine
{

   /*
      _omStrategyChangeKey define
   */
   struct _omStrategyChangeKey
   {
      string      _clsName ;
      string      _bizName ;

      _omStrategyChangeKey( const string &clsName,
                            const string &bizName )
      {
         _clsName = clsName ;
         _bizName = bizName ;
      }
      _omStrategyChangeKey()
      {
      }

      bool operator<( const _omStrategyChangeKey &right ) const
      {
         INT32 ret = _clsName.compare( right._clsName ) ;
         if ( ret < 0 )
         {
            return true ;
         }
         else if ( 0 == ret )
         {
            ret = _bizName.compare( right._bizName ) ;
            if ( ret < 0 )
            {
               return true ;
            }
         }
         return false ;
      }
   } ;
   typedef _omStrategyChangeKey omStrategyChangeKey ;

   typedef map< omStrategyChangeKey, INT64 >       MAP_BIZ_TIME ;

   /*
      _omStrategyMgr define
   */
   class _omStrategyMgr : public SDBObject
   {
      public:
         _omStrategyMgr() ;
         ~_omStrategyMgr() ;

         omSdbAdaptor* getSdbAdaptor() ;

         INT32 init( pmdEDUCB *cb ) ;
         void  fini() ;

         INT32 queryTasks( const string &clsName,
                           const string &bizName,
                           SINT64 &contextID,
                           pmdEDUCB *cb ) ;

         INT32 insertTask( omTaskInfo &taskInfo,
                           pmdEDUCB *cb ) ;

         INT32 updateTaskStatus( const string &clsName,
                                 const string &bizName,
                                 const string &taskName,
                                 INT32 status,
                                 pmdEDUCB *cb ) ;

         INT32 delTask( const string &clsName,
                        const string &bizName,
                        const string &taskName,
                        pmdEDUCB *cb ) ;

         INT32 queryStrategys( const string &clsName,
                               const string &bizName,
                               const string &taskName,
                               INT64 &contextID,
                               pmdEDUCB *cb ) ;

         INT32 insertStrategy( omTaskStrategyInfo &strategyInfo,
                               pmdEDUCB *cb ) ;

         INT32 updateStrategyNiceById( INT32 newNice,
                                       INT64 ruleID,
                                       const string &clsName,
                                       const string &bizName,
                                       pmdEDUCB *cb ) ;

         INT32 addStrategyIpsById( const set< string > &ips,
                                   INT64 ruleID,
                                   const string &clsName,
                                   const string &bizName,
                                   pmdEDUCB *cb ) ;

         INT32 delStrategyIpsById( const set< string > &ips,
                                   INT64 ruleID,
                                   const string &clsName,
                                   const string &bizName,
                                   pmdEDUCB *cb ) ;

         INT32 delAllStrategyIpsById( INT64 ruleID,
                                      const string &clsName,
                                      const string &bizName,
                                      pmdEDUCB *cb ) ;

         INT32 delStrategyById( INT64 ruleID,
                                const string &clsName,
                                const string &bizName,
                                pmdEDUCB *cb ) ;

         INT32 updateStrategyStatusById( INT32 status,
                                         INT64 ruleID,
                                         const string &clsName,
                                         const string &bizName,
                                         pmdEDUCB *cb ) ;

         INT32 updateStrategySortIDById( INT64 sortID,
                                         INT64 ruleID,
                                         const string &clsName,
                                         const string &bizName,
                                         pmdEDUCB *cb ) ;

         INT32 updateStrategyUserById( const CHAR *userName,
                                       INT64 ruleID,
                                       const string &clsName,
                                       const string &bizName,
                                       pmdEDUCB *cb ) ;

         void  flushStrategy( const string &clsName,
                              const string &bizName,
                              pmdEDUCB *cb ) ;

         INT32 getMetaRecord( const string &clsName,
                              const string &bizName,
                              BSONObj &obj,
                              pmdEDUCB *cb ) ;

         INT32 getVersion( const string &clsName,
                           const string &bizName,
                           INT32 &version,
                           pmdEDUCB *cb ) ;

         UINT32 popTimeoutBusiness( INT64 interval,
                                    set<omStrategyChangeKey> &setBiz ) ;

      protected:

         INT32 getFieldMaxValue( const CHAR *pCollection,
                                 const CHAR *pFieldName,
                                 INT64 &value,
                                 INT64 defaultVal,
                                 pmdEDUCB *cb,
                                 const BSONObj &matcher ) ;

         INT32 checkTaskStrategyInfo( omTaskStrategyInfo &strategyInfo,
                                      pmdEDUCB * cb ) ;

         INT32 getTaskID( const string &clsName,
                          const string &bizName,
                          const string &taskName,
                          INT64 &taskID,
                          pmdEDUCB *cb ) ;

         INT32 getSortID( const string &clsName,
                          const string &bizName,
                          INT64 ruleID,
                          INT64 &taskID,
                          INT64 &sortID,
                          pmdEDUCB *cb ) ;

         INT32 getARecord( const CHAR *pCollection,
                           const BSONObj &selector,
                           const BSONObj &matcher,
                           const BSONObj &orderBy,
                           pmdEDUCB *cb,
                           BSONObj &recordObj ) ;

         INT32 allocateTaskID( const string &clsName,
                               const string &bizName,
                               pmdEDUCB *cb,
                               INT64 &taskID ) ;

         INT32 allocateRuleID( const string &clsName,
                               const string &bizName,
                               pmdEDUCB *cb,
                               INT64 &ruleID ) ;

      private:
         INT32 _updateMeta( const string &clsName,
                            const string &bizName,
                            pmdEDUCB *cb ) ;

         void  _add2ChangeMap( const string &clsName,
                               const string &bizName,
                               INT64 timeout = 0 ) ;

      private:
         ossSpinXLatch                 m_mutex ;

         omSdbAdaptor                  *_pSdbAdapter ;
         MAP_BIZ_TIME                  _mapBizTimeoutInfo ;
   } ;
   typedef _omStrategyMgr omStrategyMgr ;

   omStrategyMgr* omGetStrategyMgr() ;

}

#endif // OM_STRATEGY_MGR_HPP_

