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

   Source File Name = pmdEnv.hpp

   Descriptive Name = Process MoDel Main

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains main function for SequoiaDB,
   and all other process-initialization code.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          22/04/2014  XJH Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef PMDENV_HPP_
#define PMDENV_HPP_

#include "utilCommon.hpp"
#include "ossAtomic.hpp"

using namespace bson ;

namespace engine
{

   /*
      When recieve quit event or signal, will call
   */
   typedef  void (*PMD_ON_QUIT_FUNC)() ;

   /*
      pmd system info define
   */
   typedef struct _pmdSysInfo
   {
      SDB_ROLE                      _dbrole ;
      MsgRouteID                    _nodeID ;
      ossAtomic32                   _isPrimary ;
      SDB_TYPE                      _dbType ;
      UINT64                        _startTime ;
      UINT16                        _localPort ;

      BOOLEAN                       _quitFlag ;
      PMD_ON_QUIT_FUNC              _pQuitFunc ;

      /// loop updated by pmdSyncClockEntryPoint
      volatile UINT64               _tick ;

      /// loop updated by clsReplicaSet
      volatile UINT64               _validationTick ;

      /// global id
      ossAtomic64                   _globalID ;

      _pmdSysInfo()
      :_isPrimary( 0 ), _globalID( 1 )
      {
         _dbrole        = SDB_ROLE_STANDALONE ;
         _nodeID.value  = MSG_INVALID_ROUTEID ;
         _quitFlag      = FALSE ;
         _dbType        = SDB_TYPE_DB ;
         _pQuitFunc     = NULL ;
         _startTime     = time( NULL ) ;
         _localPort     = 0 ;
         _tick          = 0 ;
         _validationTick = 0 ;
      }
   } pmdSysInfo ;

   SDB_ROLE       pmdGetDBRole() ;
   void           pmdSetDBRole( SDB_ROLE role ) ;
   SDB_TYPE       pmdGetDBType() ;
   void           pmdSetDBType( SDB_TYPE type ) ;
   MsgRouteID     pmdGetNodeID() ;
   void           pmdSetNodeID( MsgRouteID id ) ;
   BOOLEAN        pmdIsPrimary() ;
   void           pmdSetPrimary( BOOLEAN primary ) ;

   UINT64         pmdGetStartTime() ;

   void           pmdSetLocalPort( UINT16 port ) ;
   UINT16         pmdGetLocalPort() ;

   void           pmdSetQuit() ;
   BOOLEAN        pmdIsQuitApp() ;

   void           pmdUpdateDBTick() ;

   UINT64         pmdGetDBTick() ;

   UINT64         pmdGetTickSpanTime( UINT64 lastTick ) ;

   UINT64         pmdDBTickSpan2Time( UINT64 tickSpan ) ;

   void           pmdUpdateValidationTick() ;

   UINT64         pmdGetValidationTick() ;

   void           pmdGetTicks( UINT64 &tick,
                               UINT64 &validationTick ) ;

   BOOLEAN        pmdDBIsAbnormal() ;

   UINT64         pmdAcquireGlobalID() ;

   pmdSysInfo*    pmdGetSysInfo () ;

   /*
      pmd trap functions
   */

   INT32    pmdEnableSignalEvent( const CHAR *filepath,
                                  PMD_ON_QUIT_FUNC pFunc,
                                  INT32 *pDelSig = NULL ) ;

   INT32&   pmdGetSigNum() ;

   /*
      Env define
   */
   #define  PMD_SIGNUM                 pmdGetSigNum()

}

#endif //PMDENV_HPP_

