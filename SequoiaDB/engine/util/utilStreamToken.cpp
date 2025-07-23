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

   Source File Name = utilStreamToken.cpp

   Descriptive Name = Change Stream Token Source

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          06/01/2023  HGM Initial Draft

   Last Changed =

*******************************************************************************/
#include "utilStreamToken.hpp"
#include "dpsDef.hpp"
#include "ossUtil.h"
#include "pdTrace.hpp"
#include "utilTrace.hpp"

using namespace std ;

namespace engine
{

   #define UTIL_STREAM_TOKEN_CHANGE_NAME     "change"
   #define UTIL_STREAM_TOKEN_DATA_NAME       "data"
   #define UTIL_STREAM_TOKEN_UNKNOWN_NAME    "unknown"

   #define UTIL_STREAM_CHANGE_NAME  "change"
   #define UTIL_STREAM_REPLICA_NAME "replica"
   #define UTIL_STREAM_UNKNOWN_NAME "unknown"

   #define UTIL_STREAM_CONTROL_RECORD_NAME   "control"
   #define UTIL_STREAM_CHANGE_RECORD_NAME    "change"
   #define UTIL_STREAM_DATA_RECORD_NAME      "data"
   #define UTIL_STREAM_UNKNOWN_RECORD_NAME   "unknown"

   #define UTIL_STREAM_EMPTY_CONTROL_NAME    "empty"
   #define UTIL_STREAM_STOP_CONTROL_NAME     "stop"
   #define UTIL_STREAM_ERROR_CONTROL_NAME    "error"
   #define UTIL_STREAM_UNKNOWN_CONTROL_NAME  "unknown"

   // helper functions implementation
   const CHAR *utilGetTokenTypeName( UINT8 type )
   {
      const CHAR *result = UTIL_STREAM_TOKEN_UNKNOWN_NAME ;

      switch ( type )
      {
         case MSG_STREAM_TOKEN_TYPE_CHANGE:
            result = UTIL_STREAM_TOKEN_CHANGE_NAME ;
            break ;
         case MSG_STREAM_TOKEN_TYPE_DATA:
            result = UTIL_STREAM_TOKEN_DATA_NAME ;
            break ;
         default:
            break ;
      }

      return result ;
   }

   const CHAR *utilGetStreamTypeName( utilStreamType type )
   {
      const CHAR *result = UTIL_STREAM_UNKNOWN_RECORD_NAME ;

      switch ( type )
      {
         case UTIL_CHANGE_STREAM:
            result = UTIL_STREAM_CHANGE_NAME ;
            break ;
         case UTIL_REPLICA_STREAM:
            result = UTIL_STREAM_REPLICA_NAME ;
            break ;
         default:
            break ;
      }

      return result ;
   }

   const CHAR *utilGetStreamRecordTypeName( utilStreamRecordType recordType )
   {
      const CHAR *result = UTIL_STREAM_UNKNOWN_RECORD_NAME ;

      switch ( recordType )
      {
         case UTIL_STREAM_CONTROL_RECORD:
            result = UTIL_STREAM_CONTROL_RECORD_NAME ;
            break ;
         case UTIL_STREAM_CHANGE_RECORD:
            result = UTIL_STREAM_CHANGE_RECORD_NAME ;
            break ;
         case UTIL_STREAM_DATA_RECORD:
            result = UTIL_STREAM_DATA_RECORD_NAME ;
            break ;
         default:
            break ;
      }

      return result ;
   }

   const CHAR *utilGetStreamControlTypeName( utilStreamControlType controlType )
   {
      const CHAR *result = UTIL_STREAM_UNKNOWN_CONTROL_NAME ;

      switch ( controlType )
      {
         case UTIL_STREAM_CONTROL_EMPTY:
            result = UTIL_STREAM_EMPTY_CONTROL_NAME ;
            break ;
         case UTIL_STREAM_CONTROL_STOP:
            result = UTIL_STREAM_STOP_CONTROL_NAME ;
            break ;
         case UTIL_STREAM_CONTROL_ERROR:
            result = UTIL_STREAM_ERROR_CONTROL_NAME ;
            break ;
         default:
            break ;
      }

      return result ;
   }

   /*
      _utilStreamToken implement
    */
   _utilStreamTokenBase::_utilStreamTokenBase()
   {
      ossMemset( &( _token._values ), 0, MSG_STREAM_TOKEN_SIZE ) ;
      _token._struct._header._version = MSG_STREAM_TOKEN_VERSION_CUR ;
      _token._struct._header._flags = MSG_STREAM_TOKEN_FLAG_EMPTY ;
   }

   _utilStreamTokenBase::_utilStreamTokenBase( const _utilStreamTokenBase &other )
   {
      ossMemcpy( &( _token._values ),
                 &( other._token._values ),
                 MSG_STREAM_TOKEN_SIZE ) ;
   }

