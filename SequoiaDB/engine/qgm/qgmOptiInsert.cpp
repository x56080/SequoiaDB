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

   Source File Name = qgmOptiInsert.cpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains functions for agent processing.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/14/2012  YW  Initial Draft

   Last Changed =

******************************************************************************/

#include "qgmOptiInsert.hpp"

namespace engine
{
   _qgmOptiInsert::_qgmOptiInsert( _qgmPtrTable *table,
                                   _qgmParamTable *param )
   :_qgmOptiTreeNode( QGM_OPTI_TYPE_INSERT, table, param )
   {

   }

   _qgmOptiInsert::~_qgmOptiInsert()
   {

   }

   INT32 _qgmOptiInsert::outputStream( qgmOpStream &stream )
   {
      return SDB_OK ;
   }

   string _qgmOptiInsert::toString() const
   {
      stringstream ss ;
      ss << "{" << this->_qgmOptiTreeNode::toString() << "}" ;
      return ss.str() ;
   }

}
