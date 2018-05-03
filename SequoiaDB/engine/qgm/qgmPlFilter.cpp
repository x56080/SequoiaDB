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

   Source File Name = qgmPlFilter.cpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains declare for QGM operators

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          04/09/2013  YW  Initial Draft

   Last Changed =

*******************************************************************************/

#include "qgmPlFilter.hpp"
#include "qgmMatcher.hpp"
#include "ossMem.hpp"
#include "qgmConditionNode.hpp"
#include "qgmUtil.hpp"
#include "pdTrace.hpp"
#include "qgmTrace.hpp"
#include <sstream>

namespace engine
{
   _qgmPlFilter::_qgmPlFilter(  const qgmOPFieldVec &selector,
                                _qgmConditionNode *condition,
                                INT64 numSkip,
                                INT64 numReturn,
                                const qgmField &alias )
   :_qgmPlan( QGM_PLAN_TYPE_FILTER, alias ),
    _return( numReturn ),
    _skip( numSkip ),
    _currentSkip( 0 ),
    _currentReturn( 0 ),
    _matcher( condition ),
    _condition( NULL )
   {
      if ( _matcher.ready() )
      {
         _condition = condition ;
      }

      _selector.load( selector ) ;
      _initialized = TRUE ;
   }

   _qgmPlFilter::~_qgmPlFilter()
   {
      SAFE_OSS_DELETE( _condition ) ;
   }

   string _qgmPlFilter::toString() const
   {
      stringstream ss ;

      ss << "Type:" << qgmPlanType( _type ) << '\n';
      ss << "Selector:" << _selector.toString() << '\n';
      if ( !_alias.empty() )
      {
         ss << "Alias:" << _alias.toString() << '\n';
      }
      ss << "Condtion:" << _matcher.toString() << '\n';
      if ( 0 != _skip )
      {
         ss << "Skip:" << _skip << '\n';
      }
      if ( -1 != _return )
      {
         ss << "Limit:" << _return << '\n';
      }
      return ss.str() ;
   }

   INT32 _qgmPlFilter::_execute( _pmdEDUCB *eduCB )
   {
      INT32 rc = SDB_OK ;

      SDB_ASSERT( 1 == inputSize(), "impossible" ) ;
      _currentSkip = 0 ;
      _currentReturn = 0 ;
      _matcher.resetDataNode() ;

      rc = input( 0 )->execute( eduCB ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION( SDB__QGMPLFILTER__FETCHNEXT, "_qgmPlFilter::_fetchNext" )
   INT32 _qgmPlFilter::_fetchNext( qgmFetchOut &next )
   {
      PD_TRACE_ENTRY( SDB__QGMPLFILTER__FETCHNEXT ) ;
      INT32 rc = SDB_OK ;
      qgmFetchOut fetch ;
      _qgmPlan *in = input( 0 ) ;

      if ( 0 <= _return && _return <= _currentReturn )
      {
         close() ;
         rc = SDB_DMS_EOC ;
         goto error ;
      }

      while ( TRUE )
      {
         rc = in->fetchNext( fetch ) ;
         if ( SDB_OK != rc && SDB_DMS_EOC != rc )
         {
            goto error ;
         }
         else if ( SDB_DMS_EOC == rc )
         {
            break ;
         }
         else
         {
            /// do noting.
         }

         /// match
         if ( NULL != _condition )
         {
            BOOLEAN r = FALSE ;
            rc = _matcher.match( fetch, r ) ;
            if ( SDB_OK != rc )
            {
               goto error ;
            }
            else if ( !r )
            {
               continue ;
            }
            else
            {
               /// do nothing.
            }
         }

         /// skip
         if ( 0 < _skip && ++_currentSkip <= _skip )
         {
            continue ;
         }

         /// get needed fields.
         if ( !_selector.empty() )
         {
            rc = _selector.select( fetch,
                                   next.obj ) ;
            if ( SDB_OK != rc )
            {
               goto error ;
            }
         }
         else
         {
            next.obj = fetch.mergedObj() ;
         }

         if ( !_merge )
         {
            /// if sub input is a join, can _alias be empty?
            next.alias = _alias.empty()?
                         fetch.alias : _alias ;
         }

         ++_currentReturn ;
         break ;
      }
   done:
      PD_TRACE_EXITRC( SDB__QGMPLFILTER__FETCHNEXT, rc ) ;
      return rc ;
   error:
      goto done ;
   }

}