   _utilStreamTokenBase::~_utilStreamTokenBase()
   {
   }

   _utilStreamTokenBase &_utilStreamTokenBase::operator =( const _utilStreamTokenBase &other )
   {
      if ( this != &other )
      {
         ossMemcpy( &( _token._values ),
                    &( other._token._values ),
                    MSG_STREAM_TOKEN_SIZE ) ;
      }
      return *this ;
   }

   void _utilStreamTokenBase::reset()
   {
      ossMemset( &( _token._values ), 0, MSG_STREAM_TOKEN_SIZE ) ;
   }

   void _utilStreamTokenBase::resetDesc()
   {
      ossMemset( &( _token._struct._desc ), 0, MSG_STREAM_TOKEN_DESC_SIZE ) ;
   }

   INT32 _utilStreamTokenBase::_headerToBSON( bson::BSONObjBuilder &builder ) const
   {
      INT32 rc = SDB_OK ;

      try
      {
         builder.append( FIELD_NAME_VERSION, _getHeader()._version ) ;
         builder.append( FIELD_NAME_TYPE,
                         utilGetTokenTypeName( _getHeader()._tokenType ) ) ;
         builder.append( FIELD_NAME_TOKEN_FLAGS, _getHeader()._flags ) ;
         builder.append( FIELD_NAME_SOURCE, _getHeader()._source ) ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to build BSON for header, occurred exception %s",
                 e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

   done:
      return rc ;

   error:
      goto done ;
   }

   /*
      _utilChangeStreamToken implement
    */
   _utilChangeStreamToken::_utilChangeStreamToken()
   : _utilStreamTokenBase()
   {
      setTokenType( MSG_STREAM_TOKEN_TYPE_CHANGE ) ;
   }

   _utilChangeStreamToken::_utilChangeStreamToken( const _utilChangeStreamToken &other )
   : _utilStreamTokenBase( other )
   {
   }

   _utilChangeStreamToken::_utilChangeStreamToken( const _utilStreamTokenBase &other )
   : _utilStreamTokenBase( other )
   {
   }

   _utilChangeStreamToken::~_utilChangeStreamToken()
   {
   }

   void _utilChangeStreamToken::fromString( const CHAR *str )
   {
      reset() ;

      if ( NULL != str && '\0' != str[ 0 ] )
      {
         UINT32 version = 0, tokenType = 0, flags = 0, headerReserved = 0,
                source = 0, checkCode = 0, descReserved = 0 ;
         UINT64 globalTimestamp = 0, lsn = 0 ;
         ossSscanf( str, MSG_CHANGE_STREAM_TOKEN_FORMAT,
                    &( version ), &( tokenType ), &( flags ),
                    &( headerReserved ), &( source ), &( globalTimestamp ),
                    &( lsn ), &( checkCode ), &( descReserved ) ) ;
         _getHeader()._version = version ;
         _getHeader()._tokenType = tokenType ;
         _getHeader()._flags = flags ;
         _getHeader()._reserved = headerReserved ;
         _getHeader()._source = source ;
         _getDesc()._globalTimestamp = globalTimestamp ;
         _getDesc()._lsn = lsn ;
         _getDesc()._checkCode = checkCode ;
         _getDesc()._reserved = descReserved ;
      }
      else
      {
         _getDesc()._lsn = DPS_INVALID_LSN_OFFSET ;
         _getDesc()._checkCode = DPS_INVALID_LSN_VERSION ;
      }
   }

   void _utilChangeStreamToken::toString( CHAR *str ) const
   {
      ossSnprintf( str,
                   MSG_STREAM_TOKEN_STING_SIZE + 1,
                   MSG_CHANGE_STREAM_TOKEN_FORMAT,
                   _getHeader()._version,
                   _getHeader()._tokenType,
                   _getHeader()._flags,
                   _getHeader()._reserved,
                   _getHeader()._source,
                   _getDesc()._globalTimestamp,
                   _getDesc()._lsn,
                   _getDesc()._checkCode,
                   _getDesc()._reserved ) ;
   }

   INT32 _utilChangeStreamToken::toBSON( BSONObjBuilder &builder ) const
   {
      INT32 rc = SDB_OK ;

      rc = _headerToBSON( builder ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to build BSON for header, rc: %d", rc ) ;

      try
      {
         builder.append( FIELD_NAME_GLOB_TIMESTAMP, (INT64)( _getDesc()._globalTimestamp ) ) ;
         builder.append( FIELD_NAME_LSN, (INT64)( _getDesc()._lsn ) ) ;
         builder.append( FIELD_NAME_CHECK_CODE, _getDesc()._checkCode ) ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to build BSON, occurred exception %s",
                 e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

   done:
      return rc ;

   error:
      goto done ;
   }

}
