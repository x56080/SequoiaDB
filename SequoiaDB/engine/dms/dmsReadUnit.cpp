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

   Source File Name = dmsReadUnit.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          11/20/2023  HGM Initial Draft

   Last Changed =

*******************************************************************************/
#include "dmsReadUnit.hpp"
#include "dmsDef.hpp"
#include "ossErr.h"
#include "ossMem.hpp"
#include "pmdEDU.hpp"
#include "pdTrace.hpp"
#include "dmsTrace.hpp"

namespace engine
{

   /*
      _dmsReadUnitScope implement
    */
   _dmsReadUnitScope::_dmsReadUnitScope( IStorageSession *session, _pmdEDUCB *cb )
   : _cb( cb ),
     _currentReadUnit( session )
   {
      if ( NULL == _cb->getSession() )
      {
         _dummySession.attachCB( _cb ) ;
         _attached = TRUE ;
      }
      SDB_ASSERT( _cb->getSession()->getOperationContext(),
                  "operation context should be valid" ) ;
      _lastReadUnit = _cb->getSession()->getOperationContext()->getReadUnit() ;
      _cb->getSession()->getOperationContext()->setReadUnit( &_currentReadUnit ) ;
   }

   _dmsReadUnitScope::~_dmsReadUnitScope()
   {
      _cb->getSession()->getOperationContext()->setReadUnit( _lastReadUnit ) ;
      if ( _attached )
      {
         _dummySession.detachCB() ;
      }
   }

}
