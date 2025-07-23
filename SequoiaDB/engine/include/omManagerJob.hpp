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

   Source File Name = omManagerJob.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          08/21/2014  LYB Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef OMMANAGER_JOB_HPP_
#define OMMANAGER_JOB_HPP_
#include "core.hpp"
#include "oss.hpp"
#include "ossUtil.hpp"
#include "rtnBackgroundJob.hpp"
#include "ossLatch.hpp"
#include "omManager.hpp"
#include "omTaskManager.hpp"

namespace engine
{
   class omHostVersion : public SDBObject
   {
   public:
      omHostVersion() ;
      ~omHostVersion() ;

   public:
      void    incVersion( string clusterName ) ;
      void    setPrivilege( string clusterName, BOOLEAN privilege ) ;
      BOOLEAN getPrivilege( string clusterName ) ;
      void    removeVersion( string clusterName ) ;
      void    getVersionMap( map< string, UINT32 > &mapClusterVersion ) ;
      UINT32  getVersion( string clusterName ) ;

   private:
      ossSpinSLatch        _lock ;
      map<string, UINT32>  _mapClusterVersion ;
      map<string, BOOLEAN> _mapClusterPrivilege ;
      typedef map<string, UINT32 >::iterator _MAP_CV_ITER ;
      typedef map<string, UINT32 >::value_type _MAP_CV_VALUETYPE ;
   } ;

   struct omHostContent
   {
      string hostName ;
      string ip ;
      string serviceName ;
      string user ;
      string passwd ;
   } ;

   class omClusterNotifier : public SDBObject
   {
      public:
         omClusterNotifier( pmdEDUCB *cb, omManager *om, string clusterName ) ;
         ~omClusterNotifier() ;

      public:
         INT32            notify( UINT32 newVersion ) ;

      private:
         INT32            _updateNotifier() ;
         INT32            _notifyAgent() ;
         INT32            _addUpdateHostReq( pmdRemoteSession *remoteSession,
                                             const CHAR *localHostName ) ;
         void             _clearSession( pmdRemoteSession *remoteSession ) ;
         void             _getAgentService( string &serviceName ) ;

      private:
         pmdEDUCB                *_cb ;
         omManager               *_om ;
         string                  _clusterName ;
         UINT32                  _version ;

         // store all the host's info
         vector< omHostContent > _vHostTable ;
         // store the agent's info that need to update hostname
         map< string, omHostContent > _mapTargetAgents ;

         typedef map< string, omHostContent >::iterator _MAPAGENT_ITER ;
         typedef map< string, omHostContent >::value_type _MAPAGENT_VALUE ;
   } ;

   /*
      hostNotifier job
   */
   class omHostNotifierJob : public _rtnBaseJob
   {
      public:
         omHostNotifierJob( omManager *om, 
                            omHostVersion *version ) ;
         virtual ~omHostNotifierJob() ;

      public:
         virtual RTN_JOB_TYPE type() const ;
         virtual const CHAR*  name() const ;
         virtual BOOLEAN      muteXOn( const _rtnBaseJob *pOther ) ;
         virtual INT32        doit() ;

      private:
         void                 _checkUpdateCluster( 
                                      map<string, UINT32> &mapClusterVersion ) ;
         void                 _checkDeleteCluster(
                                      map<string, UINT32> &mapClusterVersion ) ;

      private:
         omManager            *_om ;
         omHostVersion        *_shareVersion ;

         map<string, omClusterNotifier*> _mapClusters ;
         typedef map<string, omClusterNotifier*>::iterator _MAP_CLUSTER_ITER ;
         typedef map<string, omClusterNotifier*>::value_type _MAP_CLUSTER_VALUE ;
   } ;
}

#endif  /* OMMANAGER_JOB_HPP_ */


