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

   Source File Name = fapMongoCursor.cpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains functions for agent processing.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who     Description
   ====== =========== ======= ==============================================
          2020/04/30  Ting YU Initial Draft

   Last Changed =

*******************************************************************************/
#include "fapMongoCursor.hpp"

namespace fap
{
   BOOLEAN _mongoCursorMgr::find( INT64 cursorID, mongoCursorInfo& info )
   {
      BOOLEAN foundOut = FALSE ;

      if ( cursorID != MONGO_INVALID_CURSORID )
      {
         CURSOR_MAP::Bucket& bucket = _cursorMap.getBucket( cursorID ) ;
         BUCKET_SLOCK( bucket ) ;
         CURSOR_MAP::map_const_iterator itr = bucket.find( cursorID ) ;
         if ( itr != bucket.end() )
         {
            info = (*itr).second ;
            foundOut = TRUE ;
         }
      }

      return foundOut ;
   }

   INT32 _mongoCursorMgr::insert( const mongoCursorInfo& info )
   {
      INT32 rc = SDB_OK ;

      if ( info.cursorID != MONGO_INVALID_CURSORID )
      {
         CURSOR_MAP::Bucket& bucket = _cursorMap.getBucket( info.cursorID ) ;
         BUCKET_XLOCK( bucket ) ;

         try
         {
            bucket.insert( CURSOR_MAP::value_type( info.cursorID, info ) ) ;
         }
         catch( std::exception &e )
         {
            PD_RC_CHECK( SDB_OOM, PDERROR, "Exception occurred: %s", e.what() ) ;
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   void _mongoCursorMgr::remove( INT64 cursorID )
   {
      if ( cursorID != MONGO_INVALID_CURSORID )
      {
         CURSOR_MAP::Bucket& bucket = _cursorMap.getBucket( cursorID ) ;
         BUCKET_XLOCK( bucket ) ;
         bucket.erase( cursorID ) ;
      }
   }

   _mongoCursorMgr* getMongoCursorMgr()
   {
      static _mongoCursorMgr cursorMgr ;
      return &cursorMgr ;
   }

}
