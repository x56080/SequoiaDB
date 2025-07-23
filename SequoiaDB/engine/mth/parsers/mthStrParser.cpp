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

   Source File Name = mthStrParser.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          15/01/2015  YW  Initial Draft

   Last Changed =

*******************************************************************************/
#include "mthStrParser.hpp"
#include "mthSActionFunc.hpp"
#include "pd.hpp"
#include "pdTrace.hpp"
#include "mthTrace.hpp"

namespace engine
{
   ///PD_TRACE_DECLARE_FUNCTION ( SDB__MTHSUBSTRPARSER_PARSE, "_mthSubStrParser::parse" )
   INT32 _mthSubStrParser::parse( const bson::BSONElement &e,
                                  _mthSAction &action ) const
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__MTHSUBSTRPARSER_PARSE ) ;
      INT32 begin = 0 ;
      INT32 limit = -1 ;

      rc = mthParseSubStrArgs( e, TRUE, begin, limit ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "invalid substr argument rc = %d", rc ) ;
      }

      action.setAttribute( MTH_S_ATTR_PROJECTION ) ;
      action.setFunc( &mthSubStrBuild,
                      &mthSubStrGet ) ;
      action.setName( _name.c_str() ) ;
      action.setArg( BSON( "arg1" << begin << "arg2" << limit ) ) ;

      PD_TRACE_EXITRC( SDB__MTHSUBSTRPARSER_PARSE, rc ) ;
      return rc ;

   }

   ///PD_TRACE_DECLARE_FUNCTION ( SDB__MTHSUBSTRCPPARSER_PARSE, "_mthSubStrCPParser::parse" )
   INT32 _mthSubStrCPParser::parse( const bson::BSONElement &e,
                                    _mthSAction &action ) const
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__MTHSUBSTRCPPARSER_PARSE ) ;
      INT32 begin = 0 ;
      INT32 limit = -1 ;

      rc = mthParseSubStrArgs( e, TRUE, begin, limit ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "invalid substrCP argument rc = %d", rc ) ;
      }

      action.setAttribute( MTH_S_ATTR_PROJECTION ) ;
      action.setFunc( &mthSubStrCPBuild,
                      &mthSubStrCPGet ) ;
      action.setName( _name.c_str() ) ;
      action.setArg( BSON( "arg1" << begin << "arg2" << limit ) ) ;

      PD_TRACE_EXITRC( SDB__MTHSUBSTRCPPARSER_PARSE, rc ) ;
      return rc ;

   }

   ///PD_TRACE_DECLARE_FUNCTION ( SDB__MTHSUBSTRBYTESPARSER_PARSE, "_mthSubStrBytesParser::parse" )
   INT32 _mthSubStrBytesParser::parse( const bson::BSONElement &e,
                                       _mthSAction &action ) const
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__MTHSUBSTRBYTESPARSER_PARSE ) ;
      INT32 begin = 0 ;
      INT32 limit = -1 ;

      rc = mthParseSubStrArgs( e, TRUE, begin, limit ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "invalid substrBytes argument rc = %d", rc ) ;
      }

      action.setAttribute( MTH_S_ATTR_PROJECTION ) ;
      action.setFunc( &mthSubStrBytesBuild,
                      &mthSubStrBytesGet ) ;
      action.setName( _name.c_str() ) ;
      action.setArg( BSON( "arg1" << begin << "arg2" << limit ) ) ;

      PD_TRACE_EXITRC( SDB__MTHSUBSTRBYTESPARSER_PARSE, rc ) ;
      return rc ;

   }

   ///PD_TRACE_DECLARE_FUNCTION ( SDB__MTHRIGHTCPPARSER_PARSE, "_mthRightCPParser::parse" )
   INT32 _mthRightCPParser::parse( const bson::BSONElement &e,
                                   _mthSAction &action ) const
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__MTHRIGHTCPPARSER_PARSE ) ;
      INT32 dummyBegin = 0 ;
      INT32 limit = -1 ;

      rc = mthParseSubStrArgs( e, FALSE, dummyBegin, limit ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "invalid rightCP argument rc = %d", rc ) ;
      }

      action.setAttribute( MTH_S_ATTR_PROJECTION ) ;
      action.setFunc( &mthRightCPBuild,
                      &mthRightCPGet ) ;
      action.setName( _name.c_str() ) ;
      action.setArg( BSON( "arg" << limit ) ) ;

      PD_TRACE_EXITRC( SDB__MTHRIGHTCPPARSER_PARSE, rc ) ;
      return rc ;

   }

   ///PD_TRACE_DECLARE_FUNCTION ( SDB__MTHRIGHTBYTESPARSER_PARSE, "_mthRightBytesParser::parse" )
   INT32 _mthRightBytesParser::parse( const bson::BSONElement &e,
                                      _mthSAction &action ) const
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__MTHRIGHTBYTESPARSER_PARSE ) ;
      INT32 dummyBegin = 0 ;
      INT32 limit = -1 ;

      rc = mthParseSubStrArgs( e, FALSE, dummyBegin, limit ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "invalid rightBytes argument rc = %d", rc ) ;
      }

      action.setAttribute( MTH_S_ATTR_PROJECTION ) ;
      action.setFunc( &mthRightBytesBuild,
                      &mthRightBytesGet ) ;
      action.setName( _name.c_str() ) ;
      action.setArg( BSON( "arg" << limit ) ) ;

      PD_TRACE_EXITRC( SDB__MTHRIGHTBYTESPARSER_PARSE, rc ) ;
      return rc ;

   }

   ///PD_TRACE_DECLARE_FUNCTION ( SDB__MTHLEFTCPPARSER_PARSE, "_mthLeftCPParser::parse" )
   INT32 _mthLeftCPParser::parse( const bson::BSONElement &e,
                                  _mthSAction &action ) const
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__MTHLEFTCPPARSER_PARSE ) ;
      INT32 dummyBegin = 0 ;
      INT32 limit = -1 ;

      rc = mthParseSubStrArgs( e, FALSE, dummyBegin, limit ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "invalid leftCP argument rc = %d", rc ) ;
      }

      action.setAttribute( MTH_S_ATTR_PROJECTION ) ;
      action.setFunc( &mthLeftCPBuild,
                      &mthLeftCPGet ) ;
      action.setName( _name.c_str() ) ;
      action.setArg( BSON( "arg" << limit ) ) ;

      PD_TRACE_EXITRC( SDB__MTHLEFTCPPARSER_PARSE, rc ) ;
      return rc ;

   }

   ///PD_TRACE_DECLARE_FUNCTION ( SDB__MTHLEFTBYTESPARSER_PARSE, "_mthLeftBytesParser::parse" )
   INT32 _mthLeftBytesParser::parse( const bson::BSONElement &e,
                                     _mthSAction &action ) const
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__MTHLEFTBYTESPARSER_PARSE ) ;
      INT32 dummyBegin = 0 ;
      INT32 limit = -1 ;

      rc = mthParseSubStrArgs( e, FALSE, dummyBegin, limit ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "invalid leftBytes argument rc = %d", rc ) ;
      }

      action.setAttribute( MTH_S_ATTR_PROJECTION ) ;
      action.setFunc( &mthLeftBytesBuild,
                      &mthLeftBytesGet ) ;
      action.setName( _name.c_str() ) ;
      action.setArg( BSON( "arg" << limit ) ) ;

      PD_TRACE_EXITRC( SDB__MTHLEFTBYTESPARSER_PARSE, rc ) ;
      return rc ;

   }

   ///PD_TRACE_DECLARE_FUNCTION ( SDB__MTHCONCATPARSER_PARSE, "_mthConcatParser::parse" )
   INT32 _mthConcatParser::parse( const bson::BSONElement &e,
                                  _mthSAction &action ) const
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__MTHCONCATPARSER_PARSE ) ;

      BOOLEAN isReturnNull = FALSE ;
      _utilString<> prefix, suffix ;

      switch ( e.type() )
      {
         case MinKey :
         case EOO :
         case BinData :
         case Undefined :
         case jstNULL :
         case RegEx :
         case DBRef :
         case CodeWScope :
         case Symbol :
         case Code :
         case MaxKey :
            isReturnNull = TRUE ;
            break ;
         case String :
         {
            rc = suffix.append( e.valuestr() ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "failed to append String:%d", rc ) ;
               goto error ;
            }
            break ;
         }
         case NumberInt :
         case NumberLong :
         case NumberDouble :
         case NumberDecimal :
         case Date :
         case Timestamp :
         case Object :
         case jstOID :
         case Bool :
         {
            rc = mthToString( e, suffix, isReturnNull ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "failed to convert element to string rc = %d", rc ) ;
               goto error ;
            }
            break ;
         }
         case Array :
         {
            rc = mthParseConcatArrayArgs( e, isReturnNull, prefix, suffix ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "failed to parse array argument rc = %d", rc ) ;
               goto error ;
            }
            break ;
         }
         default:
            isReturnNull = TRUE ;
            break ;
      }

      action.setAttribute( MTH_S_ATTR_PROJECTION ) ;
      action.setFunc( &mthConcatBuild, &mthConcatGet ) ;
      action.setName( _name.c_str() ) ;
      action.setArg( BSON( "arg1" << prefix.str() <<
                           "arg2" << suffix.str() <<
                           "arg3" << isReturnNull ) ) ;

   done:
      PD_TRACE_EXITRC( SDB__MTHCONCATPARSER_PARSE, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   ///PD_TRACE_DECLARE_FUNCTION ( SDB__MTHSTRLENPARSER_PARSE, "_mthStrLenParser::parse" )
   INT32 _mthStrLenParser::parse( const bson::BSONElement &e,
                                  _mthSAction &action ) const
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY(SDB__MTHSTRLENPARSER_PARSE ) ;

      if ( e.eoo() )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "The type of %s field can't be EOO",
                     e.fieldName() ) ;
         goto error ;
      }

      if ( !mthIsNumber1( e ) )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "The value of %s must be 1", e.fieldName() ) ;
         goto error ;
      }

      action.setAttribute( MTH_S_ATTR_PROJECTION ) ;
      action.setFunc( &mthStrLenBuild,
                      &mthStrLenGet ) ;
      action.setName( _name.c_str() ) ;
   done:
      PD_TRACE_EXITRC( SDB__MTHSTRLENPARSER_PARSE, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   ///PD_TRACE_DECLARE_FUNCTION ( SDB__MTHSTRLENBYTESPARSER_PARSE, "_mthStrLenBytesParser::parse" )
   INT32 _mthStrLenBytesParser::parse( const bson::BSONElement &e,
                                       _mthSAction &action ) const
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY(SDB__MTHSTRLENBYTESPARSER_PARSE ) ;
#if defined (_DEBUG)
      if ( 0 != _name.compare( e.fieldName() ) )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "Invalid field name[%s]", e.fieldName() ) ;
         goto error ;
      }
