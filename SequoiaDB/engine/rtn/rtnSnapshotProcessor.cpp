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

   Source File Name = rtnSnapshotProcessor.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/19/2022  YQC  Initial Draft

   Last Changed =

*******************************************************************************/
#include "utilMath.hpp"
#include "rtnSnapshotProcessor.hpp"

using namespace bson ;

namespace engine
{

   static BSONObj _transObjWithAgv( const BSONObj &obj )
   {
      INT64 lobCapacity       = 0 ;
      INT64 totalLobs         = 0 ;
      INT64 totalUsedLobSpace = 0 ;
      INT64 totalValidLobSize = 0 ;
      INT64 avgLobSize        = 0 ;
      BSONObjIterator itr1( obj ) ;
      BSONObjIterator itr2( obj ) ;
      BSONObjBuilder builder( obj.objsize() ) ;

      /// get lob info
      while ( itr1.more() )
      {
         BSONElement e = itr1.next() ;
         const CHAR *field = e.fieldName() ;

         if ( 0 == ossStrcmp( field, FIELD_NAME_LOB_CAPACITY ) )
         {
            lobCapacity = e.numberLong() ;
         }
         else if ( 0 == ossStrcmp( field, FIELD_NAME_TOTAL_LOBS ) )
         {
            totalLobs = e.numberLong() ;
         }
         else if ( 0 == ossStrcmp( field, FIELD_NAME_TOTAL_USED_LOB_SPACE ) )
         {
            totalUsedLobSpace = e.numberLong() ;
         }
         else if ( 0 == ossStrcmp( field, FIELD_NAME_TOTAL_VALID_LOB_SIZE ) )
         {
            totalValidLobSize = e.numberLong() ;
         }
      }

      if ( 0 < totalLobs )
      {
         avgLobSize = totalValidLobSize / totalLobs ;
      }

      while ( itr2.more() )
      {
         BSONElement e = itr2.next() ;
         const CHAR *field = e.fieldName() ;
         if ( 0 == ossStrcmp( field, FIELD_NAME_USED_LOB_SPACE_RATIO ) )
         {
            builder.append( FIELD_NAME_USED_LOB_SPACE_RATIO,
                           utilPercentage( totalUsedLobSpace, lobCapacity ) ) ;
         }
         else if ( 0 == ossStrcmp( field, FIELD_NAME_LOB_USAGE_RATE ) )
         {
            builder.append( FIELD_NAME_LOB_USAGE_RATE,
                           utilPercentage( totalValidLobSize, totalUsedLobSpace ) ) ;
         }
         else if ( 0 == ossStrcmp( field, FIELD_NAME_AVG_LOB_SIZE ) )
         {
            builder.append( FIELD_NAME_AVG_LOB_SIZE, avgLobSize ) ;
         }
         else
         {
            builder.append( e ) ;
         }
      }

      return builder.obj() ;
   }

   _rtnCSSnapshotProcessor::_rtnCSSnapshotProcessor()
   {
      _eof = FALSE ;
      _hasDone = FALSE ;
   }

   _rtnCSSnapshotProcessor::~_rtnCSSnapshotProcessor()
   {
      _eof = FALSE ;
      _hasDone = FALSE ;
   }

   INT32 _rtnCSSnapshotProcessor::pushIn( const BSONObj &obj )
   {
      _obj = obj ;
      return SDB_OK ;
   }

   INT32  _rtnCSSnapshotProcessor::output( BSONObj &obj, BOOLEAN & hasOut )
   {
      INT32 rc = SDB_OK ;
      BSONObj emptyObj ;
      try
      {
         if ( !_obj.isEmpty() )
         {
            obj = _transObjWithAgv( _obj ) ;
            _obj = emptyObj ;
            hasOut = TRUE ;
         }
         else
         {
            hasOut = FALSE ;
            if ( _hasDone )
            {
               _eof = TRUE ;
            }
         }
      }
      catch ( std::exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_LOG( PDERROR, "Exception captured:%s when output cs snapshot result",
                 e.what() ) ;
      }
      return rc ;
   }

   INT32 _rtnCSSnapshotProcessor::done( BOOLEAN &hasOut )
   {
      _hasDone = TRUE ;
      if ( !_obj.isEmpty() )
      {
         hasOut = TRUE ;
      }

      return SDB_OK ;
   }

   BOOLEAN _rtnCSSnapshotProcessor::eof() const
   {
      return _eof ;
   }

}  // namespace engine