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

   Source File Name = mthCastParser.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          15/01/2015  YW  Initial Draft

   Last Changed =

*******************************************************************************/
#include "mthCastParser.hpp"
#include "pd.hpp"
#include "pdTrace.hpp"
#include "mthTrace.hpp"
#include "mthSActionFunc.hpp"
#include "utilString.hpp"

namespace engine
{
   _mthCastParser::_mthCastParser()
   {
      _name = MTH_S_CAST ;

      _tl["minkey"] = MinKey ;
      _tl["double"] = NumberDouble ;
      _tl["string"] = String ;
      _tl["object"] = Object ;
      _tl["array"] = Array ;
      _tl["bindata"] = BinData ;
      _tl["oid"] = jstOID ;
      _tl["bool"] = Bool ;
      _tl["date"] = Date ;
      _tl["null"] = jstNULL ;
      _tl["int32"] = NumberInt ;
      _tl["timestamp"] = Timestamp ;
      _tl["int64"] = NumberLong ;
      _tl["decimal"] = NumberDecimal ;
      _tl["maxkey"] = MaxKey ;
   }

   ///PD_TRACE_DECLARE_FUNCTION ( SDB__MTHCASTPARSER_PARSE, "_mthCastParser::parse" )
   INT32 _mthCastParser::parse( const bson::BSONElement &e,
                                _mthSAction &action ) const
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__MTHCASTPARSER_PARSE ) ;
      BSONType castType = EOO ;
#if defined (_DEBUG)
      if ( 0 != _name.compare( e.fieldName() ) )
      {
         PD_LOG( PDERROR, "field name[%s] is not valid",
                 e.fieldName() ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }
#endif

      if ( String == e.type() )
      {
         rc = _getCastType( e.valuestr(), castType ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "invalid cast type:%s", e.valuestr () ) ;
            goto error ;
         } 
      }
      else if ( !e.isNumber() )
      {
         PD_LOG( PDERROR, "invalid cast type:%s",
                 e.toString( TRUE, TRUE ).c_str() ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      else
      {
         castType = ( BSONType )( e.numberInt() ) ;
      }

      switch (castType)
      {
      case MinKey :
         break ;
      case EOO :
         rc = SDB_INVALIDARG ;
         break ;
      case NumberDouble :
      case String :
      case Object :
         break ;
      case Array :
      case BinData :
      case Undefined :
         rc = SDB_INVALIDARG ;
         break ;
      case jstOID :
      case Bool :
      case Date :
      case jstNULL :
         break ;
      case RegEx :
      case DBRef :
      case Code :
      case Symbol :
      case CodeWScope :
         rc = SDB_INVALIDARG ;
         break ;
      case NumberInt :
      case Timestamp :
      case NumberLong :
      case NumberDecimal :
      case MaxKey :
         break ;
      default:
         rc = SDB_INVALIDARG ;
         break ;
      }

      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "unknown cast type:%d", castType ) ;
         goto error ;
      }

      action.setAttribute( MTH_S_ATTR_PROJECTION ) ;
      action.setFunc( &mthCastBuild,
                      &mthCastGet ) ;
      action.setName( _name.c_str() ) ;
      action.setArg( BSON( "arg1" << castType ) ) ;
   done:
      PD_TRACE_EXITRC( SDB__MTHCASTPARSER_PARSE, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   INT32 _mthCastParser::_getCastType( const CHAR *str,
                                       BSONType &type ) const
   {
      INT32 rc = SDB_OK ;
      TYPE_LIST::const_iterator itr ;
      BSONType t = bson::EOO ;
      utilString us ;
      const CHAR *p = str ;
      while ( '\0' != *p )
      {
         if ( 'A' <= *p &&
              *p <= 'Z' )
         {
            rc = us.append( *p + 32 ) ;
         }
         else
         {
             rc = us.append( *p ) ;
         }

         ++p ;
      }

      itr = _tl.find( us.str() ) ;
      if ( _tl.end() != itr )
      {
         t = ( BSONType )( itr->second ) ;
      }
      else
      {
         PD_LOG( PDERROR, "unknown type:%s, check your input or"
                 "use bsontype", str ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      type = t ;
   done:
      return rc ;
   error:
      goto done ;
   }
}

