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

   Source File Name = monMgr.cpp

   Descriptive Name = Monitor Service Source

   When/how to use: this program may be used on binary and text-formatted
   versions of OSS component. This file contains functions for OSS operations.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          05/24/2019  CW  Initial Draft

   Last Changed =

*******************************************************************************/
#include "monMgr.hpp"

namespace engine
{
_monMonitorManager::_monMonitorManager()
   : _monClass(MON_CLASS_MAX)
{
   for (int i = 0; i < MON_CLASS_MAX; i ++ )
   {
      //TODO: create array of function pointers
      monClassContainer *list = SDB_OSS_NEW monClassContainer((MON_CLASS_TYPE)i) ;
      _monClass[i] = list ;
   }
}

_monMonitorManager::~_monMonitorManager()
{
   for (int i = 0; i < MON_CLASS_MAX; i ++ )
   {
      SDB_OSS_DEL _monClass[i] ;
   }
}

void _monMonitorManager::cleanup()
{
   for (int i = 0; i < MON_CLASS_MAX; i++ )
   {
      monClassContainer *curContainer = _monClass[i] ;

      if ( curContainer->_numPendingArchive.fetch() > 0 ||
           curContainer->_numPendingDelete.fetch() > 0 )
      {
         curContainer->_processPendingObj() ;
      }

      if ( !curContainer->isOperational() ||
           curContainer->getArchivedListLen() >
              curContainer->getMaxArchivedListLen() )
      {
         curContainer->_removeArchivedObj() ;
      }
   }
}
} // namespace engine