#endif
      if ( e.eoo() )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "The type of %s field can't be EOO",
                     e.fieldName() ) ;
         goto error ;
      }

      if ( !mthIsNumber1( e ) )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "The value of %s must be 1", e.fieldName() ) ;
         goto error ;
      }

      action.setAttribute( MTH_S_ATTR_PROJECTION ) ;
      action.setFunc( &mthStrLenBytesBuild,
                      &mthStrLenBytesGet ) ;
      action.setName( _name.c_str() ) ;
   done:
      PD_TRACE_EXITRC( SDB__MTHSTRLENBYTESPARSER_PARSE, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   ///PD_TRACE_DECLARE_FUNCTION ( SDB__MTHSTRLENCPPARSER_PARSE, "_mthStrLenCPParser::parse" )
   INT32 _mthStrLenCPParser::parse( const bson::BSONElement &e,
                                    _mthSAction &action ) const
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY(SDB__MTHSTRLENCPPARSER_PARSE ) ;
#if defined (_DEBUG)
      if ( 0 != _name.compare( e.fieldName() ) )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "Invalid field name[%s]", e.fieldName() ) ;
         goto error ;
      }
#endif
      if ( e.eoo() )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "The type of %s field can't be EOO",
                     e.fieldName() ) ;
         goto error ;
      }

      if ( !mthIsNumber1( e ) )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "The value of %s must be 1", e.fieldName() ) ;
         goto error ;
      }

      action.setAttribute( MTH_S_ATTR_PROJECTION ) ;
      action.setFunc( &mthStrLenCPBuild,
                      &mthStrLenCPGet ) ;
      action.setName( _name.c_str() ) ;
   done:
      PD_TRACE_EXITRC( SDB__MTHSTRLENCPPARSER_PARSE, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   ///PD_TRACE_DECLARE_FUNCTION ( SDB__MTHLOWERPARSER_PARSE, "_mthLowerParser::parse" )
   INT32 _mthLowerParser::parse( const bson::BSONElement &e,
                                 _mthSAction &action ) const
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY(SDB__MTHLOWERPARSER_PARSE ) ;

      if ( e.eoo() )
      {
         PD_LOG( PDERROR, "invalid element" ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      if ( !mthIsNumber1( e ) )
      {
         rc = SDB_INVALIDARG ;
         PD_RC_CHECK( rc, PDERROR, "placeholder must be 1" ) ;
      }

      action.setAttribute( MTH_S_ATTR_PROJECTION ) ;
      action.setFunc( &mthLowerBuild,
                      &mthLowerGet ) ;
      action.setName( _name.c_str() ) ;
   done:
      PD_TRACE_EXITRC( SDB__MTHLOWERPARSER_PARSE, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   ///PD_TRACE_DECLARE_FUNCTION ( SDB__MTHUPPERPARSER_PARSE, "_mthUpperParser::parse" )
   INT32 _mthUpperParser::parse( const bson::BSONElement &e,
                                 _mthSAction &action ) const
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY(SDB__MTHUPPERPARSER_PARSE ) ;

      if ( e.eoo() )
      {
         PD_LOG( PDERROR, "invalid element" ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      if ( !mthIsNumber1( e ) )
      {
         rc = SDB_INVALIDARG ;
         PD_RC_CHECK( rc, PDERROR, "placeholder must be 1" ) ;
      }

      action.setAttribute( MTH_S_ATTR_PROJECTION ) ;
      action.setFunc( &mthUpperBuild,
                      &mthUpperGet ) ;
      action.setName( _name.c_str() ) ;
   done:
      PD_TRACE_EXITRC( SDB__MTHUPPERPARSER_PARSE, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   ///PD_TRACE_DECLARE_FUNCTION ( SDB__MTHTRIMPARSER_PARSE, "_mthTrimParser::parse" )
   INT32 _mthTrimParser::parse( const bson::BSONElement &e,
                                _mthSAction &action ) const
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY(SDB__MTHTRIMPARSER_PARSE ) ;

      if ( e.eoo() )
      {
         PD_LOG( PDERROR, "invalid element" ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      if ( !mthIsNumber1( e ) )
      {
         rc = SDB_INVALIDARG ;
         PD_RC_CHECK( rc, PDERROR, "placeholder must be 1" ) ;
      }

      action.setAttribute( MTH_S_ATTR_PROJECTION ) ;
      action.setFunc( &mthTrimBuild,
                      &mthTrimGet ) ;
      action.setName( _name.c_str() ) ;
   done:
      PD_TRACE_EXITRC( SDB__MTHTRIMPARSER_PARSE, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   ///PD_TRACE_DECLARE_FUNCTION ( SDB__MTHLTRIMPARSER_PARSE, "_mthLTrimParser::parse" )
   INT32 _mthLTrimParser::parse( const bson::BSONElement &e,
                                 _mthSAction &action ) const
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY(SDB__MTHLTRIMPARSER_PARSE ) ;

      if ( e.eoo() )
      {
         PD_LOG( PDERROR, "invalid element" ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      if ( !mthIsNumber1( e ) )
      {
         rc = SDB_INVALIDARG ;
         PD_RC_CHECK( rc, PDERROR, "placeholder must be 1" ) ;
      }

      action.setAttribute( MTH_S_ATTR_PROJECTION ) ;
      action.setFunc( &mthLTrimBuild,
                      &mthLTrimGet ) ;
      action.setName( _name.c_str() ) ;
   done:
      PD_TRACE_EXITRC( SDB__MTHLTRIMPARSER_PARSE, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   ///PD_TRACE_DECLARE_FUNCTION ( SDB__MTHRTRIMPARSER_PARSE, "_mthRTrimParser::parse" )
   INT32 _mthRTrimParser::parse( const bson::BSONElement &e,
                                 _mthSAction &action ) const
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY(SDB__MTHRTRIMPARSER_PARSE ) ;

      if ( e.eoo() )
      {
         PD_LOG( PDERROR, "invalid element" ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      if ( !mthIsNumber1( e ) )
      {
         rc = SDB_INVALIDARG ;
         PD_RC_CHECK( rc, PDERROR, "placeholder must be 1" ) ;
      }

      action.setAttribute( MTH_S_ATTR_PROJECTION ) ;
      action.setFunc( &mthRTrimBuild,
                      &mthRTrimGet ) ;
      action.setName( _name.c_str() ) ;
   done:
      PD_TRACE_EXITRC( SDB__MTHRTRIMPARSER_PARSE, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   ///PD_TRACE_DECLARE_FUNCTION ( SDB__MTHDAYPARSER_PARSE, "_mthDayParser::parse" )
   INT32 _mthDayParser::parse( const bson::BSONElement &e,
                               _mthSAction &action ) const
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__MTHDAYPARSER_PARSE ) ;

      if ( e.eoo() )
      {
         PD_LOG( PDERROR, "invalid element" ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      if ( !mthIsNumber1( e ) )
      {
         rc = SDB_INVALIDARG ;
         PD_RC_CHECK( rc, PDERROR, "placeholder must be 1" ) ;
      }

      action.setAttribute( MTH_S_ATTR_PROJECTION ) ;
      action.setFunc( &mthDayBuild, &mthDayGet ) ;
      action.setName( _name.c_str() ) ;
   done:
      PD_TRACE_EXITRC( SDB__MTHDAYPARSER_PARSE, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   ///PD_TRACE_DECLARE_FUNCTION ( SDB__MTHMONTHPARSER_PARSE, "_mthMonthParser::parse" )
   INT32 _mthMonthParser::parse( const bson::BSONElement &e,
                                 _mthSAction &action ) const
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__MTHMONTHPARSER_PARSE ) ;

      if ( e.eoo() )
      {
         PD_LOG( PDERROR, "invalid element" ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      if ( !mthIsNumber1( e ) )
      {
         rc = SDB_INVALIDARG ;
         PD_RC_CHECK( rc, PDERROR, "placeholder must be 1" ) ;
      }

      action.setAttribute( MTH_S_ATTR_PROJECTION ) ;
      action.setFunc( &mthMonthBuild, &mthMonthGet ) ;
      action.setName( _name.c_str() ) ;
   done:
      PD_TRACE_EXITRC( SDB__MTHMONTHPARSER_PARSE, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   ///PD_TRACE_DECLARE_FUNCTION ( SDB__MTHYEARPARSER_PARSE, "_mthYearParser::parse" )
   INT32 _mthYearParser::parse( const bson::BSONElement &e,
                                 _mthSAction &action ) const
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__MTHYEARPARSER_PARSE ) ;

      if ( e.eoo() )
      {
         PD_LOG( PDERROR, "invalid element" ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      if ( !mthIsNumber1( e ) )
      {
         rc = SDB_INVALIDARG ;
         PD_RC_CHECK( rc, PDERROR, "placeholder must be 1" ) ;
      }

      action.setAttribute( MTH_S_ATTR_PROJECTION ) ;
      action.setFunc( &mthYearBuild, &mthYearGet ) ;
      action.setName( _name.c_str() ) ;
   done:
      PD_TRACE_EXITRC( SDB__MTHYEARPARSER_PARSE, rc ) ;
      return rc ;
   error:
      goto done ;
   }
}

