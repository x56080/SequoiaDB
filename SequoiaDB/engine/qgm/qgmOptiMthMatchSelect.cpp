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

   Source File Name = qgmOptiMthMatchSelect.cpp

   Descriptive Name =

   When/how to use:

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          12/1/2014  ly  Initial Draft

   Last Changed =

*******************************************************************************/

#include "qgmOptiMthMatchSelect.hpp"

using namespace bson;

namespace engine
{
   qgmOptiMthMatchSelect::qgmOptiMthMatchSelect( _qgmPtrTable *pTable,
                                                _qgmParamTable *pParam )
   :_qgmOptiSelect( pTable, pParam )
   {
   }

   qgmOptiMthMatchSelect::~qgmOptiMthMatchSelect()
   {
   }

   BOOLEAN qgmOptiMthMatchSelect::isEmpty()
   {
      return FALSE;
   }

   INT32 qgmOptiMthMatchSelect::fromBson( const BSONObj &matcher )
   {
      _matcher = matcher;
      return SDB_OK;
   }

   INT32 qgmOptiMthMatchSelect::_extend( _qgmOptiTreeNode *&exNode )
   {
      INT32 rc = SDB_OK;
      rc = this->qgmOptiSelect::_extend( exNode );
      PD_RC_CHECK( rc, PDERROR,
                  "extend failed(rc=%d)", rc );
      _type = QGM_OPTI_TYPE_SCAN == _type ?
               QGM_OPTI_TYPE_MTHMCHSCAN : QGM_OPTI_TYPE_MTHMCHFILTER ;
   done:
      return rc;
   error:
      goto done;
   }
}
