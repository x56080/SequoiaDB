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
#include "mthElemMatchIterator.hpp"
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

      BSONObj args = action->getArg() ;

      INT32 begin = args.getIntField( "arg1" ) ;
      INT32 limit = args.getIntField( "arg2" ) ;
      rc = mthSlice( fieldName, e, begin, limit, builder ) ;
      PD_RC_CHECK( rc, PDERROR, "mthSlice failed:rc=%d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__MTHSLICEBUILD, rc ) ;
      return rc ;
   error:
      goto done ;
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
      BSONObjBuilder builder ;
      BSONObj obj ;

      BSONObj args = action->getArg() ;
      INT32 begin = args.getIntField( "arg1" ) ;
      INT32 limit = args.getIntField( "arg2" ) ;
      rc = mthSlice( fieldName, in, begin, limit, builder ) ;
      PD_RC_CHECK( rc, PDERROR, "mthSlice failed:rc=%d", rc ) ;

      obj = builder.obj() ;
      if ( !obj.isEmpty() )
      {
         action->setObj( obj ) ;
         out = action->getObj().getField( fieldName ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB__MTHSLICEGET, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   ///PD_TRACE_DECLARE_FUNCTION ( SDB__MTHELEMMATCHBUILDN, "mthElemMatchBuildN" )
   static INT32 mthElemMatchBuildN( const CHAR *fieldName,
                                    const bson::BSONElement &e,
                                    _mthSAction *action,
                                    INT32 n,
                                    BOOLEAN subFieldIsOp,
                                    bson::BSONObjBuilder &builder )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__MTHELEMMATCHBUILDN ) ;

      PD_CHECK( NULL != action->getMatchTree(), SDB_SYS, error, PDERROR,
                "Failed to get match tree" ) ;

      if ( Array == e.type() )
      {
         BSONArrayBuilder arrayBuilder( builder.subarrayStart( fieldName ) ) ;
         _mthElemMatchIterator i( e.embeddedObject(), action->getMatchTree(),
                                  action->getMatchTargetBob(),
                                  n, TRUE, subFieldIsOp ) ;
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
      else if ( Object == e.type() )
      {
         _mthElemMatchIterator i( e.embeddedObject(), action->getMatchTree(),
                                  action->getMatchTargetBob(),
                                  n, FALSE, subFieldIsOp ) ;
         do
         {
            BSONElement next ;
            rc = i.next( next ) ;
            if ( SDB_OK == rc )
            {
               builder.append( fieldName, next.wrap() ) ;
            }
            else if ( SDB_DMS_EOC == rc )
            {
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
                                  INT32 n,
                                  BOOLEAN subFieldIsOp,
                                  bson::BSONElement &out )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__MTHELEMMATCHGETN ) ;

      PD_CHECK( NULL != action->getMatchTree(), SDB_SYS, error, PDERROR,
                "Failed to get match tree" ) ;

      if ( Array == in.type() )
      {
         BSONObjBuilder objBuilder ;
         BSONArrayBuilder arrayBuilder( objBuilder.subarrayStart( fieldName ) ) ;
         _mthElemMatchIterator i( in.embeddedObject(),
                                  action->getMatchTree(),
                                  action->getMatchTargetBob(),
                                  n, TRUE, subFieldIsOp ) ;
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
      rc = mthElemMatchBuildN( fieldName, e, action, -1, FALSE, builder ) ;
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
      rc = mthElemMatchGetN( fieldName, in, action, -1, FALSE, out ) ;
      PD_TRACE_EXITRC( SDB__MTHELEMMATCHGET, rc ) ;
      return rc ;
   }

   ///PD_TRACE_DECLARE_FUNCTION ( SDB__MTHELEMMATCHBUILDSUBISOP, "mthElemMatchBuildSubIsOp" )
   INT32 mthElemMatchBuildSubIsOp( const CHAR *fieldName,
                                   const bson::BSONElement &e,
                                   _mthSAction *action,
                                   bson::BSONObjBuilder &builder )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__MTHELEMMATCHBUILDSUBISOP ) ;
      rc = mthElemMatchBuildN( fieldName, e, action, -1, TRUE, builder ) ;
      PD_TRACE_EXITRC( SDB__MTHELEMMATCHBUILDSUBISOP, rc ) ;
      return rc ;
   }

   ///PD_TRACE_DECLARE_FUNCTION ( SDB__MTHELEMMATCHGETSUBISOP, "mthElemMatchGetSubIsOp" )
   INT32 mthElemMatchGetSubIsOp( const CHAR *fieldName,
                                 const bson::BSONElement &in,
                                 _mthSAction *action,
                                 bson::BSONElement &out )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__MTHELEMMATCHGETSUBISOP ) ;
      rc = mthElemMatchGetN( fieldName, in, action, -1, TRUE, out ) ;
      PD_TRACE_EXITRC( SDB__MTHELEMMATCHGETSUBISOP, rc ) ;
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
      rc = mthElemMatchBuildN( fieldName, e, action, 1, FALSE, builder ) ;
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
      rc = mthElemMatchGetN( fieldName, in, action, 1, FALSE, out ) ;
      PD_TRACE_EXITRC( SDB__MTHELEMMATCHONEGET, rc ) ;
      return rc ;
   }

   ///PD_TRACE_DECLARE_FUNCTION ( SDB__MTHELEMMATCHONEBUILDSUBISOP, "mthElemMatchOneBuildSubIsOp" )
   INT32 mthElemMatchOneBuildSubIsOp( const CHAR *fieldName,
                                      const bson::BSONElement &e,
                                      _mthSAction *action,
                                      bson::BSONObjBuilder &builder )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__MTHELEMMATCHONEBUILDSUBISOP ) ;
      rc = mthElemMatchBuildN( fieldName, e, action, 1, TRUE, builder ) ;
      PD_TRACE_EXITRC( SDB__MTHELEMMATCHONEBUILDSUBISOP, rc ) ;
      return rc ;
   }

   ///PD_TRACE_DECLARE_FUNCTION ( SDB__MTHELEMMATCHONEGETSUBISOP, "mthElemMatchOneGetSubIsOp" )
   INT32 mthElemMatchOneGetSubIsOp( const CHAR *fieldName,
                                    const bson::BSONElement &in,
                                    _mthSAction *action,
                                    bson::BSONElement &out )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__MTHELEMMATCHONEGETSUBISOP ) ;
      rc = mthElemMatchGetN( fieldName, in, action, 1, TRUE, out ) ;
      PD_TRACE_EXITRC( SDB__MTHELEMMATCHONEGETSUBISOP, rc ) ;
      return rc ;
   }

   ///PD_TRACE_DECLARE_FUNCTION ( SDB__MTHABSBUILD, "mthAbsBuild" )
   INT32 mthAbsBuild( const CHAR *fieldName,
                      const bson::BSONElement &e,
                      _mthSAction *action,
                      bson::BSONObjBuilder &builder )
   {
      INT32 rc = SDB_OK ;
      INT32 flag = 0 ;
      PD_TRACE_ENTRY( SDB__MTHABSBUILD ) ;
      SDB_ASSERT( NULL != action, "can not be null" ) ;
      BOOLEAN strictDataMode = action->getStrictDataMode() ;

      rc = mthAbs( fieldName, e, builder, flag ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "mthAbs failed:rc=%d", rc ) ;
         goto error ;
      }
      if ( strictDataMode && OSS_BIT_TEST( flag, MTH_OPERATION_FLAG_OVERFLOW ) )
      {
         rc = SDB_VALUE_OVERFLOW ;
         PD_LOG( PDERROR, "overflow happened, field: %s, abs(%lld), rc = %d",
                 fieldName, e.numberLong(), rc ) ;
         goto error ;
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
      INT32 flag = 0 ;
      PD_TRACE_ENTRY( SDB__MTHABSGET ) ;
      SDB_ASSERT( NULL != action, "can not be null" ) ;
      BOOLEAN strictDataMode = action->getStrictDataMode() ;
      BSONObjBuilder builder ;
      BSONObj obj ;

      rc = mthAbs( fieldName, in, builder, flag ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "mthAbs failed:rc=%d", rc ) ;
         goto error ;
      }
      if ( strictDataMode && OSS_BIT_TEST( flag, MTH_OPERATION_FLAG_OVERFLOW ) )
      {
         rc = SDB_VALUE_OVERFLOW ;
         PD_LOG( PDERROR, "overflow happened, field: %s, abs(%lld), rc = %d",
                 fieldName, in.numberLong(), rc ) ;
         goto error ;
      }

      obj = builder.obj() ;
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
      PD_TRACE_ENTRY( SDB__MTHCEILINGBUILD ) ;
      SDB_ASSERT( NULL != action, "can not be null" ) ;

      rc = mthCeiling( fieldName, e, builder ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "mthCeiling failed:rc=%d", rc ) ;
         goto error ;
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
      PD_TRACE_ENTRY( SDB__MTHCEILINGGET ) ;
      SDB_ASSERT( NULL != action, "can not be null" ) ;
      BSONObjBuilder builder ;
      BSONObj obj ;

      rc = mthCeiling( fieldName, in, builder ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "mthCeiling failed:rc=%d", rc ) ;
         goto error ;
      }

      obj = builder.obj() ;
      if ( !obj.isEmpty() )
      {
         action->setObj( obj ) ;
         out = action->getObj().getField( fieldName ) ;
      }
   done:
      PD_TRACE_EXITRC( SDB__MTHCEILINGGET, rc ) ;
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

      rc = mthFloor( fieldName, e, builder ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "mthFloor failed:rc=%d", rc ) ;
         goto error ;
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

      rc = mthFloor( fieldName, in, builder ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "mthFloor failed:rc=%d", rc ) ;
         goto error ;
      }

      obj = builder.obj() ;
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

   ///PD_TRACE_DECLARE_FUNCTION ( SDB__MTHROUNDBUILD, "mthRoundBuild" )
   INT32 mthRoundBuild( const CHAR *fieldName,
                        const bson::BSONElement &e,
                        _mthSAction *action,
                        bson::BSONObjBuilder &builder )
   {
      INT32 rc = SDB_OK ;
      INT32 flag = 0 ;
      PD_TRACE_ENTRY( SDB__MTHROUNDBUILD ) ;
      SDB_ASSERT( NULL != action, "can not be null" ) ;
      BOOLEAN strictDataMode = action->getStrictDataMode() ;
      BSONElement arg = action->getArg().getField( "arg1" ) ;
      INT32 scale = 0 ;

      if ( !arg.isNumber() )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "invalid arg element:%s",
                 arg.toString( TRUE, TRUE ).c_str() ) ;
         goto error ;
      }
      scale = arg.numberInt() ;

      rc = mthRound( fieldName, e, scale, flag, builder ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "mthRound failed:rc=%d", rc ) ;
         goto error ;
      }
      if ( strictDataMode && OSS_BIT_TEST( flag, MTH_OPERATION_FLAG_OVERFLOW ) )
      {
         rc = SDB_VALUE_OVERFLOW ;
         PD_LOG( PDERROR, "overflow happened, field: %s, round(%s) by scale(%d), "
                 "rc = %d", fieldName, e.toPoolString( FALSE ).c_str(), scale, rc ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__MTHROUNDBUILD, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   ///PD_TRACE_DECLARE_FUNCTION ( SDB__MTHROUNDGET, "mthRoundGet" )
   INT32 mthRoundGet( const CHAR *fieldName,
                      const bson::BSONElement &e,
                      _mthSAction *action,
                      bson::BSONElement &out )
   {
      INT32 rc = SDB_OK ;
      INT32 flag = 0 ;
      PD_TRACE_ENTRY( SDB__MTHROUNDGET ) ;
      SDB_ASSERT( NULL != action, "can not be null" ) ;
      BOOLEAN strictDataMode = action->getStrictDataMode() ;
      BSONObjBuilder builder ;
      BSONObj obj ;
      BSONElement arg = action->getArg().getField( "arg1" ) ;
      INT32 scale = 0 ;

      if ( !arg.isNumber() )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "invalid arg element:%s",
                 arg.toString( TRUE, TRUE ).c_str() ) ;
         goto error ;
      }
      scale = arg.numberInt() ;

      rc = mthRound( fieldName, e, scale, flag, builder ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "mthFloor failed:rc=%d", rc ) ;
         goto error ;
      }
      if ( strictDataMode && OSS_BIT_TEST( flag, MTH_OPERATION_FLAG_OVERFLOW ) )
      {
         rc = SDB_VALUE_OVERFLOW ;
         PD_LOG( PDERROR, "overflow happened, field: %s, round(%s) by scale(%d), "
                 "rc = %d", fieldName, e.toPoolString( FALSE ).c_str(), scale, rc ) ;
         goto error ;
      }

      obj = builder.obj() ;
      if ( !obj.isEmpty() )
      {
         action->setObj( obj ) ;
         out = action->getObj().getField( fieldName ) ;
      }
   done:
      PD_TRACE_EXITRC( SDB__MTHROUNDGET, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   ///PD_TRACE_DECLARE_FUNCTION ( SDB__MTHFORMATBUILD, "mthFormatBuild" )
   INT32 mthFormatBuild( const CHAR *fieldName,
                         const bson::BSONElement &e,
                         _mthSAction *action,
                         bson::BSONObjBuilder &builder )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__MTHFORMATBUILD ) ;
      SDB_ASSERT( NULL != action, "can not be null" ) ;
      BSONElement arg = action->getArg().getField( "arg1" ) ;
      INT32 scale = 0 ;

      if ( !arg.isNumber() )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "invalid arg element:%s",
                 arg.toString( TRUE, TRUE ).c_str() ) ;
         goto error ;
      }
      scale = arg.numberInt() ;

      rc = mthFormat( fieldName, e, scale, builder ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "mthFormat failed:rc=%d", rc ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__MTHFORMATBUILD, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   ///PD_TRACE_DECLARE_FUNCTION ( SDB__MTHFORMATGET, "mthFormatGet" )
   INT32 mthFormatGet( const CHAR *fieldName,
                       const bson::BSONElement &e,
                       _mthSAction *action,
                       bson::BSONElement &out )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__MTHFORMATGET ) ;
      SDB_ASSERT( NULL != action, "can not be null" ) ;
      BSONObjBuilder builder ;
      BSONObj obj ;
      BSONElement arg = action->getArg().getField( "arg1" ) ;
      INT32 scale = 0 ;

      if ( !arg.isNumber() )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "invalid arg element:%s",
                 arg.toString( TRUE, TRUE ).c_str() ) ;
         goto error ;
      }
      scale = arg.numberInt() ;

      rc = mthFormat( fieldName, e, scale, builder ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "mthFormat failed:rc=%d", rc ) ;
         goto error ;
      }

      obj = builder.obj() ;
      if ( !obj.isEmpty() )
      {
         action->setObj( obj ) ;
         out = action->getObj().getField( fieldName ) ;
      }
   done:
      PD_TRACE_EXITRC( SDB__MTHFORMATGET, rc ) ;
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

      rc = mthMod( fieldName, e, arg, builder ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "mthMod failed:rc=%d", rc ) ;
         goto error ;
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

      rc = mthMod( fieldName, in, argEle, builder ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "mthMod failed:rc=%d", rc ) ;
         goto error ;
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

      arg = action->getArg().getField( "arg1" ) ;
      if ( !arg.isNumber() )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "invalid arg element:%s",
                 arg.toString( TRUE, TRUE ).c_str() ) ;
         goto error ;
      }

      type = ( BSONType )( arg.numberInt() ) ;
      rc = mthCast( fieldName, e, type, builder ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "mthCast failed:rc=%d", rc ) ;
         goto error ;
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
         rc = mthCast( fieldName, in, type, builder ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to cast element[%s] to"
                    " type[%d]", in.toString().c_str(), type ) ;
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

      begin = action->getArg().getIntField( "arg1" ) ;
      limit = action->getArg().getIntField( "arg2" ) ;
      rc = mthSubStr( fieldName, e, begin, limit, builder ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "mthSubStr failed:rc=%d", rc ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__MTHSUBSTRBUILD, rc ) ;
      return rc ;
   error:
      goto done ;
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
      INT32 begin = 0 ;
      INT32 limit = -1 ;

      begin = action->getArg().getIntField( "arg1" ) ;
      limit = action->getArg().getIntField( "arg2" ) ;
      rc = mthSubStr( fieldName, in, begin, limit, builder ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "mthSubStr failed:rc=%d", rc ) ;
         goto error ;
      }

      obj = builder.obj() ;
      if ( !obj.isEmpty() )
      {
         action->setObj( obj ) ;
         out = action->getObj().getField( fieldName ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB__MTHSUBSTRGET, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   ///PD_TRACE_DECLARE_FUNCTION ( SDB__MTHSUBSTRCPBUILD, "mthSubStrCPBuild" )
   INT32 mthSubStrCPBuild( const CHAR *fieldName,
                           const bson::BSONElement &e,
                           _mthSAction *action,
                           bson::BSONObjBuilder &builder )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__MTHSUBSTRCPBUILD ) ;
      SDB_ASSERT( NULL != action, "can not be null" ) ;
      INT32 begin = 0 ;
      INT32 limit = -1 ;

      begin = action->getArg().getIntField( "arg1" ) ;
      limit = action->getArg().getIntField( "arg2" ) ;
      rc = mthSubStrCP( fieldName, e, begin, limit, builder ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "mthSubStr failed:rc=%d", rc ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__MTHSUBSTRCPBUILD, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   ///PD_TRACE_DECLARE_FUNCTION ( SDB__MTHSUBSTRCPGET, "mthSubStrCPGet" )
   INT32 mthSubStrCPGet( const CHAR *fieldName,
                         const bson::BSONElement &in,
                         _mthSAction *action,
                         bson::BSONElement &out )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__MTHSUBSTRCPGET ) ;
      SDB_ASSERT( NULL != action, "can not be null" ) ;
      BSONObjBuilder builder ;
      BSONObj obj ;
      INT32 begin = 0 ;
      INT32 limit = -1 ;

      begin = action->getArg().getIntField( "arg1" ) ;
      limit = action->getArg().getIntField( "arg2" ) ;
      rc = mthSubStrCP( fieldName, in, begin, limit, builder ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "mthSubStrCP failed:rc=%d", rc ) ;
         goto error ;
      }

      obj = builder.obj() ;
      if ( !obj.isEmpty() )
      {
         action->setObj( obj ) ;
         out = action->getObj().getField( fieldName ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB__MTHSUBSTRCPGET, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   ///PD_TRACE_DECLARE_FUNCTION ( SDB__MTHSUBSTRBYTESBUILD, "mthSubStrBytesBuild" )
   INT32 mthSubStrBytesBuild( const CHAR *fieldName,
                              const bson::BSONElement &e,
                              _mthSAction *action,
                              bson::BSONObjBuilder &builder )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__MTHSUBSTRBYTESBUILD ) ;
      SDB_ASSERT( NULL != action, "can not be null" ) ;
      INT32 begin = 0 ;
      INT32 limit = -1 ;

      begin = action->getArg().getIntField( "arg1" ) ;
      limit = action->getArg().getIntField( "arg2" ) ;
      rc = mthSubStrBytes( fieldName, e, begin, limit, builder ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "mthSubStrBytes failed:rc=%d", rc ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__MTHSUBSTRBYTESBUILD, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   ///PD_TRACE_DECLARE_FUNCTION ( SDB__MTHSUBSTRBYTESGET, "mthSubStrBytesGet" )
   INT32 mthSubStrBytesGet( const CHAR *fieldName,
                            const bson::BSONElement &in,
                            _mthSAction *action,
                            bson::BSONElement &out )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__MTHSUBSTRBYTESGET ) ;
      SDB_ASSERT( NULL != action, "can not be null" ) ;
      BSONObjBuilder builder ;
      BSONObj obj ;
      INT32 begin = 0 ;
      INT32 limit = -1 ;

      begin = action->getArg().getIntField( "arg1" ) ;
      limit = action->getArg().getIntField( "arg2" ) ;
      rc = mthSubStrBytes( fieldName, in, begin, limit, builder ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "mthSubStrBytes failed:rc=%d", rc ) ;
         goto error ;
      }

      obj = builder.obj() ;
      if ( !obj.isEmpty() )
      {
         action->setObj( obj ) ;
         out = action->getObj().getField( fieldName ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB__MTHSUBSTRBYTESGET, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   ///PD_TRACE_DECLARE_FUNCTION ( SDB__MTHRIGHTCPBUILD, "mthRightCPBuild" )
   INT32 mthRightCPBuild( const CHAR *fieldName,
                          const bson::BSONElement &e,
                          _mthSAction *action,
                          bson::BSONObjBuilder &builder )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__MTHRIGHTCPBUILD ) ;
      SDB_ASSERT( NULL != action, "can not be null" ) ;
      INT32 limit = -1 ;

      limit = action->getArg().getIntField( "arg" ) ;
      rc = mthRightCP( fieldName, e, limit, builder ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "mthRightCP failed:rc=%d", rc ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__MTHRIGHTCPBUILD, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   ///PD_TRACE_DECLARE_FUNCTION ( SDB__MTHRIGHTCPGET, "mthRightCPGet" )
   INT32 mthRightCPGet( const CHAR *fieldName,
                        const bson::BSONElement &in,
                        _mthSAction *action,
                        bson::BSONElement &out )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__MTHRIGHTCPGET ) ;
      SDB_ASSERT( NULL != action, "can not be null" ) ;
      BSONObjBuilder builder ;
      BSONObj obj ;
      INT32 limit = -1 ;

      limit = action->getArg().getIntField( "arg" ) ;
      rc = mthRightCP( fieldName, in, limit, builder ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "mthRightCP failed:rc=%d", rc ) ;
         goto error ;
      }

      obj = builder.obj() ;
      if ( !obj.isEmpty() )
      {
         action->setObj( obj ) ;
         out = action->getObj().getField( fieldName ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB__MTHRIGHTCPGET, rc ) ;
      return rc ;
   error:
      goto done ;
   }


   ///PD_TRACE_DECLARE_FUNCTION ( SDB__MTHRIGHTBYTESBUILD, "mthRightBytesBuild" )
   INT32 mthRightBytesBuild( const CHAR *fieldName,
                             const bson::BSONElement &e,
                             _mthSAction *action,
                             bson::BSONObjBuilder &builder )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__MTHRIGHTBYTESBUILD ) ;
      SDB_ASSERT( NULL != action, "can not be null" ) ;
      INT32 limit = -1 ;

      limit = action->getArg().getIntField( "arg" ) ;
      rc = mthRightBytes( fieldName, e, limit, builder ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "mthRightBytes failed:rc=%d", rc ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__MTHRIGHTBYTESBUILD, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   ///PD_TRACE_DECLARE_FUNCTION ( SDB__MTHRIGHTBYTESGET, "mthRightBytesGet" )
   INT32 mthRightBytesGet( const CHAR *fieldName,
                           const bson::BSONElement &in,
                           _mthSAction *action,
                           bson::BSONElement &out )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__MTHRIGHTBYTESGET ) ;
      SDB_ASSERT( NULL != action, "can not be null" ) ;
      BSONObjBuilder builder ;
      BSONObj obj ;
      INT32 limit = -1 ;

      limit = action->getArg().getIntField( "arg" ) ;
      rc = mthRightBytes( fieldName, in, limit, builder ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "mthRightBytes failed:rc=%d", rc ) ;
         goto error ;
      }

      obj = builder.obj() ;
      if ( !obj.isEmpty() )
      {
         action->setObj( obj ) ;
         out = action->getObj().getField( fieldName ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB__MTHRIGHTBYTESGET, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   ///PD_TRACE_DECLARE_FUNCTION ( SDB__MTHLEFTCPBUILD, "mthLeftCPBuild" )
   INT32 mthLeftCPBuild( const CHAR *fieldName,
                         const bson::BSONElement &e,
                         _mthSAction *action,
                         bson::BSONObjBuilder &builder )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__MTHLEFTCPBUILD ) ;
      SDB_ASSERT( NULL != action, "can not be null" ) ;
      INT32 limit = -1 ;

      limit = action->getArg().getIntField( "arg" ) ;
      rc = mthLeftCP( fieldName, e, limit, builder ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "mthLeftCP failed:rc=%d", rc ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__MTHLEFTCPBUILD, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   ///PD_TRACE_DECLARE_FUNCTION ( SDB__MTHLEFTCPGET, "mthLeftCPGet" )
   INT32 mthLeftCPGet( const CHAR *fieldName,
                       const bson::BSONElement &in,
                       _mthSAction *action,
                       bson::BSONElement &out )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__MTHLEFTCPGET ) ;
      SDB_ASSERT( NULL != action, "can not be null" ) ;
      BSONObjBuilder builder ;
      BSONObj obj ;
      INT32 limit = -1 ;

      limit = action->getArg().getIntField( "arg" ) ;
      rc = mthLeftCP( fieldName, in, limit, builder ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "mthLeftCP failed:rc=%d", rc ) ;
         goto error ;
      }

      obj = builder.obj() ;
      if ( !obj.isEmpty() )
      {
         action->setObj( obj ) ;
         out = action->getObj().getField( fieldName ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB__MTHLEFTCPGET, rc ) ;
      return rc ;
   error:
      goto done ;
   }


   ///PD_TRACE_DECLARE_FUNCTION ( SDB__MTHLEFTBYTESBUILD, "mthLeftBytesBuild" )
   INT32 mthLeftBytesBuild( const CHAR *fieldName,
                            const bson::BSONElement &e,
                            _mthSAction *action,
                            bson::BSONObjBuilder &builder )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__MTHLEFTBYTESBUILD ) ;
      SDB_ASSERT( NULL != action, "can not be null" ) ;
      INT32 limit = -1 ;

      limit = action->getArg().getIntField( "arg" ) ;
      rc = mthLeftBytes( fieldName, e, limit, builder ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "mthLeftBytes failed:rc=%d", rc ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__MTHLEFTBYTESBUILD, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   ///PD_TRACE_DECLARE_FUNCTION ( SDB__MTHLEFTBYTESGET, "mthLeftBytesGet" )
   INT32 mthLeftBytesGet( const CHAR *fieldName,
                          const bson::BSONElement &in,
                          _mthSAction *action,
                          bson::BSONElement &out )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__MTHLEFTBYTESGET ) ;
      SDB_ASSERT( NULL != action, "can not be null" ) ;
      BSONObjBuilder builder ;
      BSONObj obj ;
      INT32 limit = -1 ;

      limit = action->getArg().getIntField( "arg" ) ;
      rc = mthLeftBytes( fieldName, in, limit, builder ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "mthLeftBytes failed:rc=%d", rc ) ;
         goto error ;
      }

      obj = builder.obj() ;
      if ( !obj.isEmpty() )
      {
         action->setObj( obj ) ;
         out = action->getObj().getField( fieldName ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB__MTHLEFTBYTESGET, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   ///PD_TRACE_DECLARE_FUNCTION ( SDB__MTHCONCATBUILD, "mthConcatBuild" )
   INT32 mthConcatBuild( const CHAR *fieldName,
                         const bson::BSONElement &e,
                         _mthSAction *action,
                         bson::BSONObjBuilder &builder )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__MTHCONCATBUILD ) ;
      SDB_ASSERT( NULL != action, "can not be null" ) ;
      const CHAR *prefix = NULL ;
      const CHAR *suffix = NULL ;
      BOOLEAN isReturnNull = FALSE ;

      prefix = action->getArg().getStringField( "arg1" ) ;
      suffix = action->getArg().getStringField( "arg2" ) ;
      isReturnNull = action->getArg().getIntField( "arg3" ) ;

      rc = mthConcat( fieldName, e, prefix, suffix, isReturnNull, builder ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "mthConcat failed:rc=%d", rc ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__MTHCONCATBUILD, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   ///PD_TRACE_DECLARE_FUNCTION ( SDB__MTHCONCATGET, "mthConcatGet" )
   INT32 mthConcatGet( const CHAR *fieldName,
                       const bson::BSONElement &in,
                       _mthSAction *action,
                       bson::BSONElement &out )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__MTHCONCATGET ) ;
      SDB_ASSERT( NULL != action, "can not be null" ) ;
      BSONObjBuilder builder ;
      BSONObj obj ;
      const CHAR *prefix = NULL ;
      const CHAR *suffix = NULL ;
      BOOLEAN isReturnNull = FALSE ;

      prefix = action->getArg().getStringField( "arg1" ) ;
      suffix = action->getArg().getStringField( "arg2" ) ;
      isReturnNull = action->getArg().getIntField( "arg3" ) ;

      rc = mthConcat( fieldName, in, prefix, suffix, isReturnNull, builder ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "mthConcat failed:rc=%d", rc ) ;
         goto error ;
      }

      obj = builder.obj() ;
      if ( !obj.isEmpty() )
      {
         action->setObj( obj ) ;
         out = action->getObj().getField( fieldName ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB__MTHCONCATGET, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   ///PD_TRACE_DECLARE_FUNCTION ( SDB__MTHDAYBUILD, "mthDayBuild" )
   INT32 mthDayBuild( const CHAR *fieldName,
                      const bson::BSONElement &e,
                      _mthSAction *action,
                      bson::BSONObjBuilder &builder )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__MTHDAYBUILD ) ;
      SDB_ASSERT( NULL != action, "can not be null" ) ;

      rc = mthDay( fieldName, e, builder ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "mthDay failed:rc=%d", rc ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__MTHDAYBUILD, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   ///PD_TRACE_DECLARE_FUNCTION ( SDB__MTHDAYGET, "mthDayGet" )
   INT32 mthDayGet( const CHAR *fieldName,
                    const bson::BSONElement &in,
                    _mthSAction *action,
                    bson::BSONElement &out )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__MTHDAYGET ) ;
      BSONObjBuilder builder ;
      BSONObj obj ;
      if ( in.eoo() )
      {
         goto done ;
      }

      rc = mthDay( fieldName, in, builder ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "mthDay failed:rc=%d", rc ) ;
         goto error ;
      }

      obj = builder.obj() ;
      if ( !obj.isEmpty() )
      {
         action->setObj( obj ) ;
         out = action->getObj().getField( fieldName ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB__MTHDAYGET, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   ///PD_TRACE_DECLARE_FUNCTION ( SDB__MTHMONTHBUILD, "mthMonthBuild" )
   INT32 mthMonthBuild( const CHAR *fieldName,
                        const bson::BSONElement &e,
                        _mthSAction *action,
                        bson::BSONObjBuilder &builder )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__MTHMONTHBUILD ) ;
      SDB_ASSERT( NULL != action, "can not be null" ) ;

      rc = mthMonth( fieldName, e, builder ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "mthMonth failed:rc=%d", rc ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__MTHMONTHBUILD, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   ///PD_TRACE_DECLARE_FUNCTION ( SDB__MTHMONTHGET, "mthMonthGet" )
   INT32 mthMonthGet( const CHAR *fieldName,
                      const bson::BSONElement &in,
                      _mthSAction *action,
                      bson::BSONElement &out )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__MTHMONTHGET ) ;
      BSONObjBuilder builder ;
      BSONObj obj ;
      if ( in.eoo() )
      {
         goto done ;
      }

      rc = mthMonth( fieldName, in, builder ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "mthMonth failed:rc=%d", rc ) ;
         goto error ;
      }

      obj = builder.obj() ;
      if ( !obj.isEmpty() )
      {
         action->setObj( obj ) ;
         out = action->getObj().getField( fieldName ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB__MTHMONTHGET, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   ///PD_TRACE_DECLARE_FUNCTION ( SDB__MTHYEARBUILD, "mthYearBuild" )
   INT32 mthYearBuild( const CHAR *fieldName,
                       const bson::BSONElement &e,
                       _mthSAction *action,
                       bson::BSONObjBuilder &builder )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__MTHYEARBUILD ) ;
      SDB_ASSERT( NULL != action, "can not be null" ) ;

      rc = mthYear( fieldName, e, builder ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "mthYear failed:rc=%d", rc ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__MTHYEARBUILD, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   ///PD_TRACE_DECLARE_FUNCTION ( SDB__MTHYEARGET, "mthYearGet" )
   INT32 mthYearGet( const CHAR *fieldName,
                     const bson::BSONElement &in,
                     _mthSAction *action,
                     bson::BSONElement &out )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__MTHYEARGET ) ;
      BSONObjBuilder builder ;
      BSONObj obj ;
      if ( in.eoo() )
      {
         goto done ;
      }

      rc = mthYear( fieldName, in, builder ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "mthYear failed:rc=%d", rc ) ;
         goto error ;
      }

      obj = builder.obj() ;
      if ( !obj.isEmpty() )
      {
         action->setObj( obj ) ;
         out = action->getObj().getField( fieldName ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB__MTHYEARGET, rc ) ;
      return rc ;
   error:
      goto done ;
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

      rc = mthStrLen( fieldName, e, builder ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "mthStrLen failed:rc=%d", rc ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__MTHSTRLENBUILD, rc ) ;
      return rc ;
   error:
      goto done ;
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

      rc = mthStrLen( fieldName, in, builder ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "mthStrLen failed:rc=%d", rc ) ;
         goto error ;
      }

      obj = builder.obj() ;
      if ( !obj.isEmpty() )
      {
         action->setObj( obj ) ;
         out = action->getObj().getField( fieldName ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB__MTHSTRLENGET, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   ///PD_TRACE_DECLARE_FUNCTION ( SDB__MTHSTRLENBYTESBUILD, "mthStrLenBytesBuild" )
   INT32 mthStrLenBytesBuild( const CHAR *fieldName,
                              const bson::BSONElement &e,
                              _mthSAction *action,
                              bson::BSONObjBuilder &builder )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__MTHSTRLENBYTESBUILD ) ;
      SDB_ASSERT( NULL != action, "can not be null" ) ;

      rc = mthStrLenBytes( fieldName, e, builder ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "mthStrLenBytes failed:rc=%d", rc ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__MTHSTRLENBYTESBUILD, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   ///PD_TRACE_DECLARE_FUNCTION ( SDB__MTHSTRLENBYTESGET, "mthStrLenBytesGet" )
   INT32 mthStrLenBytesGet( const CHAR *fieldName,
                            const bson::BSONElement &in,
                            _mthSAction *action,
                            bson::BSONElement &out )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__MTHSTRLENBYTESGET ) ;
      BSONObjBuilder builder ;
      BSONObj obj ;
      if ( in.eoo() )
      {
         goto done ;
      }

      rc = mthStrLenBytes( fieldName, in, builder ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "mthStrLenBytes failed:rc=%d", rc ) ;
         goto error ;
      }

      obj = builder.obj() ;
      if ( !obj.isEmpty() )
      {
         action->setObj( obj ) ;
         out = action->getObj().getField( fieldName ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB__MTHSTRLENBYTESGET, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   ///PD_TRACE_DECLARE_FUNCTION ( SDB__MTHSTRLENCPBUILD, "mthStrLenCPBuild" )
   INT32 mthStrLenCPBuild( const CHAR *fieldName,
                              const bson::BSONElement &e,
                              _mthSAction *action,
                              bson::BSONObjBuilder &builder )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__MTHSTRLENCPBUILD ) ;
      SDB_ASSERT( NULL != action, "can not be null" ) ;

      rc = mthStrLenCP( fieldName, e, builder ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "mthStrLenCP failed:rc=%d", rc ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__MTHSTRLENCPBUILD, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   ///PD_TRACE_DECLARE_FUNCTION ( SDB__MTHSTRLENCPGET, "mthStrLenCPGet" )
   INT32 mthStrLenCPGet( const CHAR *fieldName,
                            const bson::BSONElement &in,
                            _mthSAction *action,
                            bson::BSONElement &out )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__MTHSTRLENCPGET ) ;
      BSONObjBuilder builder ;
      BSONObj obj ;
      if ( in.eoo() )
      {
         goto done ;
      }

      rc = mthStrLenCP( fieldName, in, builder ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "mthStrLenCP failed:rc=%d", rc ) ;
         goto error ;
      }

      obj = builder.obj() ;
      if ( !obj.isEmpty() )
      {
         action->setObj( obj ) ;
         out = action->getObj().getField( fieldName ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB__MTHSTRLENCPGET, rc ) ;
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

      rc = mthLower( fieldName, e, builder ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "mthLower failed:rc=%d", rc ) ;
         goto error ;
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

      rc = mthLower( fieldName, in, builder ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "mthLower failed:rc=%d", rc ) ;
         goto error ;
      }

      obj = builder.obj() ;
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

      rc = mthUpper( fieldName, e, builder ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "mthUpper failed:rc=%d", rc ) ;
         goto error ;
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

      rc = mthUpper( fieldName, in, builder ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "mthUpper failed:rc=%d", rc ) ;
         goto error ;
      }

      obj = builder.obj() ;
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

   ///PD_TRACE_DECLARE_FUNCTION ( SDB__MTHLRTRIMBUILD, "mthLRTrimBuild" )
   static INT32 mthLRTrimBuild( const CHAR *fieldName,
                                const bson::BSONElement &e,
                                _mthSAction *action,
                                INT8 lr,
                                bson::BSONObjBuilder &builder )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__MTHLRTRIMBUILD ) ;

      rc = mthTrim( fieldName, e, lr, builder ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "mthTrim failed:rc=%d", rc ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__MTHLRTRIMBUILD, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   ///PD_TRACE_DECLARE_FUNCTION ( SDB__MTHLRTRIMGET, "mthLRTrimGet" )
   static INT32 mthLRTrimGet( const CHAR *fieldName,
                              const bson::BSONElement &in,
                              _mthSAction *action,
                              INT8 lr,
                              bson::BSONElement &out )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__MTHLRTRIMGET ) ;
      BSONObjBuilder builder ;

      rc = mthTrim( fieldName, in, lr, builder ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "mthTrim failed:rc=%d", rc ) ;
         goto error ;
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
      INT32 flag = 0 ;
      PD_TRACE_ENTRY( SDB__MTHADDBUILD ) ;
      SDB_ASSERT( NULL != action, "can not be null" ) ;
      BOOLEAN strictDataMode = action->getStrictDataMode() ;
      const BSONObj &obj = action->getArg() ;
      BSONElement arg = obj.getField( "arg1" ) ;
      SDB_ASSERT( arg.isNumber(), "must be numeric" ) ;

      rc = mthAdd( fieldName, e, arg, builder, flag ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "mthAdd failed:rc=%d", rc ) ;
         goto error ;
      }
      if ( strictDataMode && OSS_BIT_TEST( flag, MTH_OPERATION_FLAG_OVERFLOW ) )
      {
         rc = SDB_VALUE_OVERFLOW ;
         PD_LOG( PDERROR, "overflow happened, field: %s, (%lld + %lld), rc = %d",
                 fieldName, e.numberLong(), arg.numberLong(), rc ) ;
         goto error ;
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
      INT32 flag = 0 ;
      PD_TRACE_ENTRY( SDB__MTHADDGET ) ;
      BSONObjBuilder builder ;
      BSONObj obj ;
      BOOLEAN strictDataMode = action->getStrictDataMode() ;
      BSONElement arg = action->getArg().getField( "arg1" ) ;
      SDB_ASSERT( arg.isNumber(), "must be numeric" ) ;

      rc = mthAdd( fieldName, in, arg, builder, flag ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "mthAdd failed:rc=%d", rc ) ;
         goto error ;
      }
      if ( strictDataMode && OSS_BIT_TEST( flag, MTH_OPERATION_FLAG_OVERFLOW ) )
      {
         rc = SDB_VALUE_OVERFLOW ;
         PD_LOG( PDERROR, "overflow happened, field: %s, (%lld + %lld), rc = %d",
                 fieldName, in.numberLong(), arg.numberLong(), rc ) ;
         goto error ;
      }

      obj = builder.obj() ;
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
      INT32 flag = 0 ;
      PD_TRACE_ENTRY( SDB__MTHSUBTRACTBUILD ) ;
      SDB_ASSERT( NULL != action, "can not be null" ) ;
      BOOLEAN strictDataMode = action->getStrictDataMode() ;
      const BSONObj &obj = action->getArg() ;
      BSONElement arg = obj.getField( "arg1" ) ;
      SDB_ASSERT( arg.isNumber(), "must be numeric" ) ;

      rc = mthSub( fieldName, e, arg, builder, flag ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "mthSub failed:rc=%d", rc ) ;
         goto error ;
      }
      if ( strictDataMode && OSS_BIT_TEST( flag, MTH_OPERATION_FLAG_OVERFLOW ) )
      {
         rc = SDB_VALUE_OVERFLOW ;
         PD_LOG( PDERROR, "overflow happened, field: %s, (%lld - %lld), rc = %d",
                 fieldName, e.numberLong(), arg.numberLong(), rc ) ;
         goto error ;
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
      INT32 flag = 0 ;
      PD_TRACE_ENTRY( SDB__MTHSUBTRACTGET ) ;
      BSONObjBuilder builder ;
      BSONObj obj ;
      BOOLEAN strictDataMode = action->getStrictDataMode() ;
      BSONElement arg = action->getArg().getField( "arg1" ) ;
      SDB_ASSERT( arg.isNumber(), "must be numeric" ) ;

      rc = mthSub( fieldName, in, arg, builder, flag ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "mthSub failed:rc=%d", rc ) ;
         goto error ;
      }
      if ( strictDataMode && OSS_BIT_TEST( flag, MTH_OPERATION_FLAG_OVERFLOW ) )
      {
         rc = SDB_VALUE_OVERFLOW ;
         PD_LOG( PDERROR, "overflow happened, field: %s, (%lld - %lld), rc = %d",
                 fieldName, in.numberLong(), arg.numberLong(), rc ) ;
         goto error ;
      }

      obj = builder.obj() ;
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
      INT32 flag = 0 ;
      PD_TRACE_ENTRY( SDB__MTHMULTIPLYBUILD ) ;
      BOOLEAN strictDataMode = action->getStrictDataMode() ;
      const BSONObj &obj = action->getArg() ;
      BSONElement arg = obj.getField( "arg1" ) ;
      SDB_ASSERT( arg.isNumber(), "must be numeric" ) ;

      rc = mthMultiply( fieldName, e, arg, builder, flag ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "mthMultiply failed:rc=%d", rc ) ;
         goto error ;
      }
      if ( strictDataMode && OSS_BIT_TEST( flag, MTH_OPERATION_FLAG_OVERFLOW ) )
      {
         rc = SDB_VALUE_OVERFLOW ;
         PD_LOG( PDERROR, "overflow happened, field: %s, (%lld * %lld), rc = %d",
                 fieldName, e.numberLong(), arg.numberLong(), rc ) ;
         goto error ;
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
      INT32 flag = 0 ;
      PD_TRACE_ENTRY( SDB__MTHMULTIPLYGET ) ;
      BSONObjBuilder builder ;
      BSONObj obj ;
      BOOLEAN strictDataMode = action->getStrictDataMode() ;
      BSONElement arg = action->getArg().getField( "arg1" ) ;
      SDB_ASSERT( arg.isNumber(), "must be numeric" ) ;

      rc = mthMultiply( fieldName, in, arg, builder, flag ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "mthMultiply failed:rc=%d", rc ) ;
         goto error ;
      }
      if ( strictDataMode && OSS_BIT_TEST( flag, MTH_OPERATION_FLAG_OVERFLOW ) )
      {
         rc = SDB_VALUE_OVERFLOW ;
         PD_LOG( PDERROR, "overflow happened, field: %s, (%lld * %lld), rc = %d",
                 fieldName, in.numberLong(), arg.numberLong(), rc ) ;
         goto error ;
      }

      obj = builder.obj() ;
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
      INT32 flag = 0 ;
      PD_TRACE_ENTRY( SDB__MTHDIVIDEBUILD ) ;
      BOOLEAN strictDataMode = action->getStrictDataMode() ;
      const BSONObj &obj = action->getArg() ;
      BSONElement arg = obj.getField( "arg1" ) ;
      SDB_ASSERT( arg.isNumber(), "must be numeric" ) ;

      rc = mthDivide( fieldName, e, arg, builder, flag ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "mthDivide failed:rc=%d", rc ) ;
         goto error ;
      }
      if ( strictDataMode && OSS_BIT_TEST( flag, MTH_OPERATION_FLAG_OVERFLOW ) )
      {
         rc = SDB_VALUE_OVERFLOW ;
         PD_LOG( PDERROR, "overflow happened, field: %s, (%lld / %lld), rc = %d",
                 fieldName, e.numberLong(), arg.numberLong(), rc ) ;
         goto error ;
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
      INT32 flag = 0 ;
      PD_TRACE_ENTRY( SDB__MTHDIVIDEGET ) ;
      BSONObjBuilder builder ;
      BSONObj obj ;
      BOOLEAN strictDataMode = action->getStrictDataMode() ;
      BSONElement arg = action->getArg().getField( "arg1" ) ;
      SDB_ASSERT( arg.isNumber(), "must be numeric" ) ;

      rc = mthDivide( fieldName, in, arg, builder, flag ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "mthDivide failed:rc=%d", rc ) ;
         goto error ;
      }
      if ( strictDataMode && OSS_BIT_TEST( flag, MTH_OPERATION_FLAG_OVERFLOW ) )
      {
         rc = SDB_VALUE_OVERFLOW ;
         PD_LOG( PDERROR, "overflow happened, field: %s, (%lld / %lld), rc = %d",
                 fieldName, in.numberLong(), arg.numberLong(), rc ) ;
         goto error ;
      }

      obj = builder.obj() ;
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

   ///PD_TRACE_DECLARE_FUNCTION ( SDB__MTHSIZEBUILD, "mthSizeBuild" )
   INT32 mthSizeBuild( const CHAR *fieldName, const bson::BSONElement &e,
                       _mthSAction *action, bson::BSONObjBuilder &builder )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__MTHSIZEBUILD ) ;

      rc = mthSize( fieldName, e, builder ) ;
      PD_RC_CHECK( rc, PDERROR, "mthSize failed:rc=%d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__MTHSIZEBUILD, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   ///PD_TRACE_DECLARE_FUNCTION ( SDB__MTHSIZEGET, "mthSizeGet" )
   INT32 mthSizeGet( const CHAR *fieldName, const bson::BSONElement &in,
                     _mthSAction *action, bson::BSONElement &out )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__MTHSIZEGET ) ;
      BSONObjBuilder builder ;
      BSONObj obj ;

      rc = mthSize( fieldName, in, builder ) ;
      PD_RC_CHECK( rc, PDERROR, "mthSize failed:rc=%d", rc ) ;

      obj = builder.obj() ;
      if ( !obj.isEmpty() )
      {
         action->setObj( obj ) ;
         out = action->getObj().getField( fieldName ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB__MTHSIZEGET, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   ///PD_TRACE_DECLARE_FUNCTION ( SDB__MTHTYPEBUILD, "mthTypeBuild" )
   INT32 mthTypeBuild( const CHAR *fieldName, const bson::BSONElement &e,
                       _mthSAction *action, bson::BSONObjBuilder &builder )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__MTHTYPEBUILD ) ;
      INT32 resultType = 1 ;
      BSONElement arg = action->getArg().getField( "arg1" ) ;
      SDB_ASSERT( arg.isNumber(), "must be numeric" ) ;

      resultType = arg.numberInt() ;

      rc = mthType( fieldName, resultType, e, builder ) ;
      PD_RC_CHECK( rc, PDERROR, "mthType failed:rc=%d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__MTHTYPEBUILD, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   ///PD_TRACE_DECLARE_FUNCTION ( SDB__MTHTYPEGET, "mthTypeGet" )
   INT32 mthTypeGet( const CHAR *fieldName, const bson::BSONElement &in,
                     _mthSAction *action, bson::BSONElement &out )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__MTHTYPEGET ) ;
      BSONObjBuilder builder ;
      BSONObj obj ;
      INT32 resultType = 1 ;
      BSONElement arg = action->getArg().getField( "arg1" ) ;
      SDB_ASSERT( arg.isNumber(), "must be numeric" ) ;

      resultType = arg.numberInt() ;

      rc = mthType( fieldName, resultType, in, builder ) ;
      PD_RC_CHECK( rc, PDERROR, "mthType failed:rc=%d", rc ) ;

      obj = builder.obj() ;
      if ( !obj.isEmpty() )
      {
         action->setObj( obj ) ;
         out = action->getObj().getField( fieldName ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB__MTHTYPEGET, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   ///PD_TRACE_DECLARE_FUNCTION ( SDB__MTHIFNULLBUILD, "mthIfNullBuild" )
   INT32 mthIfNullBuild( const CHAR *fieldName, const bson::BSONElement &e,
                         _mthSAction *action, bson::BSONObjBuilder &builder )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__MTHIFNULLBUILD ) ;
      BSONElement arg = action->getArg().getField( "arg1" ) ;

      rc = mthIfNull( fieldName, e, arg, builder ) ;
      PD_RC_CHECK( rc, PDERROR, "mthIfNull failed:rc=%d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__MTHIFNULLBUILD, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   ///PD_TRACE_DECLARE_FUNCTION ( SDB__MTHIFNULLGET, "mthIfNullGet" )
   INT32 mthIfNullGet( const CHAR *fieldName, const bson::BSONElement &in,
                       _mthSAction *action, bson::BSONElement &out )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__MTHIFNULLGET ) ;
      BSONObjBuilder builder ;
      BSONObj obj ;
      BSONElement arg = action->getArg().getField( "arg1" ) ;

      rc = mthIfNull( fieldName, in, arg, builder ) ;
      PD_RC_CHECK( rc, PDERROR, "mthIfNull failed:rc=%d", rc ) ;

      obj = builder.obj() ;
      if ( !obj.isEmpty() )
      {
         action->setObj( obj ) ;
         out = action->getObj().getField( fieldName ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB__MTHIFNULLGET, rc ) ;
      return rc ;
   error:
      goto done ;
   }
}


