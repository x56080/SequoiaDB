/*******************************************************************************


   Copyright (C) 2011-2018 SequoiaDB Ltd.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

   http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

   Source File Name = pmdProc.cpp

   Descriptive Name = pmdProc

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains main function for sdbcm,
   which is used to do cluster managing.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          12/26/2013  JHL  Initial Draft

   Last Changed =

*******************************************************************************/
#include "pmdProc.hpp"
#include "ossEDU.hpp"
#include "pd.hpp"

#if defined (_LINUX)
#include <sys/types.h>
#include <sys/wait.h>
#endif

namespace engine
{
   /*
      _iPmdProc implement
   */

   BOOLEAN _iPmdProc::_isRunning = FALSE;

   _iPmdProc::_iPmdProc()
   {
      _isRunning = TRUE ;
   }

   _iPmdProc::~_iPmdProc()
   {
   }

   void _iPmdProc::stop( INT32 sigNum )
   {
      if ( 0 != sigNum )
      {
         PD_LOG( PDEVENT, "Recieved signal[%d], stop...", sigNum ) ;
      }
      _isRunning = FALSE ;
   }

   INT32 _iPmdProc::regSignalHandler()
   {
      INT32 rc = SDB_OK;
#if defined (_LINUX)
      ossSigSet sigSet ;
      sigSet.sigAdd( SIGHUP ) ;
      sigSet.sigAdd( SIGINT ) ;
      sigSet.sigAdd( SIGTERM ) ;
      sigSet.sigAdd( SIGPWR ) ;
      rc = ossRegisterSignalHandle( sigSet,
               (SIG_HANDLE)(&iPmdProc::stop) ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to register signals, rc = %d",
                   rc ) ;

   done:
      return rc ;
   error:
      goto done ;
#else
      return rc ;
#endif
   }

   BOOLEAN _iPmdProc::isRunning()
   {
      return _isRunning ;
   }

}
