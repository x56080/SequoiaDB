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

   Source File Name = qgmOptiSplit.cpp

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

#include "qgmOptiSplit.hpp"

namespace engine
{
   _qgmOptiSplit::_qgmOptiSplit( _qgmPtrTable *table,
                                 _qgmParamTable *param,
                                 const qgmDbAttr &splitby )
   :_qgmOptiTreeNode(QGM_OPTI_TYPE_SPLIT, table, param ),
    _splitby(splitby)
   {
      
   }

   _qgmOptiSplit::~_qgmOptiSplit()
   {

   }

   INT32 _qgmOptiSplit::outputStream( qgmOpStream &stream )
   {
      SDB_ASSERT( 1 == _children.size(), "impossible" ) ;
      return (*(_children.begin()))->outputStream( stream ) ;
   }

   string _qgmOptiSplit::toString() const
   {
      stringstream ss ;
      ss << "{" << this->_qgmOptiTreeNode::toString() ;
      ss << ", split by[" << _splitby.toString() << "]}" ;
      return ss.str() ;
   }
}
