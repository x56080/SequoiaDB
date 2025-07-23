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

   Source File Name = rplMonitorStore.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          01/04/2019  Linyoubin  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef REPLAY_MONITOR_STORE_HPP_
#define REPLAY_MONITOR_STORE_HPP_

#include "oss.hpp"
#include "ossFile.hpp"
#include "rplMonitor.hpp"
#include "rplOutputter.hpp"
#include <string>

using namespace std ;
using namespace engine ;

namespace replay
{
   class rplMonitorStore : public SDBObject {
   public:
      rplMonitorStore( Monitor *monitor ) ;
      ~rplMonitorStore() ;

   public:
      INT32 init( const CHAR *storeFile ) ;
      INT32 init() ;
      INT32 flushAndSave() ;
      INT32 submitAndSave() ;
      INT32 save() ;
      void captureOutputter( rplOutputter *outputter ) ;

   private:
      INT32 _initMonitor( ossFile& statusFile ) ;
      INT32 _saveMonitor() ;

   private:
      rplOutputter *_outputter ;
      Monitor *_monitor ;

      BOOLEAN _isNeedWriteStatusFile ;
      // full file name
      string _tmpStatusFileName ;
      // full file name
      string _statusFileName ;
   } ;
}

#endif  /* REPLAY_MONITOR_STORE_HPP_ */

