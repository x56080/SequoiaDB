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

   Source File Name = qgmOptiCommand.hpp

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

#ifndef QGMOPTICOMMAND_HPP_
#define QGMOPTICOMMAND_HPP_

#include "qgmOptiTree.hpp"

namespace engine
{
   class _qgmOptiCommand : public _qgmOptiTreeNode
   {
   public:
      _qgmOptiCommand( _qgmPtrTable *table,
                       _qgmParamTable *param )
      :_qgmOptiTreeNode( QGM_OPTI_TYPE_COMMAND,
                         table, param ),
       _commandType( SQL_GRAMMAR::SQLMAX ),
       _uniqIndex( FALSE )
      {

      }

      virtual ~_qgmOptiCommand(){}

   public:
      virtual INT32 outputStream( qgmOpStream &stream )
      {
         return SDB_SYS ;
      }

   public:
      INT32 _commandType ;
      qgmDbAttr _fullName ;
      qgmField _indexName ;
      qgmOPFieldVec _indexColumns ;
      qgmOPFieldVec _partition ;
      BOOLEAN _uniqIndex ;
   } ;
   typedef class _qgmOptiCommand qgmOptiCommand ;
}

#endif

