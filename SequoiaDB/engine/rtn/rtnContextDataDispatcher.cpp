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

   Source File Name = rtnContextDataDispatcher.cpp

   Descriptive Name = Runtime Context Data Dispatcher

   When/how to use: this program may be used on binary and text-formatted
   versions of Runtime component. This file contains Runtime Context helper
   functions.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          12/04/2017  HGM Initial Draft

   Last Changed =

*******************************************************************************/
#include "rtnContextDataDispatcher.hpp"
#include "pmd.hpp"
#include "pmdCB.hpp"
#include "pdTrace.hpp"
#include "rtnTrace.hpp"

namespace engine
{

   /*
      _IRtnCtxDataProcessor implement
    */
   _IRtnCtxDataProcessor::_IRtnCtxDataProcessor ()
   {
   }

   _IRtnCtxDataProcessor::~_IRtnCtxDataProcessor ()
   {
   }

   /*
      _rtnCtxDataDispatcher implement
    */
   _rtnCtxDataDispatcher::_rtnCtxDataDispatcher ()
   : _hasProcessor( FALSE ),
     _needCheckData( FALSE ),
     _needCheckSubContext( FALSE )
   {
   }

   _rtnCtxDataDispatcher::~_rtnCtxDataDispatcher ()
   {
   }

   void _rtnCtxDataDispatcher::registerProcessor ( _IRtnCtxDataProcessor * processor )
   {
      SDB_ASSERT( NULL != processor, "processor is invalid" ) ;

      BOOLEAN found = FALSE ;

      for ( rtnCtxDataProcessorList::iterator iter = _processors.begin() ;
            iter != _processors.end() ;
            iter ++ )
      {
         if ( ( *iter ) == processor )
         {
            found = TRUE ;
            break ;
         }
      }

      if ( !found )
      {
         _processors.push_back( processor ) ;
         _hasProcessor = TRUE ;
         if ( processor->needCheckData() )
         {
            _needCheckData = TRUE ;
         }
         if ( processor->needCheckSubContext() )
         {
            _needCheckSubContext = TRUE ;
         }
      }
   }

   void _rtnCtxDataDispatcher::unregisterProcessor ( _IRtnCtxDataProcessor * processor )
   {
      SDB_ASSERT( NULL != processor, "processor is invalid" ) ;

      for ( rtnCtxDataProcessorList::iterator iter = _processors.begin() ;
            iter != _processors.end() ;
            iter ++ )
      {
         if ( ( *iter ) == processor )
         {
            _processors.erase( iter ) ;
            break ;
         }
      }
      _clearFlags() ;
   }

   void _rtnCtxDataDispatcher::unregisterAllProcessors ()
   {
      _processors.clear() ;
      _clearFlags() ;
   }

   INT32 _rtnCtxDataDispatcher::_processData ( INT64 processorType,
                                               INT64 dataID,
                                               const CHAR * data,
                                               INT32 dataSize,
                                               INT32 dataNum )
   {
      INT32 rc = SDB_OK ;

      for ( rtnCtxDataProcessorList::iterator iter = _processors.begin() ;
            iter != _processors.end() ;
            iter ++ )
      {
         _IRtnCtxDataProcessor * processor = ( *iter ) ;

         SDB_ASSERT( NULL != processor, "processor is invalid" ) ;

         if ( processorType == (INT64)processor->getProcessType() )
         {
            rc = processor->processData( dataID, data, dataSize, dataNum ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to process data, rc: %d", rc ) ;
         }
         else if ( processor->needCheckData() )
         {
            rc = processor->checkData( dataID, data, dataSize, dataNum ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to check data, rc: %d", rc ) ;
         }
      }

   done :
      return rc ;

   error :
      goto done ;
   }

   INT32 _rtnCtxDataDispatcher::_checkSubContext ( INT64 dataID )
   {
      INT32 rc = SDB_OK ;

      for ( rtnCtxDataProcessorList::iterator iter = _processors.begin() ;
            iter != _processors.end() ;
            iter ++ )
      {
         _IRtnCtxDataProcessor * processor = ( *iter ) ;

         SDB_ASSERT( NULL != processor, "processor is invalid" ) ;

         if ( processor->needCheckSubContext() )
         {
            rc = processor->checkSubContext( dataID ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to check sub context, "
                         "rc: %d", rc ) ;
         }
      }

   done :
      return rc ;

   error :
      goto done ;
   }

   void _rtnCtxDataDispatcher::_clearFlags ()
   {
      if ( _processors.empty() )
      {
         _hasProcessor = FALSE ;
         _needCheckData = FALSE ;
         _needCheckSubContext = FALSE ;
      }
   }

}
