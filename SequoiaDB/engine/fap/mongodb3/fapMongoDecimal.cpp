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

   Source File Name = fapMongoDecimal.cpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains functions for agent processing.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who        Description
   ====== =========== ========== ==============================================
          2020/06/18  fangjiabin Initial Draft

   Last Changed =

*******************************************************************************/
#include "fapMongoDecimal.hpp"
#if !defined( _ARMLIN64 )
#include "../../thirdparty/intelDecimal/LIBRARY/src/bid_conf.h"
#include "../../thirdparty/intelDecimal/LIBRARY/src/bid_functions.h"
#endif

namespace fap
{
#define FAP_MONGO_CLIENT_VERSION_2      2
#define FAP_MONGO_CLIENT_VERSION_3      3
#define FAP_MONGO_CLIENT_SUBVERSION_2   2
#define FAP_MONGO_CLIENT_SUBVERSION_4   4
#define FAP_MONGO_CLIENT_FIXVERSION_12  12

   static INT32 isSdbRecordHasDecimal( const BSONObj &sdbRecord,
                                       BOOLEAN &hasDecimal )
   {
      INT32 rc = SDB_OK ;

      try
      {
         BSONObjIterator itr( sdbRecord ) ;
         while ( itr.more() )
         {
            BSONElement ele = itr.next() ;
            if ( Object == ele.type() || Array == ele.type() )
            {
               rc = isSdbRecordHasDecimal( ele.Obj(), hasDecimal ) ;
               if ( rc )
               {
                  goto error ;
               }
               if ( hasDecimal )
               {
                  goto done ;
               }
            }
            else if ( NumberDecimal == ele.type() )
            {
               hasDecimal = TRUE ;
               goto done ;
            }
         }
      }
      catch( std::bad_alloc )
      {
         rc = SDB_OOM ;
         goto error ;
      }
      catch( std::exception )
      {
         rc = SDB_SYS ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   static BOOLEAN hasMongoSpecialStr( const CHAR* decimalStr )
   {
      BOOLEAN has = FALSE ;
      INT32 len = ossStrlen( decimalStr ) ;

      if ( len == 4 && decimalStr[0] == '+' )
      {
         if ( ( decimalStr[1] == 'n' || decimalStr[1] == 'N' ) &&
              ( decimalStr[2] == 'a' || decimalStr[2] == 'A' ) &&
              ( decimalStr[3] == 'n' || decimalStr[3] == 'N' ) )
         {
            has = TRUE ;
         }
         else if ( ( decimalStr[1] == 'm' || decimalStr[1] == 'M' ) &&
                 ( decimalStr[2] == 'i' || decimalStr[2] == 'I' ) &&
                 ( decimalStr[3] == 'n' || decimalStr[3] == 'N' ) )
         {
            has = TRUE ;
         }
         else if ( ( decimalStr[1] == 'm' || decimalStr[1] == 'M' ) &&
                 ( decimalStr[2] == 'a' || decimalStr[2] == 'A' ) &&
                 ( decimalStr[3] == 'x' || decimalStr[3] == 'X' ) )
         {
            has = TRUE ;
         }
         else if ( ( decimalStr[1] == 'i' || decimalStr[1] == 'I' ) &&
                 ( decimalStr[2] == 'n' || decimalStr[2] == 'N' ) &&
                 ( decimalStr[3] == 'f' || decimalStr[3] == 'F' ) )
         {
            has = TRUE ;
         }
      }

      return has ;
   }

#if defined( _ARMLIN64 )
   // ARM64 doesn't support decimal in fap
   INT32 sdbDecimal2MongoDecimal( const BSONObj &sdbRecord,
                                  BSONObjBuilder &mongoRecordBob,
                                  BOOLEAN &hasDecimal )
   {
      INT32 rc = SDB_OK ;

      // if sdbRecord has decimal, we don't need to check.
      if ( !hasDecimal )
      {
         rc = isSdbRecordHasDecimal( sdbRecord, hasDecimal ) ;
         if ( rc )
         {
            goto error ;
         }
      }

      if ( hasDecimal )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 sdbDecimal2MongoDecimal( const BSONObj &sdbRecord,
                                  BSONArrayBuilder &mongoRecordBab,
                                  BufBuilder &mongoRecordBb )
   {
      return SDB_OK ;
   }

#else
   INT32 sdbDecimal2MongoDecimal( const BSONObj &sdbRecord,
                                  BSONObjBuilder &mongoRecordBob,
                                  BOOLEAN &hasDecimal )
   {
      INT32 rc = SDB_OK ;

      // if sdbRecord has decimal, we don't need to check.
      if ( !hasDecimal )
      {
         rc = isSdbRecordHasDecimal( sdbRecord, hasDecimal ) ;
         if ( rc )
         {
            goto error ;
         }
      }

      // After checking, if sdbRecord doesn't has decimal,
      // we don't need to convert.
      if ( !hasDecimal )
      {
         goto done ;
      }

      try
      {
         BSONObjIterator itr( sdbRecord ) ;
         while ( itr.more() )
         {
            BSONElement ele = itr.next() ;
            if ( Object == ele.type() )
            {
               BSONObjBuilder subBob(
                              mongoRecordBob.subobjStart( ele.fieldName() ) ) ;
               rc = sdbDecimal2MongoDecimal( ele.Obj(), subBob, hasDecimal ) ;
               if ( rc )
               {
                  goto error ;
               }
               subBob.doneFast() ;
            }
            else if ( Array == ele.type() )
            {
               BufBuilder &subBb = mongoRecordBob.subarrayStart(
                                   ele.fieldName() ) ;
               BSONArrayBuilder subBob( subBb ) ;
               rc = sdbDecimal2MongoDecimal( ele.Obj(), subBob, subBb ) ;
               if ( rc )
               {
                  goto error ;
               }
               subBob.doneFast() ;
            }
            else if ( NumberDecimal == ele.type() )
            {
               UINT32 signalingFlags = 0 ;
               string value = ele.numberDecimal().toString() ;
               CHAR   decimalStr[FAP_MONGO_DECIAML_STR_MAX_SIZE] = { 0 } ;
               if ( value.length() > FAP_MONGO_DECIAML_STR_MAX_SIZE )
               {
                  rc = SDB_INVALIDARG ;
                  goto error ;
               }
               ossMemcpy( decimalStr, value.c_str(), value.length() ) ;
               BID_UINT128 dec128 = bid128_from_string( decimalStr, 0,
                                                        &signalingFlags ) ;

               mongoRecordBob.bb().appendNum(
                                  (CHAR)FAP_MONGO_BSON_DECIMALBID_TYPE ) ;
               mongoRecordBob.bb().appendStr( ele.fieldName() );
               mongoRecordBob.bb().appendBuf( (void*)&dec128, sizeof(dec128) ) ;
            }
            else
            {
               mongoRecordBob.append( ele ) ;
            }
         }
      }
      catch( std::bad_alloc )
      {
         rc = SDB_OOM ;
         goto error ;
      }
      catch( std::exception )
      {
         rc = SDB_SYS ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 sdbDecimal2MongoDecimal( const BSONObj &sdbRecord,
                                  BSONArrayBuilder &mongoRecordBab,
                                  BufBuilder &mongoRecordBb )
   {
      INT32 rc = SDB_OK ;

      try
      {
         BSONObjIterator itr( sdbRecord ) ;
         while ( itr.more() )
         {
            BSONElement ele = itr.next() ;
            if ( Object == ele.type() )
            {
               BOOLEAN hasDecimal = TRUE ;
               BSONObjBuilder subBob( mongoRecordBab.subobjStart() ) ;
               rc = sdbDecimal2MongoDecimal( ele.Obj(), subBob, hasDecimal ) ;
               if ( rc )
               {
                  goto error ;
               }
               subBob.doneFast() ;
            }
            else if ( Array == ele.type() )
            {
               BufBuilder &subBb = mongoRecordBab.subarrayStart() ;
               BSONArrayBuilder subBob( subBb ) ;
               rc = sdbDecimal2MongoDecimal( ele.Obj(), subBob, subBb ) ;
               if ( rc )
               {
                  goto error ;
               }
               subBob.doneFast() ;
            }
            else if ( NumberDecimal == ele.type() )
            {
               UINT32 signalingFlags = 0 ;
               string value = ele.numberDecimal().toString() ;
               CHAR   decimalStr[FAP_MONGO_DECIAML_STR_MAX_SIZE] = { 0 } ;
               if ( value.length() > FAP_MONGO_DECIAML_STR_MAX_SIZE )
               {
                  rc = SDB_INVALIDARG ;
                  goto error ;
               }
               ossMemcpy( decimalStr, value.c_str(), value.length() ) ;
               BID_UINT128 dec128 = bid128_from_string( decimalStr, 0,
                                                        &signalingFlags ) ;

               mongoRecordBb.appendNum( (CHAR)FAP_MONGO_BSON_DECIMALBID_TYPE ) ;
               mongoRecordBb.appendStr( ele.fieldName() );
               mongoRecordBb.appendBuf( (void*)&dec128, sizeof(dec128) ) ;
            }
            else
            {
               mongoRecordBab.append( ele ) ;
            }
         }
      }
      catch( std::bad_alloc )
      {
         rc = SDB_OOM ;
         goto error ;
      }
      catch( std::exception )
      {
         rc = SDB_SYS ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }
#endif

   INT32 mongoDecimal2SdbDecimal( const BSONObj &mongoMsgObj,
                                  BSONObjBuilder &sdbMsgObjBob )
   {
      INT32 rc = SDB_OK ;

      try
      {
         mongoBSONObjIterator itr( mongoMsgObj ) ;
         while ( itr.more() )
         {
            BSONElement ele = itr.next() ;
            if ( Object == ele.type() )
            {
               BSONObjBuilder subBob( sdbMsgObjBob.subobjStart(
                                      ele.fieldName() ) ) ;
               rc = mongoDecimal2SdbDecimal( ele.Obj(), subBob ) ;
               if ( rc )
               {
                  goto error ;
               }
               subBob.doneFast() ;
            }
            else if ( Array == ele.type() )
            {
               BSONArrayBuilder subBob( sdbMsgObjBob.subarrayStart(
                                        ele.fieldName() ) ) ;
               rc = mongoDecimal2SdbDecimal( ele.Obj(), subBob ) ;
               if ( rc )
               {
                  goto error ;
               }
               subBob.doneFast() ;
            }
            else if ( FAP_MONGO_BSON_DECIMALBID_TYPE == ele.type() )
            {
#if defined( _ARMLIN64 )
               // ARM64 doesn't support decimal in fap
               rc = SDB_INVALIDARG ;
               goto error ;
#else
               UINT32 signalingFlags = 0 ;
               BID_UINT128 dec128 ;
               CHAR decimalStr[FAP_MONGO_DECIAML_STR_MAX_SIZE] = { 0 } ;
               BOOLEAN appendSucc = FALSE ;

               ossMemcpy( &dec128, ele.value(), FAP_MONGO_DECIAMLOID_SIZE ) ;
               bid128_to_string( decimalStr, dec128, &signalingFlags ) ;

               if ( hasMongoSpecialStr( decimalStr ) )
               {
                  // if the decimalStr is +NaN, +Inf, +Max or
                  // +Min( not case sensitive ), we should
                  // append NaN instead of +NaN, Inf instead of +Inf...
                  appendSucc = sdbMsgObjBob.appendDecimal( ele.fieldName(),
                                                           decimalStr+1 ) ;
               }
               else
               {
                  appendSucc = sdbMsgObjBob.appendDecimal( ele.fieldName(),
                                                           decimalStr ) ;
               }
               if ( !appendSucc )
               {
                  rc = SDB_INVALIDARG ;
                  goto error ;
               }
#endif
            }
            else
            {
               sdbMsgObjBob.append( ele ) ;
            }
         }
      }
      catch( std::bad_alloc )
      {
         rc = SDB_OOM ;
         goto error ;
      }
      catch( std::exception )
      {
         rc = SDB_SYS ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 mongoDecimal2SdbDecimal( const BSONObj &mongoMsgObj,
                                  BSONArrayBuilder &sdbMsgObjBab )
   {
      INT32 rc = SDB_OK ;

      try
      {
         mongoBSONObjIterator itr( mongoMsgObj ) ;
         while ( itr.more() )
         {
            BSONElement ele = itr.next() ;
            if ( Object == ele.type() )
            {
               BSONObjBuilder subBob( sdbMsgObjBab.subobjStart() ) ;
               rc = mongoDecimal2SdbDecimal( ele.Obj(), subBob ) ;
               if ( rc )
               {
                  goto error ;
               }
               subBob.doneFast() ;
            }
            else if ( Array == ele.type() )
            {
               BSONArrayBuilder subBob( sdbMsgObjBab.subarrayStart() ) ;
               rc = mongoDecimal2SdbDecimal( ele.Obj(), subBob ) ;
               if ( rc )
               {
                  goto error ;
               }
               subBob.doneFast() ;
            }
            else if ( FAP_MONGO_BSON_DECIMALBID_TYPE == ele.type() )
            {
#if defined( _ARMLIN64 )
               // ARM64 doesn't support decimal in fap
               rc = SDB_INVALIDARG ;
               goto error ;
#else
               bsonDecimal decimalObj ;
               UINT32 signalingFlags = 0 ;
               BID_UINT128 dec128 ;
               CHAR decimalStr[FAP_MONGO_DECIAML_STR_MAX_SIZE] = { 0 } ;

               ossMemcpy( &dec128, ele.value(), FAP_MONGO_DECIAMLOID_SIZE ) ;
               bid128_to_string( decimalStr, dec128, &signalingFlags ) ;

               if ( hasMongoSpecialStr( decimalStr ) )
               {
                  // if the decimalStr is +NaN, +Inf, +Max or
                  // +Min( not case sensitive ), we should
                  // append NaN instead of +NaN, Inf instead of +Inf...
                  rc = decimalObj.fromString( decimalStr+1 ) ;
               }
               else
               {
                  rc = decimalObj.fromString( decimalStr ) ;
               }
               if ( rc )
               {
                  goto error ;
               }
               sdbMsgObjBab.append( decimalObj ) ;
#endif
            }
            else
            {
               sdbMsgObjBab.append( ele ) ;
            }
         }
      }
      catch( std::bad_alloc )
      {
         rc = SDB_OOM ;
         goto error ;
      }
      catch( std::exception )
      {
         rc = SDB_SYS ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   // MongoDB only started to support decimal after version 3.4. So if the
   // mongo shell version < 3.4, Java drive version < 3.4 or nodejs version <
   // 2.2.12, we don't need to convert mongodb decimal to sdb decimal
   BOOLEAN mongoIsSupportDecimal( const mongoClientInfo &clientInfo )
   {
      BOOLEAN isSupportDecimal = TRUE ;

      if ( JAVA_DRIVER == clientInfo.type ||
           MONGO_SHELL == clientInfo.type )
      {
         if ( ( clientInfo.version < FAP_MONGO_CLIENT_VERSION_3 ) ||
              ( FAP_MONGO_CLIENT_VERSION_3 == clientInfo.version &&
                clientInfo.subVersion < FAP_MONGO_CLIENT_SUBVERSION_4 ) )
         {
            isSupportDecimal = FALSE ;
         }
      }
      else if ( NODEJS_DRIVER == clientInfo.type )
      {
         if ( ( clientInfo.version < FAP_MONGO_CLIENT_VERSION_2 ) ||
              ( FAP_MONGO_CLIENT_VERSION_2 == clientInfo.version &&
                clientInfo.subVersion < FAP_MONGO_CLIENT_SUBVERSION_2 ) ||
              ( FAP_MONGO_CLIENT_VERSION_2 == clientInfo.version &&
                FAP_MONGO_CLIENT_SUBVERSION_2 == clientInfo.subVersion &&
                clientInfo.fixVersion < FAP_MONGO_CLIENT_FIXVERSION_12 ) )
         {
            isSupportDecimal = FALSE ;
         }
      }

      return isSupportDecimal ;
   }

}
