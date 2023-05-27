/*******************************************************************************


   Copyright (C) 2011-2018 SequoiaDB Ltd.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU Affero General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.

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
#include "msgDef.hpp"

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

   INT32 _qgmOptiSplit::parseArguments( const BSONObj &hints )
   {
      INT32 rc = SDB_OK ;
      BSONElement ele = hints.getField( FIELD_NAME_STRICT ) ;
      if ( !ele.eoo() )
      {
         if ( !ele.isBoolean() )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG( PDERROR, "Invalid value for strict[%s], rc: %d",
                    ele.toString( TRUE, TRUE ).c_str(), rc ) ;
            goto error ;
         }
         _arguments.strict = ele.Bool() ;
      }

      ele = hints.getField( FIELD_NAME_ARRAY_INDEX_ALIAS ) ;
      if ( !ele.eoo() )
      {
         if ( String != ele.type() || 0 == ele.valuestrsize() )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG( PDERROR, "Invalid value for array index alias[%s], rc: %d",
                    ele.toString( TRUE, TRUE ).c_str(), rc ) ;
            goto error ;
         }
         _arguments.arrayIndexAlias = ele.valuestr() ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   const qgmSplitByArgs* _qgmOptiSplit::getArguments() const
   {
      return &_arguments ;
   }
}
