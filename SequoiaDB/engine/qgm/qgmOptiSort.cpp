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

   Source File Name = qgmOptiSort.cpp

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

#include "qgmOptiSort.hpp"
#include "qgmOprUnit.hpp"
#include "pdTrace.hpp"
#include "qgmTrace.hpp"

namespace engine
{
   _qgmOptiSort::_qgmOptiSort( _qgmPtrTable *table, _qgmParamTable *param )
   :_qgmOptiTreeNode( QGM_OPTI_TYPE_SORT, table, param )
   {
   }

   _qgmOptiSort::~_qgmOptiSort()
   {
   }

   qgmOPFieldVec* _qgmOptiSort::getOrderby()
   {
      return &_orderby ;
   }

   // PD_TRACE_DECLARE_FUNCTION( SDB__QGMOPTISORT_INIT, "_qgmOptiSort::init" )
   INT32 _qgmOptiSort::init()
   {
      PD_TRACE_ENTRY( SDB__QGMOPTISORT_INIT ) ;
      INT32 rc = SDB_OK ;

      // create a sort unit
      qgmOprUnit *sortUnit = SDB_OSS_NEW qgmOprUnit( QGM_OPTI_TYPE_SORT ) ;
      if ( !sortUnit )
      {
         rc = SDB_OOM ;
         goto error ;
      }
      sortUnit->setFields( _orderby ) ;
      _oprUnits.push_back( sortUnit ) ;

   done:
      PD_TRACE_EXITRC( SDB__QGMOPTISORT_INIT, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION( SDB__QGMOPTISORT__PUSHOPRUNIT, "_qgmOptiSort::_pushOprUnit" )
   INT32 _qgmOptiSort::_pushOprUnit( qgmOprUnit * oprUnit, PUSH_FROM from )
   {
      PD_TRACE_ENTRY( SDB__QGMOPTISORT__PUSHOPRUNIT ) ;
      INT32 rc = SDB_OK ;
      qgmOprUnit *typeUnit = getOprUnitByType( oprUnit->getType() ) ;

      if ( QGM_OPTI_TYPE_SORT == oprUnit->getType() )
      {
         if ( typeUnit )
         {
            removeOprUnit( typeUnit, TRUE, TRUE ) ;
         }
         _orderby = *(oprUnit->getFields()) ;
         // need to push frist
         _oprUnits.insert( _oprUnits.begin(), oprUnit ) ;
      }
      else if ( QGM_OPTI_TYPE_FILTER == oprUnit->getType() )
      {
         if ( !typeUnit )
         {
            if ( oprUnit->getFields()->size() > 0 )
            {
               oprUnit->addOpField( _orderby, TRUE ) ;
            }
            _oprUnits.push_back( oprUnit ) ;
         }
         else // merge filter
         {
            qgmFilterUnit *oldUnit = (qgmFilterUnit*)typeUnit ;
            qgmFilterUnit *newUnit = (qgmFilterUnit*)oprUnit ;

            if ( !newUnit->isWildCardField() &&
                  newUnit->getFields()->size() != 0 )
            {
               oldUnit->setFields( *(newUnit->getFields()) ) ;
               oldUnit->addOpField( _orderby, TRUE ) ;
            }

            if ( newUnit->hasCondition() )
            {
               qgmConditionNodePtrVec conds = newUnit->getConditions() ;
               oldUnit->addCondition( conds ) ;
               newUnit->emptyCondition() ;
            }

            SDB_OSS_DEL newUnit ;
         }
      }
      else
      {
         rc = SDB_SYS ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__QGMOPTISORT__PUSHOPRUNIT, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION( SDB__QGMOPTISORT__RMOPRUNIT, "_qgmOptiSort::_removeOprUnit" )
   INT32 _qgmOptiSort::_removeOprUnit( qgmOprUnit * oprUnit )
   {
      PD_TRACE_ENTRY( SDB__QGMOPTISORT__RMOPRUNIT ) ;
      INT32 rc = SDB_OK ;

      if ( oprUnit->isOptional() || oprUnit->isNodeIDValid() )
      {
         goto done ;
      }

      if ( QGM_OPTI_TYPE_SORT == oprUnit->getType() )
      {
         _orderby.clear() ;
      }
      else
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Node[%s] remove oprUnit[%s] type error",
                 toString().c_str(), oprUnit->toString().c_str() ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__QGMOPTISORT__RMOPRUNIT, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   INT32 _qgmOptiSort::_updateChange( qgmOprUnit * oprUnit )
   {
      _orderby = *(oprUnit->getFields()) ;
      return SDB_OK ;
   }

   BOOLEAN _qgmOptiSort::isEmpty()
   {
      return getOprUnitCount() == 0 ? TRUE : FALSE ;
   }

   INT32 _qgmOptiSort::outputStream( qgmOpStream &stream )
   {
      return (*(_children.begin()))->outputStream( stream ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION( SDB__QGMOPTISORT_APPEND, "_qgmOptiSort::append" )
   INT32 _qgmOptiSort::append( const qgmOPFieldVec &field,
                               BOOLEAN keepRelegation )
   {
      PD_TRACE_ENTRY( SDB__QGMOPTISORT_APPEND ) ;
      INT32 rc = SDB_OK ;

      qgmOPFieldVec::const_iterator itr = field.begin() ;
      for ( ; itr != field.end(); itr++ )
      {
         if ( keepRelegation )
         {
            _orderby.push_back( *itr ) ;
         }
         else
         {
            qgmOpField f ;
            f.value.attr() = itr->value.attr() ;
            _orderby.push_back( f ) ;
         }
      }
      PD_TRACE_EXITRC( SDB__QGMOPTISORT_APPEND, rc ) ;
      return rc ;
   }

   string _qgmOptiSort::toString() const
   {
      stringstream ss ;
      ss << "{" << this->_qgmOptiTreeNode::toString() ;
      if ( !_orderby.empty() )
      {
         ss << ",orderby:[" ;
         vector<qgmOpField>::const_iterator itr =
                               _orderby.begin() ;
         for ( ; itr != _orderby.end(); itr++ )
         {
            string t ;
            if ( SQL_GRAMMAR::DBATTR == itr->type
                 || SQL_GRAMMAR::ASC == itr->type )
            {
               t = "asc" ;
            }
            else
            {
               t = "desc" ;
            }
            ss << "{value:" << itr->value.toString()
               << ",type:" << t << "}," ;
         }
         ss.seekp((INT32)ss.tellp()-1 ) ;
         ss << "]}" ;
      }
      return ss.str() ;
   }

}

