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

   Source File Name = qgmPIMthMatcherFilter.cpp

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

#include "qgmPlMthMatcherFilter.hpp"
#include "pdTrace.hpp"
#include "qgmTrace.hpp"

using namespace bson;

namespace engine
{
   qgmPlMthMatcherFilter::qgmPlMthMatcherFilter( const qgmOPFieldVec &selector,
                                                INT64 numSkip,
                                                INT64 numReturn,
                                                const qgmField &alias )
   :_qgmPlFilter( selector, NULL, numSkip, numReturn, alias )
   {
   }

   INT32 qgmPlMthMatcherFilter::loadPattern( bson::BSONObj matcher )
   {
      return _matcher.loadPattern( matcher );
   }

   //PD_TRACE_DECLARE_FUNCTION( SDB__QGMPLMTHMATCHERFILTER__FETCHNEXT, "qgmPlMthMatcherFilter::_fetchNext" )
   INT32 qgmPlMthMatcherFilter::_fetchNext( qgmFetchOut & next )
   {
      PD_TRACE_ENTRY( SDB__QGMPLMTHMATCHERFILTER__FETCHNEXT ) ;
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
         BOOLEAN r = FALSE ;
         rc = _matcher.matches( fetch.obj, r );
         if ( rc != SDB_OK )
         {
            goto error;
         }
         else if ( !r )
         {
            continue;
         }
         else
         {
            /// do nothing
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
      PD_TRACE_EXITRC( SDB__QGMPLMTHMATCHERFILTER__FETCHNEXT, rc ) ;
      return rc ;
   error:
      goto done ;
   }
}
