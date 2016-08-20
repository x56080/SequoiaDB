/******************************************************************************

   Copyright (C) 2011-2014 SequoiaDB Ltd.

   This program is free software: you can redistribute it and/or modify
   it under the term of the GNU Affero General Public License, version 3,
   as published by the Free Software Foundation.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warrenty of
   MARCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program. If not, see <http://www.gnu.org/license/>.

   Source File Name = mthSActionFunc.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          15/01/2015  YW  Initial Draft

   Last Changed =

*******************************************************************************/

#include "mthSActionFunc.hpp"
#include "pd.hpp"
#include "pdTrace.hpp"
#include "mthTrace.hpp"
#include "mthSAction.hpp"
#include "mthSliceIterator.hpp"
#include "mthElemMatchIterator.hpp"
#include "mthMatcher.hpp"
#include "utilString.hpp"
#include "utilStr.hpp"
#include "../util/fromjson.hpp"


using namespace bson ;

namespace engine
{
   ///PD_TRACE_DECLARE_FUNCTION ( SDB__MTHINCLUDEBUILD, "mthIncludeBuild" )
   INT32 mthIncludeBuild( const CHAR *fieldName,
                          const bson::BSONElement &e,
                          _mthSAction *action,
                          bson::BSONObjBuilder &builder )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__MTHINCLUDEBUILD ) ;
      SDB_ASSERT( NULL != action, "can not be null" ) ;
      if ( !e.eoo() )
      {
         builder.append( e ) ;
      }
      PD_TRACE_EXITRC( SDB__MTHINCLUDEBUILD, rc ) ;
      return rc ;
   }

   ///PD_TRACE_DECLARE_FUNCTION ( SDB__MTHINCLUDEGET, "mthIncludeGet" )
   INT32 mthIncludeGet( const CHAR *fieldName,
                        const bson::BSONElement &in,
                        _mthSAction *action,
                        bson::BSONElement &out )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__MTHINCLUDEGET ) ;
      SDB_ASSERT( NULL != action, "can not be null" ) ;
      out = in ;
      PD_TRACE_EXITRC( SDB__MTHINCLUDEGET, rc ) ;
      return rc ;
   }
 
   ///PD_TRACE_DECLARE_FUNCTION ( SDB__MTHDEFAULTBUILD, "mthDefaultBuild" )
   INT32 mthDefaultBuild( const CHAR *fieldName,
                          const bson::BSONElement &e,
                          _mthSAction *action,
                          bson::BSONObjBuilder &builder )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__MTHDEFAULTBUILD ) ;
      SDB_ASSERT( NULL != action, "can not be null" ) ;
      if ( e.eoo() )
      {
         builder.appendAs( action->getValue(), fieldName ) ;
      }
      else
      {
         builder.append( e ) ;
      }
      PD_TRACE_EXITRC( SDB__MTHDEFAULTBUILD, rc ) ;
      return rc ;
   }

   ///PD_TRACE_DECLARE_FUNCTION ( SDB__MTHDEFAULTGET, "mthDefaultGet" )
   INT32 mthDefaultGet( const CHAR *fieldName,
                        const bson::BSONElement &in,
                        _mthSAction *action,
                        bson::BSONElement &out )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__MTHDEFAULTGET ) ;
      SDB_ASSERT( NULL != action, "can not be null" ) ;
      if ( !in.eoo() )
      {
         out = in ;
         goto done ;
      }

      if ( action->getObj().isEmpty() )
      {
         bson::BSONObjBuilder builder ;
         builder.appendAs( action->getValue(), fieldName ) ;
         bson::BSONObj obj = builder.obj() ;
         action->setObj( obj ) ;
         action->setValue( obj.getField( fieldName ) ) ;
      }

      out = action->getValue() ;
   done:
      PD_TRACE_EXITRC( SDB__MTHDEFAULTGET, rc ) ;
      return rc ;
   }

   ///PD_TRACE_DECLARE_FUNCTION ( SDB__MTHSLICEBUILD, "mthSliceBuild" )
   INT32 mthSliceBuild( const CHAR *fieldName,
                        const bson::BSONElement &e,
                        _mthSAction *action,
                        bson::BSONObjBuilder &builder )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__MTHSLICEBUILD ) ;
      SDB_ASSERT( NULL != action, "can not be null" ) ;

      if ( e.eoo() )
      {
         goto done ;
      }
      else if ( Array == e.type() )
      {
         BSONObj args = action->getArg() ;
         _mthSliceIterator i( e.embeddedObject(),
                              args.getIntField( "arg1" ),
                              args.getIntField("arg2") ) ;
         BSONArrayBuilder sliceBuilder( builder.subarrayStart( fieldName ) ) ;
         while ( i.more() )
         {
            sliceBuilder.append( i.next() ) ;
         }
         sliceBuilder.doneFast() ;
      }
      else
      {
         builder.append( e ) ;
      }
   done:
      PD_TRACE_EXITRC( SDB__MTHSLICEBUILD, rc ) ;
      return rc ;
   }

   ///PD_TRACE_DECLARE_FUNCTION ( SDB__MTHSLICEGET, "mthSliceGet" )
   INT32 mthSliceGet( const CHAR *fieldName,
                      const bson::BSONElement &in,
                      _mthSAction *action,
                      bson::BSONElement &out )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__MTHSLICEGET ) ;
      SDB_ASSERT( NULL != action, "can not be null" ) ;

      if ( Array != in.type() )
      {
         out = in ;
         goto done ; 
      }
      else if ( Array == in.type() )
      {
         BSONObjBuilder subBuilder ;
         BSONObj args = action->getArg() ;
         _mthSliceIterator i( in.embeddedObject(), 
                              args.getIntField( "arg1" ),
                              args.getIntField( "arg2" ) ) ;
         BSONArrayBuilder sliceBuilder( subBuilder.subarrayStart( fieldName ) ) ;
         while ( i.more() )
         {
            sliceBuilder.append( i.next() ) ;
         }
         sliceBuilder.doneFast() ;
         action->setObj( subBuilder.obj() ) ;
         out = action->getObj().getField( fieldName ) ;
      }
   done:
      PD_TRACE_EXITRC( SDB__MTHSLICEGET, rc ) ;
      return rc ;
   }

   ///PD_TRACE_DECLARE_FUNCTION ( SDB__MTHELEMMATCHBUILDN, "mthElemMatchBuildN" )
   static INT32 mthElemMatchBuildN( const CHAR *fieldName,
                                    const bson::BSONElement &e,
                                    _mthSAction *action,
                                    bson::BSONObjBuilder &builder,
                                    INT32 n )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__MTHELEMMATCHBUILDN ) ;
      if ( Array == e.type() )
      {
         BSONArrayBuilder arrayBuilder( builder.subarrayStart( fieldName ) ) ;
         _mthElemMatchIterator i( e.embeddedObject(),
                                  &( action->getMatcher() ),
                                  n ) ;
         do
         {
            BSONElement next ;
            rc = i.next( next ) ;
            if ( SDB_OK == rc )
            {
               arrayBuilder.append( next ) ;    
            }
            else if ( SDB_DMS_EOC == rc )
            {
               arrayBuilder.doneFast() ;
               rc = SDB_OK ;
               break ;
            }
            else
            {
               PD_LOG( PDERROR, "failed to get next element:%d", rc ) ;
               goto error ;
            }
         } while ( TRUE ) ;
      }
   done:
      PD_TRACE_EXITRC( SDB__MTHELEMMATCHBUILDN, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   ///PD_TRACE_DECLARE_FUNCTION ( SDB__MTHELEMMATCHGETN, "mthElemMatchGetN" )
   static INT32 mthElemMatchGetN( const CHAR *fieldName,
                                  const bson::BSONElement &in,
                                  _mthSAction *action,
                                  bson::BSONElement &out,
                                  INT32 n )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__MTHELEMMATCHGETN ) ;
      if ( Array == in.type() )
      {
         BSONObjBuilder objBuilder ;
         BSONArrayBuilder arrayBuilder( objBuilder.subarrayStart( fieldName ) ) ;
         _mthElemMatchIterator i( in.embeddedObject(),
                                  &( action->getMatcher() ),
                                  n ) ;
         do
         {
            BSONElement next ;
            rc = i.next( next ) ;
            if ( SDB_OK == rc )
            {
               arrayBuilder.append( next ) ;
            }
            else if ( SDB_DMS_EOC == rc )
            {
               arrayBuilder.doneFast() ;
               rc = SDB_OK ;
               break ;
            }
            else
            {
               PD_LOG( PDERROR, "failed to get next element:%d", rc ) ;
               goto error ;
            }
         } while ( TRUE ) ;

         action->setObj( objBuilder.obj() ) ;
         out = action->getObj().getField( fieldName ) ;
      }
      else
      {
         out = BSONElement() ;
      }
   done:
      PD_TRACE_EXITRC( SDB__MTHELEMMATCHGETN, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   ///PD_TRACE_DECLARE_FUNCTION ( SDB__MTHELEMMATCHBUILD, "mthElemMatchBuild" )
   INT32 mthElemMatchBuild( const CHAR *fieldName,
                            const bson::BSONElement &e,
                            _mthSAction *action,
                            bson::BSONObjBuilder &builder )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__MTHELEMMATCHBUILD ) ;
      rc = mthElemMatchBuildN( fieldName, e, action, builder, -1 ) ;
      PD_TRACE_EXITRC( SDB__MTHELEMMATCHBUILD, rc ) ;
      return rc ;
   }

   ///PD_TRACE_DECLARE_FUNCTION ( SDB__MTHELEMMATCHGET, "mthElemMatchGet" )
   INT32 mthElemMatchGet( const CHAR *fieldName,
                          const bson::BSONElement &in,
                          _mthSAction *action,
                          bson::BSONElement &out )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__MTHELEMMATCHGET ) ;
      rc = mthElemMatchGetN( fieldName, in, action, out, -1 ) ;
      PD_TRACE_EXITRC( SDB__MTHELEMMATCHGET, rc ) ;
      return rc ;
   }

   ///PD_TRACE_DECLARE_FUNCTION ( SDB__MTHELEMMATCHONEBUILD, "mthElemMatchOneBuild" )
   INT32 mthElemMatchOneBuild( const CHAR *fieldName,
                               const bson::BSONElement &e,
                               _mthSAction *action,
                               bson::BSONObjBuilder &builder )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__MTHELEMMATCHONEBUILD ) ;
      rc = mthElemMatchBuildN( fieldName, e, action, builder, 1 ) ;
      PD_TRACE_EXITRC( SDB__MTHELEMMATCHONEBUILD, rc ) ;
      return rc ;
   }

   ///PD_TRACE_DECLARE_FUNCTION ( SDB__MTHELEMMATCHONEGET, "mthElemMatchOneGet" )
   INT32 mthElemMatchOneGet( const CHAR *fieldName,
                             const bson::BSONElement &in,
                             _mthSAction *action,
                             bson::BSONElement &out )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__MTHELEMMATCHONEGET ) ;
      rc = mthElemMatchGetN( fieldName, in, action, out, 1 ) ;
      PD_TRACE_EXITRC( SDB__MTHELEMMATCHONEGET, rc ) ;
      return rc ;
   }

   ///PD_TRACE_DECLARE_FUNCTION ( SDB__MTHABSBUILD, "mthAbsBuild" )
   INT32 mthAbsBuild( const CHAR *fieldName,
                      const bson::BSONElement &e,
                      _mthSAction *action,
                      bson::BSONObjBuilder &builder )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__MTHABSBUILD ) ;
      SDB_ASSERT( NULL != action, "can not be null" ) ;
      if ( NumberDouble == e.type() )
      {
         builder.append( fieldName, fabs( e.Double() ) ) ;
      }
      else if ( NumberInt == e.type() )
      {
         INT32 v = e.numberInt() ;
         /// - 2 ^ 31
         if ( -2147483648 != v )
         {
            builder.append( fieldName, 0 <= v ? v : -v ) ;
         }
         else
         {
            builder.append( fieldName, -((INT64)v) ) ;
         }
      }
      else if ( NumberLong == e.type() )
      {
         INT64 v = e.numberLong() ;
         /// return -9223372036854775808 when v is -9223372036854775808
         builder.append( fieldName, 0 <= v ? ( INT64 )v : ( INT64 )( -v ) ) ;
      }
      else if ( NumberDecimal == e.type() )
      {
         bsonDecimal decimal ;
         decimal = e.numberDecimal() ;
         rc = decimal.abs() ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to ceil decimal:%s,rc=%d", 
                    decimal.toString().c_str(), rc ) ;
            goto error ;
         }
         builder.append( fieldName, decimal ) ;
      }
      else if ( !e.eoo() )
      {
         builder.appendNull( fieldName ) ;
      }
      else
      {
         /// do nothing.
      }
   done:
      PD_TRACE_EXITRC( SDB__MTHABSBUILD, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   ///PD_TRACE_DECLARE_FUNCTION ( SDB__MTHABSGET, "mthAbsGet" )
   INT32 mthAbsGet( const CHAR *fieldName,
                    const bson::BSONElement &in,
                    _mthSAction *action,
                    bson::BSONElement &out )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__MTHABSGET ) ;
      SDB_ASSERT( NULL != action, "can not be null" ) ;
      BSONObjBuilder builder ;
      BSONObj obj ;
      if ( NumberDouble == in.type() )
      {
         builder.append( fieldName, fabs( in.Double() ) ) ;
         obj = builder.obj() ;
      }
      else if ( NumberInt == in.type() )
      {
         INT32 v = 0 ;
         if ( 0 <= in.Int() )
         {
            out = in ;
            goto done ;
         }

         v = in.numberInt() ;
         if ( -2147483648 != v )
         {
            builder.append( fieldName, ( INT32 )( -v ) ) ;
         }
         else
         {
            builder.append( fieldName, -(( INT64 )v) ) ;
         }
         obj = builder.obj() ;
      }
      else if ( NumberLong == in.type() )
      {
         if ( 0 <= in.Long() )
         {
            out = in ;
            goto done ;
         }
         builder.append( fieldName, ( INT64 )( -( in.Long() ) ) ) ;
         obj = builder.obj() ;
      }
      else if ( NumberDecimal == in.type() )
      {
         bsonDecimal decimal ;
         decimal = in.numberDecimal() ;
         rc = decimal.abs() ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to abs decimal:%s,rc=%d", 
                    decimal.toString().c_str(), rc ) ;
            goto error ;
         }
         builder.append( fieldName, decimal ) ;
         obj = builder.obj() ;
      }
      else if ( !in.eoo() )
      {
         builder.appendNull( fieldName ) ;
         obj = builder.obj() ;
      }
      else
      {
         /// do nothing
      }

      if ( !obj.isEmpty() )
      {
         action->setObj( obj ) ;
         out = action->getObj().getField( fieldName ) ;
      }
   done:
      PD_TRACE_EXITRC( SDB__MTHABSGET, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   ///PD_TRACE_DECLARE_FUNCTION ( SDB__MTHCEILINGBUILD, "mthCeilingBuild" )
   INT32 mthCeilingBuild( const CHAR *fieldName,
                          const bson::BSONElement &e,
                          _mthSAction *action,
                          bson::BSONObjBuilder &builder )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__MTHABSGET ) ;
      SDB_ASSERT( NULL != action, "can not be null" ) ;
      if ( NumberLong == e.type() )
      {
         builder.append( fieldName,
                         ( INT64 )( e.numberLong() ) ) ;
      }
      else if ( NumberInt == e.type() )
      {
         builder.append( fieldName,
                         ( INT32 )( e.numberInt() ) ) ;
      }
      else if ( NumberDouble == e.type() )
      {
         builder.append( fieldName,
                        ( FLOAT64 )ceil( e.numberDouble() ) ) ;      
      }
      else if ( NumberDecimal == e.type() )
      {
         bsonDecimal decimal ;
         bsonDecimal result ;
         decimal = e.numberDecimal() ;
         result.init() ;

         rc = decimal.ceil( result ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to ceil decimal:%s,rc=%d", 
                    decimal.toString().c_str(), rc ) ;
            goto error ;
         }
         builder.append( fieldName, result ) ;
      }
      else if ( !e.eoo() )
      {
         builder.appendNull( fieldName ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB__MTHCEILINGBUILD, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   ///PD_TRACE_DECLARE_FUNCTION ( SDB__MTHCEILINGGET, "mthCeilingGet" )
   INT32 mthCeilingGet( const CHAR *fieldName,
                        const bson::BSONElement &in,
                        _mthSAction *action,
                        bson::BSONElement &out )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__MTHABSGET ) ;
      SDB_ASSERT( NULL != action, "can not be null" ) ;
      BSONObjBuilder builder ;
      BSONObj obj ;

      if ( NumberLong == in.type() )
      {
         builder.append( fieldName,
                         ( INT64 )( in.numberLong() ) ) ;
         obj = builder.obj() ;
      }
      else if ( NumberInt == in.type() )
      {
         builder.append( fieldName,
                         ( INT32 )( in.numberInt() ) ) ;
         obj = builder.obj() ;
      }
      else if ( NumberDouble == in.type() )
      {
         builder.append( fieldName,
                        ( FLOAT64 )( ceil( in.Number() ) ) ) ;
         obj = builder.obj() ;
      }
      else if ( NumberDecimal == in.type() )
      {
         bsonDecimal decimal ;
         bsonDecimal result ;
         decimal = in.numberDecimal() ;
         result.init() ;

         rc = decimal.ceil( result ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to ceil decimal:%s,rc=%d", 
                    decimal.toString().c_str(), rc ) ;
            goto error ;
         }
         builder.append( fieldName, result ) ;
         obj = builder.obj() ;
      }
      else if ( !in.eoo() )
      {
         builder.appendNull( fieldName ) ;
         obj = builder.obj() ;
      }

      if ( !obj.isEmpty() )
      {
         action->setObj( obj ) ;
         out = action->getObj().getField( fieldName ) ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   ///PD_TRACE_DECLARE_FUNCTION ( SDB__MTHFLOORBUILD, "mthFloorBuild" )
   INT32 mthFloorBuild( const CHAR *fieldName,
                        const bson::BSONElement &e,
                        _mthSAction *action,
                        bson::BSONObjBuilder &builder )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__MTHFLOORBUILD ) ;
      SDB_ASSERT( NULL != action, "can not be null" ) ;
      if ( NumberInt == e.type() )
      {
         builder.append( fieldName,
                        ( INT32 )( e.numberInt() ) ) ;
      }
      else if ( NumberLong == e.type() )
      {
         builder.append( fieldName,
                       ( INT64 )( e.numberLong() ) ) ;
      }
      else if ( NumberDouble == e.type() )
      {
         builder.append( fieldName,
                        ( FLOAT64 )floor( e.numberDouble() ) ) ;
      }
      else if ( NumberDecimal == e.type() )
      {
         bsonDecimal decimal ;
         bsonDecimal result ;
         result.init() ;

         decimal = e.numberDecimal() ;
         rc = decimal.floor( result ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to floor decimal:%s,rc=%d", 
                    decimal.toString().c_str(), rc ) ;
            goto error ;
         }

         builder.append( fieldName, result ) ;
      }
      else if ( !e.eoo() )
      {
         builder.appendNull( fieldName ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB__MTHFLOORBUILD, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   ///PD_TRACE_DECLARE_FUNCTION ( SDB__MTHFLOORGET, "mthFloorGet" )
   INT32 mthFloorGet( const CHAR *fieldName,
                      const bson::BSONElement &in,
                      _mthSAction *action,
                      bson::BSONElement &out )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__MTHFLOORGET ) ;
      SDB_ASSERT( NULL != action, "can not be null" ) ;
      BSONObjBuilder builder ;
      BSONObj obj ;

      if ( NumberInt == in.type() )
      {
         builder.append( fieldName,
                       ( INT32 )( in.numberInt() ) ) ;
         obj = builder.obj() ;
      }
      else if ( NumberLong == in.type() )
      {
         builder.append( fieldName,
                        ( INT64 )( in.numberLong() ) ) ;
         obj = builder.obj() ;
      }
      else if ( NumberDouble == in.type() )
      {
         builder.append( fieldName,
                        ( FLOAT64 )floor( in.numberDouble() ) ) ;
         obj = builder.obj() ;
      }
      else if ( NumberDecimal == in.type() )
      {
         bsonDecimal decimal ;
         bsonDecimal result ;
         result.init() ;

         decimal = in.numberDecimal() ;
         rc = decimal.floor( result ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to floor decimal:%s,rc=%d", 
                    decimal.toString().c_str(), rc ) ;
            goto error ;
         }

         builder.append( fieldName, result ) ;
         obj = builder.obj() ;
      }
      else if ( !in.eoo() )
      {
         builder.appendNull( fieldName ) ;
         obj = builder.obj() ;
      }

      if ( !obj.isEmpty() )
      {
         action->setObj( obj ) ;
         out = action->getObj().getField( fieldName ) ;
      }
   done:
      PD_TRACE_EXITRC( SDB__MTHFLOORGET, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   ///PD_TRACE_DECLARE_FUNCTION ( SDB__MTHMODBUILD, "mthModBuild" )
   INT32 mthModBuild( const CHAR *fieldName,
                      const bson::BSONElement &e,
                      _mthSAction *action,
                      bson::BSONObjBuilder &builder )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__MTHMODBUILD ) ;
      SDB_ASSERT( NULL != action, "can not be null" ) ;
      const BSONObj &obj = action->getArg() ;
      BSONElement arg = obj.getField( "arg1" ) ;
      if ( e.eoo() )
      {
         /// do nothing.
      }
      else if ( !e.isNumber() ||
                !arg.isNumber() )
      {
         builder.appendNull( fieldName ) ;
      }
      else if ( NumberDecimal == e.type() || 
                NumberDecimal == arg.type() ) 
      {
         bsonDecimal decimal ;
         bsonDecimal decimalArg ;
         bsonDecimal result ;
         result.init() ;

         decimal   = e.numberDecimal() ;
         decimalArg = arg.numberDecimal() ;
         rc = decimal.mod( decimalArg, result ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to mod decimal:%s mod %s,rc=%d", 
                    decimal.toString().c_str(), 
                    decimalArg.toString().c_str(), rc ) ;
            goto error ;
         }

         builder.append( fieldName, result ) ;
      }
      else if ( FALSE == isModValid( arg ) )
      {
         builder.appendNull( fieldName ) ;
      }
      else if ( NumberDouble == e.type() &&
                NumberDouble == arg.type() )
      {
         FLOAT64 v = MTH_MOD( e.numberDouble(),
                              arg.numberDouble() ) ;
         builder.append( fieldName, v ) ;
      }
      else if ( NumberDouble != e.type () &&
                NumberDouble == arg.type() )
      {
         FLOAT64 v = MTH_MOD( e.numberLong(),
                              arg.numberDouble() ) ;
         builder.append( fieldName, v ) ;
      }
      else if ( NumberDouble == e.type () &&
                NumberDouble != arg.type() )
      {
         FLOAT64 v = MTH_MOD( e.numberDouble(),
                              arg.numberLong() ) ;
         builder.append( fieldName, v ) ;
      }
      else
      {
         INT64 v = e.numberLong() % arg.numberLong() ;
         builder.appendNumber( fieldName, v ) ;
      }
   done:
      PD_TRACE_EXITRC( SDB__MTHMODBUILD, rc ) ;
      return rc ;
   error:
      goto done ;
   }

    ///PD_TRACE_DECLARE_FUNCTION ( SDB__MTHMODGET, "mthModGet" )
   INT32 mthModGet( const CHAR *fieldName,
                    const bson::BSONElement &in,
                    _mthSAction *action,
                    bson::BSONElement &out )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__MTHMODBUILD ) ;
      SDB_ASSERT( NULL != action, "can not be null" ) ;
      BSONObjBuilder builder ;
      BSONObj obj ;
      const BSONObj &arg = action->getArg() ;
      BSONElement argEle = arg.getField( "arg1" ) ;
      if ( in.eoo() )
      {
         /// do nothing.
      }
      else if ( !in.isNumber() ||
                !argEle.isNumber() )
      {
         builder.appendNull( fieldName ) ;
      }
      else if ( NumberDecimal == in.type() || 
                NumberDecimal == argEle.type() ) 
      {
         bsonDecimal decimal ;
         bsonDecimal decimalArg ;
         bsonDecimal result ;
         result.init() ;

         decimal   = in.numberDecimal() ;
         decimalArg = argEle.numberDecimal() ;
         rc = decimal.mod( decimalArg, result ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to mod decimal:%s mod %s,rc=%d", 
                    decimal.toString().c_str(), 
                    decimalArg.toString().c_str(), rc ) ;
            goto error ;
         }

         builder.append( fieldName, result ) ;
      }
      else if ( FALSE == isModValid( argEle ) )
      {
         builder.appendNull( fieldName ) ;
      }
      else if ( NumberDouble == in.type() &&
                NumberDouble == argEle.type() )
      {
         FLOAT64 v = MTH_MOD( in.numberDouble(),
                              argEle.numberDouble() ) ;
         builder.append( fieldName, v ) ;
      }
      else if ( NumberDouble != in.type () &&
                NumberDouble == argEle.type() )
      {
         FLOAT64 v = MTH_MOD( in.numberLong(),
                              argEle.numberDouble() ) ;
         builder.append( fieldName, v ) ;
      }
      else if ( NumberDouble == in.type () &&
                NumberDouble != argEle.type() )
      {
         FLOAT64 v = MTH_MOD( in.numberDouble(),
                              argEle.numberLong() ) ;
         builder.append( fieldName, v ) ;
      }
      else
      {
         INT64 v = in.numberLong() % argEle.numberLong() ;
         builder.append( fieldName, v ) ;
      }

      obj = builder.obj() ;
      if ( !obj.isEmpty() )
      {
         action->setObj( obj ) ;
         out = action->getObj().getField( fieldName ) ; 
      }
   done:
      PD_TRACE_EXITRC( SDB__MTHMODBUILD, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   static INT32 _mthCast( const CHAR *fieldName,
                         const bson::BSONElement &e,
                         BSONType type,
                         BSONObjBuilder &builder )
   {
      INT32 rc = SDB_OK ;
      SDB_ASSERT( e.type() != type, "should not be same" ) ;
      switch ( type )
      {
      case MinKey :
         builder.appendMinKey( fieldName ) ;
         break ;
      case EOO :
         rc = SDB_INVALIDARG ;
         break ;
      case NumberDouble :
      {
         if ( Bool == e.type() )
         {
            FLOAT64 f = e.Bool() ? 1.0 : 0.0 ;
            builder.appendNumber( fieldName, f ) ;
         }
         else if ( String != e.type() )
         {
            FLOAT64 f = e.numberDouble() ;
            if ( isInf( f ) )
            {
               f = 0.0 ;
            }
            builder.appendNumber( fieldName, f ) ;
         }
         else
         {
            try
            {
               FLOAT64 f = 0.0 ;
               f = boost::lexical_cast<FLOAT64>( e.valuestr () ) ;
               builder.appendNumber( fieldName, f ) ;
            }
            catch ( boost::bad_lexical_cast &e )
            {
               builder.appendNumber( fieldName, 0.0 ) ;
            }
         }
         break ;
      }
      case String :
      {
         if ( NumberInt == e.type() )
         {
            utilString us ;
            rc = us.appendINT32( e.numberInt() ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "failed to append int32:%d", rc ) ;
               goto error ;
            }
            builder.append( fieldName, us.str() ) ;
         }
         else if ( NumberLong == e.type() )
         {
            utilString us ;
            rc = us.appendINT64( e.numberLong() ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "failed to append int64:%d", rc ) ;
               goto error ;
            }
            builder.append( fieldName, us.str() ) ;
         }
         else if ( NumberDouble == e.type() )
         {
            utilString us ;
            rc = us.appendDouble( e.numberDouble() ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "failed to append float64:%d", rc ) ;
               goto error ;
            }
            builder.append( fieldName, us.str() ) ;
         }
         else if ( NumberDecimal == e.type() )
         {
            utilString us ;
            bsonDecimal decimal ;
            string value ;
            decimal.init() ;

            decimal = e.numberDecimal() ;
            value   = decimal.toString() ;
            rc      = us.append( value.c_str(), value.length() );
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "failed to append decimal=%s,rc=%d", 
                       value.c_str(), rc ) ;
               goto error ;
            }
            builder.append( fieldName, us.str() ) ;
         }
         else if ( Date == e.type() )
         {
            CHAR buffer[64] = { 0 };
            time_t timer = (time_t)( ( INT64 )( e.date() ) / 1000 ) ;
            struct tm psr ;
            local_time ( &timer, &psr ) ;
            sprintf ( buffer,
                      "%04d-%02d-%02d",
                      psr.tm_year + 1900,
                      psr.tm_mon + 1,
                      psr.tm_mday ) ;
            builder.append( fieldName, buffer ) ;
         }
         else if ( Timestamp == e.type() )
         {
            Date_t date = e.timestampTime () ;
            unsigned int inc = e.timestampInc () ;
            char buffer[128] = { 0 };
            time_t timer = (time_t)((( INT64 )(date.millis))/1000) ;
            struct tm psr ;
            local_time ( &timer, &psr ) ;
            sprintf ( buffer,
                      "%04d-%02d-%02d-%02d.%02d.%02d.%06d",
                      psr.tm_year + 1900,
                      psr.tm_mon + 1,
                      psr.tm_mday,
                      psr.tm_hour,
                      psr.tm_min,
                      psr.tm_sec,
                      inc ) ;
            builder.append( fieldName, buffer ) ;
         }
         else if ( jstOID == e.type() )
         {
            builder.append( fieldName, e.OID().str() ) ;
         }
         else if ( Object == e.type() )
         {
            builder.append( fieldName,
                            e.embeddedObject().toString( FALSE, TRUE ) ) ; 
         }
         else if ( Array == e.type() )
         {
            builder.append( fieldName,
                            e.embeddedObject().toString( TRUE, TRUE ) ) ;
         }
         else if ( Bool == e.type() )
         {
            builder.append( fieldName,
                            e.booleanSafe() ?
                            "true" : "false" ) ;
         }
         else
         {
            builder.appendNull( fieldName ) ;
         }
         break ;
      }   
      case Object :
      {
         if ( String == e.type() )
         {
            BSONObj obj ;
            INT32 r = fromjson( e.valuestr(), obj ) ;
            if ( SDB_OK == r )
            {
               builder.append( fieldName, obj ) ;
            }
            else
            {
               builder.appendNull( fieldName ) ;
            }
         }
         else
         {
            builder.appendNull( fieldName ) ;
         }
         break ;
      }
      case Array :
      case BinData :
      case Undefined :
         builder.appendNull( fieldName ) ;
         break ;
      case jstOID :
      {
         if ( String == e.type() &&
              25 == e.valuestrsize() )
         {
            bson::OID o( e.valuestr() ) ;
            builder.appendOID( fieldName, &o ) ;
         }
         else
         {
            builder.appendNull( fieldName ) ;
         }
         break ;
      }
      case Bool :
         builder.appendBool( fieldName, e.trueValue() ) ;
         break ;
      case Date :
      {
         UINT64 tm = 0 ;
         if ( e.isNumber() )
         {
            Date_t d( e.numberLong() ) ;
            builder.appendDate( fieldName, d ) ;
         }
         else if ( String == e.type() &&
                   SDB_OK == utilStr2Date( e.valuestr(), tm ))
         {
            builder.appendDate( fieldName, Date_t( tm ) ) ;
         }
         else if ( Timestamp == e.type() )
         {
            builder.appendDate( fieldName, e.timestampTime() ) ;
         }
         else
         {
            builder.appendNull( fieldName ) ;
         }
         break ;
      }
      case jstNULL :
      case RegEx :
      case DBRef :
      case Code :
      case Symbol :
      case CodeWScope :
         builder.appendNull( fieldName ) ;
         break ;
      case NumberInt :
      {
         if ( Date == e.type() )
         {
            builder.appendNumber( fieldName,
                                  ( INT32 )( e.date().millis ) ) ;
         }
         else if ( Timestamp == e.type() )
         {
            UINT64 l = e.timestampTime().millis ;
            l += e.timestampInc() / 1000 ;
            builder.appendNumber( fieldName, ( INT32 )l ) ;
         }
         else if ( Bool == e.type() )
         {
            INT32 v = e.Bool() ? 1 : 0 ;
            builder.append( fieldName, v ) ;
         }
         else if ( NumberLong == e.type() )
         {
            INT32 i = 0 ;
            INT64 l = e.numberLong() ;
            if ( l > 2147483647LL || l < -2147483648LL )
            {
               i = 0 ;
            }
            else
            {
               i = ( INT32 )l ;
            }
            builder.appendNumber( fieldName, i ) ;
         }
         else if ( NumberDecimal == e.type() )
         {
            INT32 i = 0 ;
            INT64 l = 0 ;
            e.numberDecimal().toLong( &l) ;
            if ( l > 2147483647LL || l < -2147483648LL )
            {
               i = 0 ;
            }
            else
            {
               i = ( INT32 )l ;
            }

            builder.appendNumber( fieldName, i ) ;
         }
         else if ( NumberDouble == e.type() )
         {
            INT32 i = 0 ;
            double d = e.Double() ;
            if ( d > 2147483647.0 || d < -2147483648.0 )
            {
               i = 0 ;
            }
            else
            {
               i = ( INT32 )d ;
            }
            builder.appendNumber( fieldName, i ) ;
         }
         else if ( String != e.type() )
         {
            builder.appendNumber( fieldName, e.numberInt() ) ;
         }
         else
         {
            try
            {
               INT32 i = 0 ;
               double v = 0 ;
               v = boost::lexical_cast<double>( e.valuestr () ) ;
               if ( v > 2147483647.0 || v < -2147483648.0 )
               {
                  i = 0 ;
               }
               else
               {
                  i = ( INT32 )v ;
               }
               builder.appendNumber( fieldName, i ) ;
            }
            catch ( boost::bad_lexical_cast &e )
            {
               builder.appendNumber( fieldName, 0 ) ;
            }
         }
         break ;
      }
      case Timestamp :
      {
         time_t tm = 0 ;;
         UINT64 usec = 0 ;
         if ( e.isNumber() )
         {
            /// millis
            OpTime t( (unsigned) (e.numberLong() / 1000) , 0 );
            builder.appendTimestamp( fieldName, t.asDate() ) ;
         }
         else if ( String == e.type() &&
                   SDB_OK == engine::utilStr2TimeT( e.valuestr(),
                                                    tm,
                                                    &usec ))
         {
            OpTime t( (unsigned) (tm) , usec );
            builder.appendTimestamp( fieldName, t.asDate() ) ;
         }
         else if ( Date == e.type() )
         {
            builder.appendTimestamp( fieldName, e.date().millis, 0 ) ;
         }
         else
         {
            builder.appendNull( fieldName ) ;
         }
         break ;
      }
      case NumberLong :
      {
         if ( Date == e.type() )
         {
            builder.appendNumber( fieldName,
                                  ( INT64 )( e.date().millis ) ) ;
         }
         else if ( Timestamp == e.type() )
         {
            UINT64 l = e.timestampTime().millis ;
            l += e.timestampInc() / 1000 ;
            builder.appendNumber( fieldName, ( INT64 )l ) ;
         }
         else if ( Bool == e.type() )
         {
            INT64 v = e.Bool() ? 1 : 0 ;
            builder.append( fieldName, v ) ;
         }
         else if ( NumberDouble == e.type() )
         {
            INT64 l = 0 ;
            double d = e.Double() ;
            if ( d >= 0 && d < 9223372036854775808.0 )
            {
               l = (INT64)d ;
            }
            else if ( d < 0 && d >= -9223372036854775808.0 )
            {
               l = (INT64)d ;
            }
            builder.appendNumber( fieldName, l ) ;
         }
         else if ( String != e.type() )
         {
            builder.appendNumber( fieldName, e.numberLong() ) ;
         }
         else
         {
            try
            {  
               //if the STRING has "." "e" or "E" use double type
               if ( ossStrchr ( e.valuestr (), '.' ) != NULL || 
                    ossStrchr ( e.valuestr (), 'E' ) != NULL || 
                    ossStrchr ( e.valuestr (), 'e' ) != NULL )
               {
                  double d = 0  ;
                  INT64 l = 0 ;
                  d = boost::lexical_cast<double>( e.valuestr () ) ;
                  if ( d >= 0 && d < 9223372036854775808.0 )
                  {
                     l = (INT64)d ;
                  }
                  else if ( d < 0 && d >= -9223372036854775808.0 )
                  {
                     l = (INT64)d ;
                  }
                  builder.appendNumber( fieldName, l ) ;
               }
               else
               {
                  INT64 l = 0 ;
                  l = boost::lexical_cast<INT64>( e.valuestr () ) ;
                  builder.appendNumber( fieldName, l ) ;
               }
            }
            catch ( boost::bad_lexical_cast &e )
            {
               builder.appendNumber( fieldName, 0 ) ;
            }
         }
         break ;
      }
      case NumberDecimal :
      {
         if ( Date == e.type() )
         {
            bsonDecimal decimal ;
            decimal.init() ;
            decimal.fromLong( ( INT64 )( e.date().millis ) ) ;
            builder.append( fieldName, decimal ) ;
         }
         else if ( Timestamp == e.type() )
         {
            bsonDecimal decimal ;
            UINT64 l = e.timestampTime().millis ;
            l        += e.timestampInc() / 1000 ;

            decimal.init() ;
            decimal.fromLong( ( INT64 )l ) ;
            builder.append( fieldName, decimal ) ;
         }
         else if ( Bool == e.type() )
         {
            bsonDecimal decimal ;
            INT64 v = e.Bool() ? 1 : 0 ;

            decimal.init() ;
            decimal.fromLong( ( INT64 )v ) ;
            builder.append( fieldName, decimal ) ;
         }
         else if ( NumberLong == e.type() )
         {
            bsonDecimal decimal ;
            decimal.init() ;
            decimal.fromLong( e.numberLong() ) ;
            builder.append( fieldName, decimal ) ;
         }
         else if ( String != e.type() )
         {
            bsonDecimal decimal ;
            decimal.init() ;
            decimal.fromDouble( e.numberDouble() ) ;
            builder.append( fieldName, decimal ) ;
         }
         else
         {
            bsonDecimal decimal ;
            decimal.init() ;
            decimal.fromString( e.String().c_str() ) ;
            builder.append( fieldName, decimal ) ;
         }
         break ;
      }
      case MaxKey :
         builder.appendMaxKey( fieldName ) ;
         break ;
      default:
         rc = SDB_INVALIDARG ;
         break ;
      }

      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "invalid cast type:%d", type ) ;
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   ///PD_TRACE_DECLARE_FUNCTION ( SDB__MTHCASTBUILD, "mthCastBuild" )
   INT32 mthCastBuild( const CHAR *fieldName,
                       const bson::BSONElement &e,
                       _mthSAction *action,
                       bson::BSONObjBuilder &builder )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__MTHCASTBUILD ) ;
      BSONElement arg ;
      BSONType type = EOO ;

      if ( e.eoo() )
      {
         goto done ;
      }

      arg = action->getArg().getField( "arg1" ) ;
      if ( !arg.isNumber() )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "invalid arg element:%s",
                 arg.toString( TRUE, TRUE ).c_str() ) ;
         goto error ;
      }

      type = ( BSONType )( arg.numberInt() ) ;
      if ( EOO == type )
      {
         PD_LOG( PDERROR, "can not cast to eoo" ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      else if ( e.type() == type )
      {
         builder.appendAs( e, fieldName ) ;
      }
      else
      {
         rc = _mthCast( fieldName, e, type, builder ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to cast element[%s] to"
                    " type[%d]", e.toString( TRUE, TRUE ).c_str(), type ) ;
            goto error ;
                   
         }
      }
   done:
      PD_TRACE_EXITRC( SDB__MTHCASTBUILD, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   ///PD_TRACE_DECLARE_FUNCTION ( SDB__MTHCASTGET, "mthCastGet" )
   INT32 mthCastGet( const CHAR *fieldName,
                     const bson::BSONElement &in,
                     _mthSAction *action,
                     bson::BSONElement &out )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__MTHCASTGET ) ;
      BSONElement arg ;
      BSONType type = EOO ;
      BSONObjBuilder builder ;

      if ( in.eoo() )
      {
         goto done ;
      }

      arg = action->getArg().getField( "arg1" ) ;
      if ( !arg.isNumber() )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "invalid arg element:%s",
                 arg.toString( TRUE, TRUE ).c_str() ) ;
         goto error ;
      }

      type = ( BSONType )( arg.numberInt() ) ;
      if ( in.type() == type )
      {
         out = in ;
         goto done ;
      }
      else
      {
         rc = _mthCast( fieldName, in, type, builder ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to cast element[%s] to"
                    " type[%d]", in.toString( TRUE, TRUE ).c_str(), type ) ;
            goto error ;

         }
         action->setObj( builder.obj() ) ;
         out = action->getObj().getField( fieldName ) ;    
      }
   done:
      PD_TRACE_EXITRC( SDB__MTHCASTGET, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   static void _getSubStr( const CHAR *str,
                           INT32 strLen,
                           INT32 begin,
                           INT32 limit,
                           const CHAR *&pos,
                           INT32 &cpLen )
   {
      const CHAR *cpBegin = NULL ;

      if ( strLen < 0 )
      {
         goto error ;
      }

      if ( 0 <= begin )
      {
         if ( strLen <= begin )
         {
            goto error ;
         }
         cpBegin = str + begin ;
         cpLen = strLen - begin ;
      }
      else
      {
         INT32 beginPos = strLen + begin ;
         if ( beginPos < 0 )
         {
            goto error ;
         }
         cpBegin = str + beginPos ;
         cpLen = strLen - beginPos ;
      }

      if ( 0 <= limit && limit < cpLen )
      {
         cpLen = limit ;
      }

      pos = cpBegin ;
   done:
      return ;
   error:
      pos = NULL ;
      cpLen = -1 ;
      goto done ;
   }

   ///PD_TRACE_DECLARE_FUNCTION ( SDB__MTHSUBSTRBUILD, "mthSubStrBuild" )
   INT32 mthSubStrBuild( const CHAR *fieldName,
                         const bson::BSONElement &e,
                         _mthSAction *action,
                         bson::BSONObjBuilder &builder )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__MTHSUBSTRBUILD ) ;
      SDB_ASSERT( NULL != action, "can not be null" ) ;
      INT32 begin = 0 ;
      INT32 limit = -1 ;
      if ( e.eoo() )
      {
         goto done ;
      }
      else if ( String != e.type() )
      {
         builder.appendNull( fieldName ) ;
      }
      else
      {
         begin = action->getArg().getIntField( "arg1" ) ;
         limit = action->getArg().getIntField( "arg2" ) ;
         const CHAR *pos = NULL ;
         INT32 cpLen = -1 ;
         _getSubStr( e.valuestr(),
                     e.valuestrsize() - 1,
                     begin, limit,
                     pos, cpLen ) ;
         if ( NULL == pos || -1 == cpLen )
         {
            builder.append( fieldName, "" ) ;
         }
         else
         {
            builder.appendStrWithNoTerminating( fieldName, pos, cpLen ) ;
         }
      }
   done:
      PD_TRACE_EXITRC( SDB__MTHSUBSTRBUILD, rc ) ;
      return rc ;
   }

   ///PD_TRACE_DECLARE_FUNCTION ( SDB__MTHSUBSTRGET, "mthSubStrGet" )
   INT32 mthSubStrGet( const CHAR *fieldName,
                       const bson::BSONElement &in,
                       _mthSAction *action,
                       bson::BSONElement &out )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__MTHSUBSTRGET ) ;
      SDB_ASSERT( NULL != action, "can not be null" ) ;
      BSONObjBuilder builder ;
      BSONObj obj ;
      if ( in.eoo() )
      {
         goto done ;
      }
      else if ( String != in.type() )
      {
         builder.appendNull( fieldName ) ;
      }
      else
      {
         INT32 begin = action->getArg().getIntField( "arg1" ) ;
         INT32 limit = action->getArg().getIntField( "arg2" ) ;
         const CHAR *pos = NULL ;
         INT32 cpLen = -1 ;
         _getSubStr( in.valuestr(),
                     in.valuestrsize(),
                     begin, limit,
                     pos, cpLen ) ;
         if ( NULL == pos || -1 == cpLen )
         {
            builder.append( fieldName, "" ) ;
            obj = builder.obj() ;
         }
         else if ( pos == in.valuestr() &&
                   cpLen == in.valuestrsize() - 1 )
         {
            out = in ;
            goto done ;
         }
         else
         {
            builder.appendStrWithNoTerminating( fieldName, pos, cpLen ) ;
            obj = builder.obj() ;
         }
      }

      if ( !obj.isEmpty() )
      {
         action->setObj( obj ) ;
         out = action->getObj().getField( fieldName ) ;      
      }
      
      PD_TRACE_EXITRC( SDB__MTHSUBSTRGET, rc ) ;
   done:
      return rc ;
   }

   ///PD_TRACE_DECLARE_FUNCTION ( SDB__MTHSTRLENBUILD, "mthStrLenBuild" )
   INT32 mthStrLenBuild( const CHAR *fieldName,
                         const bson::BSONElement &e,
                         _mthSAction *action,
                         bson::BSONObjBuilder &builder )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__MTHSTRLENBUILD ) ;
      SDB_ASSERT( NULL != action, "can not be null" ) ;
      if ( e.eoo() )
      {
         goto done ;
      }
      else if ( String != e.type() )
      {
         builder.appendNull( fieldName ) ;
      }
      else
      {
         builder.append( fieldName, e.valuestrsize() - 1 ) ;
      }
   done:
      PD_TRACE_EXITRC( SDB__MTHSTRLENBUILD, rc ) ;
      return rc ;
   }

   ///PD_TRACE_DECLARE_FUNCTION ( SDB__MTHSTRLENGET, "mthStrLenGet" )
   INT32 mthStrLenGet( const CHAR *fieldName,
                       const bson::BSONElement &in,
                       _mthSAction *action,
                       bson::BSONElement &out )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__MTHSTRLENGET ) ;
      BSONObjBuilder builder ;
      BSONObj obj ;
      if ( in.eoo() )
      {
         goto done ;
      }
      else if ( String != in.type() )
      {
         builder.appendNull( fieldName ) ;
         obj = builder.obj() ;
      }
      else
      {
         builder.append( fieldName, in.valuestrsize() - 1 ) ;
         obj = builder.obj() ;
      }

      if ( !obj.isEmpty() )
      {
         action->setObj( obj ) ;
         out = action->getObj().getField( fieldName ) ;
      }
   done:
      PD_TRACE_EXITRC( SDB__MTHSTRLENGET, rc ) ;
      return rc ;
   }

   static BOOLEAN _isLower( const CHAR *str )
   {
      BOOLEAN rc = TRUE ; 
      const CHAR *p = str ;
      while ( '\0' != *p )
      {
         if ( 'A' <= *p &&
              *p <= 'Z' )
         {
            rc = FALSE ;
            break ;
         }
         ++p ;
      }
      return rc ;
   }

   static BOOLEAN _isUpper( const CHAR *str )
   {
      BOOLEAN rc = TRUE ;
      const CHAR *p = str ;
      while ( '\0' != *p )
      {
         if ( 'a' <= *p &&
              *p <= 'z' )
         {
            rc = FALSE ;
            break ;
         }
         ++p ;
      }
      return rc ;
   }

   /// TODO:move lower and upper to utilStr.hpp
   static INT32 _lower( const CHAR *str,
                        UINT32 len,
                        utilString &us )
   {
      INT32 rc = SDB_OK ;
      us.resize( len ) ;
      for ( UINT32 i = 0; i < len; ++i )
      {
         const CHAR *p = str + i ;
         if ( 'A' <= *p &&
              *p <= 'Z' )
         {
            rc = us.append( *p + 32 ) ;
         }
         else
         {
             rc = us.append( *p ) ;
         }

         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to append str:%d", rc ) ;
            goto error ;
         }
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   static INT32 _upper( const CHAR *str,
                        UINT32 len,
                        utilString &us )
   {
      INT32 rc = SDB_OK ;
      us.resize( len ) ;
      for ( UINT32 i = 0; i < len; ++i )
      {
         const CHAR *p = str + i ;
         if ( 'a' <= *p &&
              *p <= 'z' )
         {
            rc = us.append( *p - 32 ) ;
         }
         else
         {
             rc = us.append( *p ) ;
         }

         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to append str:%d", rc ) ;
            goto error ;
         }
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   ///PD_TRACE_DECLARE_FUNCTION ( SDB__MTHLOWERBUILD, "mthLowerBuild" )
   INT32 mthLowerBuild( const CHAR *fieldName,
                        const bson::BSONElement &e,
                        _mthSAction *action,
                        bson::BSONObjBuilder &builder )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__MTHLOWERBUILD ) ;
      if ( e.eoo() )
      {
         goto done ;
      }
      else if ( String != e.type() )
      {
         builder.appendNull( fieldName ) ;
      }
      else
      {
         utilString us ;
         rc = _lower( e.valuestr(),
                      e.valuestrsize(),
                      us ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to create lower str:%d", rc ) ;
            goto error ;
         }

         builder.append( fieldName, us.str() ) ;
      }
   done:
      PD_TRACE_EXITRC( SDB__MTHLOWERBUILD, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   ///PD_TRACE_DECLARE_FUNCTION ( SDB__MTHLOWERGET, "mthLowerGet" )
   INT32 mthLowerGet( const CHAR *fieldName,
                      const bson::BSONElement &in,
                      _mthSAction *action,
                      bson::BSONElement &out )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__MTHLOWERGET ) ;
      BSONObjBuilder builder ;
      BSONObj obj ;

      if ( in.eoo() )
      {
         goto done ;
      }
      else if ( String != in.type() )
      {
         builder.appendNull( fieldName ) ;
         obj = builder.obj() ;
      }
      else if ( _isLower( in.valuestr() ) )
      {
         out = in ;
         goto done ;
      }
      else
      {
         utilString us ;
         rc = _lower( in.valuestr(),
                      in.valuestrsize(),
                      us ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to create lower str:%d", rc ) ;
            goto error ;
         }

         builder.append( fieldName, us.str() ) ;
         obj = builder.obj() ;
      }

      if ( !obj.isEmpty() )
      {
         action->setObj( obj ) ;
         out = action->getObj().getField( fieldName ) ;      
      }
   done:
      PD_TRACE_EXITRC( SDB__MTHLOWERGET, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   ///PD_TRACE_DECLARE_FUNCTION ( SDB__MTHUPPERBUILD, "mthUpperBuild" )
   INT32 mthUpperBuild( const CHAR *fieldName,
                        const bson::BSONElement &e,
                        _mthSAction *action,
                        bson::BSONObjBuilder &builder )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__MTHUPPERBUILD ) ;
      if ( e.eoo() )
      {
         goto done ;
      }
      else if ( String != e.type() )
      {
         builder.appendNull( fieldName ) ;
      }
      else
      {
         utilString us ;
         rc = _upper( e.valuestr(),
                      e.valuestrsize(),
                      us ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to create upper str:%d", rc ) ;
            goto error ;
         }

         builder.append( fieldName, us.str() ) ;
      }
   done:
      PD_TRACE_EXITRC( SDB__MTHUPPERBUILD, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   ///PD_TRACE_DECLARE_FUNCTION ( SDB__MTHUPPERGET, "mthUpperGet" )
   INT32 mthUpperGet( const CHAR *fieldName,
                      const bson::BSONElement &in,
                      _mthSAction *action,
                      bson::BSONElement &out )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__MTHUPPERGET ) ;
      BSONObjBuilder builder ;
      BSONObj obj ;

      if ( in.eoo() )
      {
         goto done ;
      }
      else if ( String != in.type() )
      {
         builder.appendNull( fieldName ) ;
         obj = builder.obj() ;
      }
      else if ( _isUpper( in.valuestr() ) )
      {
         out = in ;
         goto done ;
      }
      else
      {
         utilString us ;
         rc = _upper( in.valuestr(),
                      in.valuestrsize(),
                      us ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to create upper str:%d", rc ) ;
            goto error ;
         }

         builder.append( fieldName, us.str() ) ;
         obj = builder.obj() ;
      }

      if ( !obj.isEmpty() )
      {
         action->setObj( obj ) ;
         out = action->getObj().getField( fieldName ) ;
      }
   done:
      PD_TRACE_EXITRC( SDB__MTHUPPERGET, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   /// lr: -1(ltrim) 0(trim) 1(rtrim)
   static BOOLEAN isTrimed( const CHAR *str,
                            INT32 size,
                            INT8 lr )
   {
      BOOLEAN rc = TRUE ;
      SDB_ASSERT( NULL != str, "can not be null" ) ;
      INT32 strLen = 0 <= size ? size : ossStrlen( str ) ;
      if ( 0 == strLen )
      {
         goto done ;
      }

      if ( lr <= 0 )
      {
         if ( ' ' == *str ||
              '\t' == *str ||
              '\n' == *str ||
              '\r' == *str )
         {
            rc = FALSE ;
            goto done ;
         }
      }

      if ( 0 <= lr )
      {
         if ( ' ' == *( str + strLen - 1 ) ||
              '\t' == *( str + strLen - 1 ) ||
              '\n' == *( str + strLen - 1 ) ||
              '\r' == *( str + strLen - 1 ) )
         {
            rc = FALSE ;
            goto done ;
         }
      }
   done:
      return rc ;
   }

   static void ltrim( const CHAR *str,
                      const CHAR *&trimed )
   {
      const CHAR *p = str ;
      while ( '\0' != *p )
      {
         if ( ' ' != *p &&
              '\t' != *p &&
              '\n' != *p &&
              '\r' != *p )
         {
            break ;
         }
         ++p ;
      }
      trimed = p ;
      return ;
   }

   static INT32 rtrim( const CHAR *str,
                       INT32 size,
                       _utilString &us )
   {
      INT32 rc = SDB_OK ;
      INT32 pos = size - 1 ;
      while ( 0 <= pos )
      {
         const CHAR *p = str + pos ;
         if ( ' ' != *p &&
              '\t' != *p &&
              '\n' != *p &&
              '\r' != *p )
         {
            break ;
         }
         --pos ;
      }

      if ( 0 <= pos )
      {
         rc = us.append( str, pos + 1 ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to append string:%d", rc ) ;
            goto error ;
         }
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   static INT32 trim( const CHAR *str,
                      INT32 size,
                      INT8 lr,
                      _utilString &us )
   {
      INT32 rc = SDB_OK ;
      SDB_ASSERT( NULL != str, "can not be null" ) ;
      INT32 strLen = 0 <= size ? size : ossStrlen( str ) ;
      const CHAR *p = str ;
      if ( 0 == strLen )
      {
         goto done ;
      }

      if ( lr <= 0 )
      {
         const CHAR *newP = NULL ;
         ltrim( p, newP ) ;
         p = newP ;
      }

      if ( 0 <= lr )
      {
         rc = rtrim( p, size - ( p - str ), us ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to trim right site:%d", rc ) ;
            goto error ;
         }
      }
      else
      {
         /// necessary to avoid one more copy when 
         /// str is like "  abc" ?
         rc = us.append( p, size - ( p - str ) ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to trim right site:%d", rc ) ;
            goto error ;
         }
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   ///PD_TRACE_DECLARE_FUNCTION ( SDB__MTHLRTRIMBUILD, "mthLRTrimBuild" )
   static INT32 mthLRTrimBuild( const CHAR *fieldName,
                                const bson::BSONElement &e,
                                _mthSAction *action,
                                INT8 lr,
                                bson::BSONObjBuilder &builder )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__MTHLRTRIMBUILD ) ;
      if ( e.eoo() )
      {
         goto done ;
      }
      else if ( String != e.type() )
      {
         builder.appendNull( fieldName ) ;
      }
      else if ( isTrimed( e.valuestr(),
                          e.valuestrsize() - 1,
                          lr ) )
      {
         builder.appendAs( e, fieldName ) ;
      }
      else
      {
         utilString us ;
         rc = trim( e.valuestr(),
                    e.valuestrsize() - 1,
                    lr, us ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to trim string:%d", rc ) ;
            goto error ;
         }

         builder.append( fieldName, us.str() ) ;
      }
   done:
      PD_TRACE_EXITRC( SDB__MTHLRTRIMBUILD, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   ///PD_TRACE_DECLARE_FUNCTION ( SDB__MTHLRTRIMGET, "mthLRTrimGet" )
   INT32 mthLRTrimGet( const CHAR *fieldName,
                       const bson::BSONElement &in,
                       _mthSAction *action,
                       INT8 lr,
                       bson::BSONElement &out )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__MTHLRTRIMGET ) ;
      BSONObjBuilder builder ;
      if ( in.eoo() )
      {
         goto done ;
      }
      else if ( String != in.type() )
      {
         builder.appendNull( fieldName ) ;
      }
      else if ( isTrimed( in.valuestr(),
                          in.valuestrsize() - 1 ,
                          lr ) )
      {
         out = in ;
         goto done ;
      }
      else
      {
         utilString us ;
         rc = trim( in.valuestr(),
                    in.valuestrsize() - 1,
                    lr, us ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to trim string:%d", rc ) ;
            goto error ;
         }

         builder.append( fieldName, us.str() ) ;
      }

      action->setObj( builder.obj() ) ;
      out = action->getObj().getField( fieldName ) ;
   done:
      PD_TRACE_EXITRC( SDB__MTHLRTRIMGET, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   ///PD_TRACE_DECLARE_FUNCTION ( SDB__MTHTRIMBUILD, "mthTrimBuild" )
   INT32 mthTrimBuild( const CHAR *fieldName,
                       const bson::BSONElement &e,
                       _mthSAction *action,
                       bson::BSONObjBuilder &builder )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__MTHTRIMBUILD ) ;
      rc = mthLRTrimBuild( fieldName, e, action, 0, builder ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to trim string:%d", rc ) ;
         goto error ;
      }
   done:
      PD_TRACE_EXITRC( SDB__MTHTRIMBUILD, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   ///PD_TRACE_DECLARE_FUNCTION ( SDB__MTHTRIMGET, "mthTrimGet" )
   INT32 mthTrimGet( const CHAR *fieldName,
                     const bson::BSONElement &in,
                     _mthSAction *action,
                     bson::BSONElement &out )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__MTHTRIMGET ) ;
      rc = mthLRTrimGet( fieldName, in, action, 0, out ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to trim string:%d", rc ) ;
         goto error ;
      }
   done:
      PD_TRACE_EXITRC( SDB__MTHTRIMGET, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   ///PD_TRACE_DECLARE_FUNCTION ( SDB__MTHLTRIMBUILD, "mthLTrimBuild" )
   INT32 mthLTrimBuild( const CHAR *fieldName,
                        const bson::BSONElement &e,
                        _mthSAction *action,
                        bson::BSONObjBuilder &builder )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__MTHLTRIMBUILD ) ;
      rc = mthLRTrimBuild( fieldName, e, action, -1, builder ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to trim string:%d", rc ) ;
         goto error ;
      }
   done:
      PD_TRACE_EXITRC( SDB__MTHLTRIMBUILD, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   ///PD_TRACE_DECLARE_FUNCTION ( SDB__MTHLTRIMGET, "mthLTrimGet" )
   INT32 mthLTrimGet( const CHAR *fieldName,
                      const bson::BSONElement &in,
                      _mthSAction *action,
                      bson::BSONElement &out )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__MTHLTRIMGET ) ;
      rc = mthLRTrimGet( fieldName, in, action, -1, out ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to trim string:%d", rc ) ;
         goto error ;
      }
   done:
      PD_TRACE_EXITRC( SDB__MTHLTRIMGET, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   ///PD_TRACE_DECLARE_FUNCTION ( SDB__MTHRTRIMBUILD, "mthRTrimBuild" )
   INT32 mthRTrimBuild( const CHAR *fieldName,
                        const bson::BSONElement &e,
                        _mthSAction *action,
                        bson::BSONObjBuilder &builder )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__MTHRTRIMBUILD ) ;
      rc = mthLRTrimBuild( fieldName, e, action, 1, builder ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to trim string:%d", rc ) ;
         goto error ;
      }
   done:
      PD_TRACE_EXITRC( SDB__MTHRTRIMBUILD, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   ///PD_TRACE_DECLARE_FUNCTION ( SDB__MTHRTRIMGET, "mthRTrimGet" )
   INT32 mthRTrimGet( const CHAR *fieldName,
                      const bson::BSONElement &in,
                      _mthSAction *action,
                      bson::BSONElement &out )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__MTHRTRIMGET ) ;
      rc = mthLRTrimGet( fieldName, in, action, 1, out ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to trim string:%d", rc ) ;
         goto error ;
      }
   done:
      PD_TRACE_EXITRC( SDB__MTHRTRIMGET, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   ///PD_TRACE_DECLARE_FUNCTION ( SDB__MTHADDBUILD, "mthAddBuild" )
   INT32 mthAddBuild( const CHAR *fieldName,
                      const bson::BSONElement &e,
                      _mthSAction *action,
                      bson::BSONObjBuilder &builder )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__MTHADDBUILD ) ;
      SDB_ASSERT( NULL != action, "can not be null" ) ;
      const BSONObj &obj = action->getArg() ;
      BSONElement arg = obj.getField( "arg1" ) ;
      SDB_ASSERT( arg.isNumber(), "must be numeric" ) ;

      if ( e.eoo() )
      {
         goto done ;
      }
      else if ( !e.isNumber() )
      {
         builder.appendNull( fieldName ) ;
      }
      else if ( NumberDecimal == e.type() || 
                NumberDecimal == arg.type() )
      {
         bsonDecimal decimalE ;
         bsonDecimal decimalArg ;
         bsonDecimal result ;
         result.init() ;

         decimalE   = e.numberDecimal() ;
         decimalArg = arg.numberDecimal() ;
         rc = decimalE.add( decimalArg, result ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to add decimal:%s+%s,rc=%d", 
                    decimalE.toString().c_str(), 
                    decimalArg.toString().c_str(), rc ) ;
            goto error ;
         }

         builder.append( fieldName, result ) ;
      }
      else if ( NumberDouble == e.type() ||
                NumberDouble == arg.type() )
      {
         FLOAT64 f = arg.numberDouble() + e.numberDouble() ;
         builder.appendNumber( fieldName, f ) ;
      }
      else
      {
         INT64 i = arg.numberLong() + e.numberLong() ;
         builder.appendIntOrLL( fieldName, i ) ;
      }
   done:
      PD_TRACE_EXITRC( SDB__MTHADDBUILD, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   ///PD_TRACE_DECLARE_FUNCTION ( SDB__MTHADDGET, "mthAddGet" )
   INT32 mthAddGet( const CHAR *fieldName,
                    const bson::BSONElement &in,
                    _mthSAction *action,
                    bson::BSONElement &out )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__MTHADDGET ) ;
      BSONObjBuilder builder ;
      BSONObj obj ;
      BSONElement arg = action->getArg().getField( "arg1" ) ;
      SDB_ASSERT( arg.isNumber(), "must be numeric" ) ;

      if ( in.eoo() )
      {
         /// do nothing.
      }
      else if ( !in.isNumber() )
      {
         builder.appendNull( fieldName ) ;
         obj = builder.obj() ;
      }
      else if ( NumberDecimal == in.type() || 
                NumberDecimal == arg.type() )
      {
         bsonDecimal decimalE ;
         bsonDecimal decimalArg ;
         bsonDecimal result ;
         result.init() ;

         decimalE   = in.numberDecimal() ;
         decimalArg = arg.numberDecimal() ;
         rc = decimalE.add( decimalArg, result ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to add decimal:%s+%s,rc=%d", 
                    decimalE.toString().c_str(), 
                    decimalArg.toString().c_str(), rc ) ;
            goto error ;
         }

         builder.append( fieldName, result ) ;
         obj = builder.obj() ;
      }
      else if ( NumberDouble == in.type() ||
                NumberDouble == arg.type() )
      {
         FLOAT64 f = in.numberDouble() + arg.numberDouble() ;
         builder.appendNumber( fieldName, f ) ;
         obj = builder.obj() ;
      }
      else
      {
         INT64 l = in.numberLong() + arg.numberLong() ;
         builder.appendIntOrLL( fieldName, l ) ;
         obj = builder.obj() ;
      }

      if ( !obj.isEmpty() )
      {
         action->setObj( obj ) ;
         out = action->getObj().getField( fieldName ) ;
      }
   done:
      PD_TRACE_EXITRC( SDB__MTHADDGET, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   ///PD_TRACE_DECLARE_FUNCTION ( SDB__MTHSUBTRACTBUILD, "mthSubtractBuild" )
   INT32 mthSubtractBuild( const CHAR *fieldName,
                           const bson::BSONElement &e,
                           _mthSAction *action,
                           bson::BSONObjBuilder &builder )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__MTHSUBTRACTBUILD ) ;
      SDB_ASSERT( NULL != action, "can not be null" ) ;
      const BSONObj &obj = action->getArg() ;
      BSONElement arg = obj.getField( "arg1" ) ;
      SDB_ASSERT( arg.isNumber(), "must be numeric" ) ;

      if ( e.eoo() )
      {
         goto done ;
      }
      else if ( !e.isNumber() )
      {
         builder.appendNull( fieldName ) ;
      }
      else if ( NumberDecimal == e.type() || 
                NumberDecimal == arg.type() )
      {
         bsonDecimal decimalE ;
         bsonDecimal decimalArg ;
         bsonDecimal result ;
         result.init() ;

         decimalE   = e.numberDecimal() ;
         decimalArg = arg.numberDecimal() ;
         rc = decimalE.sub( decimalArg, result ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to sub decimal:%s-%s,rc=%d", 
                    decimalE.toString().c_str(), 
                    decimalArg.toString().c_str(), rc ) ;
            goto error ;
         }

         builder.append( fieldName, result ) ;
      }
      else if ( NumberDouble == e.type() ||
                NumberDouble == arg.type() )
      {
         FLOAT64 f = e.numberDouble() - arg.numberDouble() ;
         builder.appendNumber( fieldName, f ) ;
      }
      else
      {
         INT64 i = e.numberLong() - arg.numberLong() ;
         builder.appendIntOrLL( fieldName, i ) ;
      }
   done:
      PD_TRACE_EXITRC( SDB__MTHSUBTRACTBUILD, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   ///PD_TRACE_DECLARE_FUNCTION ( SDB__MTHSUBTRACTGET, "mthSubtractGet" )
   INT32 mthSubtractGet( const CHAR *fieldName,
                         const bson::BSONElement &in,
                         _mthSAction *action,
                         bson::BSONElement &out )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__MTHSUBTRACTGET ) ;
      BSONObjBuilder builder ;
      BSONObj obj ;
      BSONElement arg = action->getArg().getField( "arg1" ) ;
      SDB_ASSERT( arg.isNumber(), "must be numeric" ) ;

      if ( in.eoo() )
      {
         /// do nothing.   
      }
      else if ( !in.isNumber() )
      {
         builder.appendNull( fieldName ) ;
         obj = builder.obj() ;
      }
      else if ( NumberDecimal == in.type() || 
                NumberDecimal == arg.type() )
      {
         bsonDecimal decimalE ;
         bsonDecimal decimalArg ;
         bsonDecimal result ;
         result.init() ;

         decimalE   = in.numberDecimal() ;
         decimalArg = arg.numberDecimal() ;
         rc = decimalE.sub( decimalArg, result ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to sub decimal:%s-%s,rc=%d", 
                    decimalE.toString().c_str(), 
                    decimalArg.toString().c_str(), rc ) ;
            goto error ;
         }

         builder.append( fieldName, result ) ;
         obj = builder.obj() ;
      }
      else if ( NumberDouble == in.type() ||
                NumberDouble == arg.type() )
      {
         FLOAT64 f = in.numberDouble() - arg.numberDouble() ;
         builder.appendNumber( fieldName, f ) ;
         obj = builder.obj() ;
      }
      else
      {
         INT64 l = in.numberLong() - arg.numberLong() ;
         builder.appendIntOrLL( fieldName, l ) ;
         obj = builder.obj() ;
      }

      if ( !obj.isEmpty() )
      {
         action->setObj( obj ) ;
         out = action->getObj().getField( fieldName ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB__MTHSUBTRACTGET, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   ///PD_TRACE_DECLARE_FUNCTION ( SDB__MTHMULTIPLYBUILD, "mthMultiplyBuild" )
   INT32 mthMultiplyBuild( const CHAR *fieldName,
                           const bson::BSONElement &e,
                           _mthSAction *action,
                           bson::BSONObjBuilder &builder )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__MTHMULTIPLYBUILD ) ;
      const BSONObj &obj = action->getArg() ;
      BSONElement arg = obj.getField( "arg1" ) ;
      SDB_ASSERT( arg.isNumber(), "must be numeric" ) ;

      if ( e.eoo() )
      {
         goto done ;
      }
      else if ( !e.isNumber() )
      {
         builder.appendNull( fieldName ) ;
      }
      else if ( NumberDecimal == e.type() || 
                NumberDecimal == arg.type() )
      {
         bsonDecimal decimal ;
         bsonDecimal decimalArg ;
         bsonDecimal result ;
         result.init() ;

         decimal    = e.numberDecimal() ;
         decimalArg = arg.numberDecimal() ;
         rc = decimal.mul( decimalArg, result ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to mul decimal:%s*%s,rc=%d", 
                    decimal.toString().c_str(), 
                    decimalArg.toString().c_str(), rc ) ;
            goto error ;
         }

         builder.append( fieldName, result ) ;
      }
      else if ( NumberDouble == e.type() ||
                NumberDouble == arg.type() )
      {
         FLOAT64 f = arg.numberDouble() * e.numberDouble() ;
         builder.appendNumber( fieldName, f ) ;
      }
      else
      {
         INT64 i = arg.numberLong() * e.numberLong() ;
         builder.appendIntOrLL( fieldName, i ) ;
      }
   done:
      PD_TRACE_EXITRC( SDB__MTHMULTIPLYBUILD, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   ///PD_TRACE_DECLARE_FUNCTION ( SDB__MTHMULTIPLYGET, "mthMultiplyGet" )
   INT32 mthMultiplyGet( const CHAR *fieldName,
                         const bson::BSONElement &in,
                         _mthSAction *action,
                         bson::BSONElement &out )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__MTHMULTIPLYGET ) ;
      BSONObjBuilder builder ;
      BSONObj obj ;
      BSONElement arg = action->getArg().getField( "arg1" ) ;
      SDB_ASSERT( arg.isNumber(), "must be numeric" ) ;

      if ( in.eoo() )
      {
         /// do nothing.
      }
      else if ( !in.isNumber() )
      {
         builder.appendNull( fieldName ) ;
         obj = builder.obj() ;
      }
      else if ( NumberDecimal == in.type() || 
                NumberDecimal == arg.type() )
      {
         bsonDecimal decimal ;
         bsonDecimal decimalArg ;
         bsonDecimal result ;
         result.init() ;

         decimal    = in.numberDecimal() ;
         decimalArg = arg.numberDecimal() ;
         rc = decimal.mul( decimalArg, result ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to mul decimal:%s*%s,rc=%d", 
                    decimal.toString().c_str(), 
                    decimalArg.toString().c_str(), rc ) ;
            goto error ;
         }

         builder.append( fieldName, result ) ;
         obj = builder.obj() ;
      }
      else if ( NumberDouble == in.type() ||
                NumberDouble == arg.type() )
      {
         FLOAT64 f = in.numberDouble() * arg.numberDouble() ;
         builder.appendNumber( fieldName, f ) ;
         obj = builder.obj() ;
      }
      else
      {
         INT64 l = in.numberLong() * arg.numberLong() ;
         builder.appendIntOrLL( fieldName, l ) ;
         obj = builder.obj() ;
      }

      if ( !obj.isEmpty() )
      {
         action->setObj( obj ) ;
         out = action->getObj().getField( fieldName ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB__MTHMULTIPLYGET, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   ///PD_TRACE_DECLARE_FUNCTION ( SDB__MTHDIVIDEBUILD, "mthDivideBuild" )
   INT32 mthDivideBuild( const CHAR *fieldName,
                         const bson::BSONElement &e,
                         _mthSAction *action,
                         bson::BSONObjBuilder &builder )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__MTHDIVIDEBUILD ) ;
      const BSONObj &obj = action->getArg() ;
      BSONElement arg = obj.getField( "arg1" ) ;
      SDB_ASSERT( arg.isNumber(), "must be numeric" ) ;

      if ( e.eoo() )
      {
         goto done ;
      }
      else if ( !e.isNumber() )
      {
         builder.appendNull( fieldName ) ;
      }
      else if ( NumberDecimal == e.type() || 
                NumberDecimal == arg.type() )
      {
         bsonDecimal decimal ;
         bsonDecimal decimalArg ;
         bsonDecimal result ;
         result.init() ;

         decimal    = e.numberDecimal() ;
         decimalArg = arg.numberDecimal() ;
         rc = decimal.div( decimalArg, result ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to div decimal:%s/%s,rc=%d", 
                    decimal.toString().c_str(), 
                    decimalArg.toString().c_str(), rc ) ;
            goto error ;
         }

         builder.append( fieldName, result ) ;
      }
      else if ( NumberDouble == e.type() ||
                NumberDouble == arg.type() )
      {
         FLOAT64 r = arg.numberDouble() ;
         if ( fabs(r) < OSS_EPSILON )
         {
            PD_LOG( PDERROR, "invalid argument:%f", r ) ;
            rc = SDB_SYS ;
            goto error ;
         }
         builder.appendNumber( fieldName, e.numberDouble() / r ) ;
      }
      else
      {
         INT64 l = e.numberLong() ;
         INT64 r = arg.numberLong() ;
         if ( 0 == r )
         {
            PD_LOG( PDERROR, "invalid argument:%lld", r ) ;
            rc = SDB_SYS ; /// should not happen. so use sdb_sys.
            goto error ;
         }
         else if ( 0 == l % r )
         {
            builder.appendIntOrLL( fieldName, l / r ) ;
         }
         else
         {
            builder.appendNumber( fieldName, l / ( FLOAT64 ) r ) ;
         }
      }
   done:
      PD_TRACE_EXITRC( SDB__MTHDIVIDEBUILD, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   ///PD_TRACE_DECLARE_FUNCTION ( SDB__MTHDIVIDEGET, "mthDivideGet" )
   INT32 mthDivideGet( const CHAR *fieldName,
                       const bson::BSONElement &in,
                       _mthSAction *action,
                       bson::BSONElement &out )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__MTHDIVIDEGET ) ;
      BSONObjBuilder builder ;
      BSONObj obj ;
      BSONElement arg = action->getArg().getField( "arg1" ) ;
      SDB_ASSERT( arg.isNumber(), "must be numeric" ) ;

      if ( in.eoo() )
      {
         /// do nothing
      }
      else if ( !in.isNumber() )
      {
         builder.appendNull( fieldName ) ;
         obj = builder.obj() ;
      }
      else if ( NumberDecimal == in.type() || 
                NumberDecimal == arg.type() )
      {
         bsonDecimal decimal ;
         bsonDecimal decimalArg ;
         bsonDecimal result ;
         result.init() ;

         decimal    = in.numberDecimal() ;
         decimalArg = arg.numberDecimal() ;
         rc = decimal.div( decimalArg, result ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to div decimal:%s/%s,rc=%d", 
                    decimal.toString().c_str(), 
                    decimalArg.toString().c_str(), rc ) ;
            goto error ;
         }

         builder.append( fieldName, result ) ;
         obj = builder.obj() ;
      }
      else if ( 0 == arg.Number() )
      {
         builder.appendNull( fieldName ) ;
         obj = builder.obj() ;
      }
      else if ( NumberDouble == in.type() ||
                NumberDouble == arg.type() )
      {
         FLOAT64 r = arg.numberDouble() ;
         if ( fabs(r) < OSS_EPSILON )
         {
            PD_LOG( PDERROR, "invalid argument:%f", r ) ;
            rc = SDB_SYS ;
            goto error ;
         }
         builder.appendNumber( fieldName, in.numberDouble() / r ) ;
         obj = builder.obj() ;
      }
      else
      {
         INT64 l = in.numberLong() ;
         INT64 r = arg.numberLong() ;
         if ( 0 == r )
         {
            PD_LOG( PDERROR, "invalid argument:%lld", r ) ;
            rc = SDB_SYS ;
            goto error ;
         }
         else if ( 0 == l % r )
         {
            builder.appendIntOrLL( fieldName, l / r ) ;
         }
         else
         {
            builder.appendNumber( fieldName, l / ( FLOAT64 ) r ) ;
         }
         obj = builder.obj() ;
      }

      if ( !obj.isEmpty() )
      {
         action->setObj( obj ) ;
         out = action->getObj().getField( fieldName ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB__MTHDIVIDEGET, rc ) ;
      return rc ;
   error:
      goto done ;
   }
}


