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

   Source File Name = dpsTransVersionCtrl.cpp

   Descriptive Name = dps transaction version control

   When/how to use: this program may be used on binary and text-formatted
   versions of Data Protection component. This file contains functions for
   transaction isolation control through version control implmenetation.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          05/08/2019  Linyoub  Initial Draft

   Last Changed =

*******************************************************************************/
#include "dpsUtil.hpp"
#include "dpsTransDef.hpp"
#include "ossUtil.hpp"
#include "msgDef.hpp"
#include "pd.hpp"

using namespace bson ;
using namespace std ;

namespace engine
{

#define DPS_RECORD_FLAGS_NON_BUSINESSOP            "NonBusinessOP"
#define DPS_STATUS_SEPARATOR                       " | "

   dpsLogConfig &dpsGetGlobalLogConfig()
   {
      static dpsLogConfig g_logConfig ;
      return g_logConfig ;
   }

   const CHAR* dpsTransStatusToString( INT32 status )
   {
      const CHAR *pStr = "Unknown" ;
      switch ( status )
      {
         case DPS_TRANS_DOING :
            pStr = "Doing" ;
            break ;
         case DPS_TRANS_WAIT_COMMIT :
            pStr = "WaitCommit" ;
            break ;
         case DPS_TRANS_COMMIT :
            pStr = "Commited" ;
            break ;
         case DPS_TRANS_ROLLBACK :
            pStr = "Rollbacked" ;
            break ;
         case DPS_TRANS_DOING_INTERRUPT :
            pStr = "DoingInterrupted" ;
            break ;
         case DPS_TRANS_PRE_WAIT_COMMIT :
            pStr = "PrepareWaitCommit" ;
            break ;
         default :
            break ;
      }
      return pStr ;
   }

   INT32 dpsGetTransIDFromString( const CHAR *pStr, DPS_TRANS_ID &transID )
   {
      INT32 rc = SDB_OK ;
      INT32 len = ossStrlen( pStr ) ;
      transID.reset() ;

      if ( 22 == len )
      {
         if ( '0' == pStr[0] && ( 'x' == pStr[1] || 'X' == pStr[1] ) )
         {
            UINT32 nodeID = DPS_INVALID_TRANSID_NODEID ;
            DPS_TRANSID_SN globSN = DPS_INVALID_TRANSID_SN ;
            ossSscanf( pStr, "0x%04x%016llx", &nodeID, &globSN ) ;
            transID.setNodeID( (DPS_TRANSID_NODEID) nodeID ) ;
            transID.setSN( globSN ) ;
            goto done ;
         }
      }
      else if ( 16 == len )
      {
         if ( '0' == pStr[0] && ( 'x' == pStr[1] || 'X' == pStr[1] ) )
         {
            UINT32 nodeID = 0 ;
            DPS_TRANS_ID_V0 id = 0 ;
            ossSscanf( pStr, "0x%04x%010llx", &nodeID, &id ) ;
            id = (UINT64)nodeID << DPS_TRANSID_NODEID_SHIFT_BITS_V0 | id ;
            transID.convertFromV0( id ) ;
            goto done ;
         }
      }

      rc = SDB_INVALIDARG ;

   done:
      return rc ;
   }

   const CHAR* dpsTransIDToString( const DPS_TRANS_ID &transID,
                                   CHAR *pBuff,
                                   UINT32 bufSize )
   {
      SDB_ASSERT( pBuff && bufSize > 0, "Invalid input" ) ;

      ossSnprintf( pBuff, bufSize, "0x%04x%016llx",
                   transID.getNodeID(),
                   transID.getGlobSN() ) ;

      return pBuff ;
   }

   ossPoolString dpsTransIDToString( const DPS_TRANS_ID &transID )
   {
      CHAR tmpStr[ DPS_TRANS_STR_LEN + 1 ] = { 0 } ;
      try
      {
         return dpsTransIDToString( transID, tmpStr, DPS_TRANS_STR_LEN ) ;
      }
      catch( std::exception &e )
      {
         try
         {
            return e.what() ;
         }
         catch (...)
         {
            return "Out-of-memory" ;
         }
      }
   }

   const CHAR *dpsTransSNToString( const DPS_TRANSID_SN &transSN,
                                   CHAR *buffer,
                                   UINT32 bufferSize )
   {
      SDB_ASSERT( NULL != buffer, "buffer is invalid" ) ;
      SDB_ASSERT( bufferSize > 0, "buffer size is invalid" ) ;
      ossSnprintf( buffer, bufferSize, "%llu(0x%016llx)",
                   transSN, transSN ) ;
      return buffer ;
   }

   ossPoolString dpsTransSNToString( const DPS_TRANSID_SN &transSN )
   {
      CHAR tmpStr[ DPS_TRANS_STR_LEN + 1 ] = { 0 } ;
      return dpsTransSNToString( transSN, tmpStr, DPS_TRANS_STR_LEN ) ;
   }

   const CHAR *dpsTransSNToHEXString( const DPS_TRANSID_SN &transSN,
                                      CHAR *buffer,
                                      UINT32 bufferSize )
   {
      SDB_ASSERT( NULL != buffer, "buffer is invalid" ) ;
      SDB_ASSERT( bufferSize > 0, "buffer size is invalid" ) ;
      ossSnprintf( buffer, bufferSize, "0x%016llx",
                   transSN, transSN ) ;
      return buffer ;
   }

   ossPoolString dpsTransSNToHEXString( const DPS_TRANSID_SN &transSN )
   {
      CHAR tmpStr[ DPS_TRANS_STR_LEN + 1 ] = { 0 } ;
      return dpsTransSNToHEXString( transSN, tmpStr, DPS_TRANS_STR_LEN ) ;
   }

   const CHAR *dpsTransTimeToString( const stpLogicalTimeUS &time,
                                     CHAR *buffer,
                                     UINT32 bufferSize )
   {
      SDB_ASSERT( NULL != buffer, "buffer is invalid" ) ;
      SDB_ASSERT( bufferSize > 0, "buffer size is invalid" ) ;
      ossSnprintf( buffer, bufferSize, "%llu(0x%016llx), TE: %u",
                   time.getTime(), time.getTime(), time.getTimeError() ) ;
      return buffer ;
   }

   ossPoolString dpsTransTimeToString( const stpLogicalTimeUS &time )
   {
      CHAR tmpStr[ 2 * DPS_TRANS_STR_LEN + 1 ] = { 0 } ;
      return dpsTransTimeToString( time, tmpStr, 2 * DPS_TRANS_STR_LEN ) ;
   }

   static void _dpsAppendFlagString( CHAR *pBuffer,
                                     UINT32 bufSize,
                                     const CHAR *flagStr )
   {
      if ( 0 != *pBuffer )
      {
         ossStrncat( pBuffer, DPS_STATUS_SEPARATOR,
                     bufSize - ossStrlen( pBuffer ) ) ;
      }
      ossStrncat( pBuffer, flagStr, bufSize - ossStrlen( pBuffer ) ) ;
   }

   const CHAR* dpsTransIDAttrToString( const DPS_TRANS_ID &transID,
                                       CHAR *pBuff,
                                       UINT32 bufSize )
   {
      SDB_ASSERT( pBuff && bufSize > 0, "Invalid input" ) ;

      if ( transID.isFirstOp() )
      {
         _dpsAppendFlagString( pBuff, bufSize, "Start" ) ;
      }
      if ( transID.isAutoCommit() )
      {
         _dpsAppendFlagString( pBuff, bufSize, "AutoCommit" ) ;
      }
      if ( transID.isRollback() )
      {
         _dpsAppendFlagString( pBuff, bufSize, "Rollback" ) ;
      }
      if ( transID.isRBPending() )
      {
         _dpsAppendFlagString( pBuff, bufSize, "Pending" ) ;
      }
      if ( transID.isGlobTrans() )
      {
         _dpsAppendFlagString( pBuff, bufSize, "Global" ) ;
      }

      return pBuff ;
   }

   ossPoolString dpsTransIDAttrToString( const DPS_TRANS_ID &transID )
   {
      CHAR tmpStr[ DPS_TRANS_STR_LEN + 1 ] = { 0 } ;
      try
      {
         return dpsTransIDAttrToString( transID, tmpStr, DPS_TRANS_STR_LEN ) ;
      }
      catch( std::exception &e )
      {
         try
         {
            return e.what() ;
         }
         catch (...)
         {
            return "Out-of-memory" ;
         }
      }
   }

   INT32 dpsTransIDToBSON( const DPS_TRANS_ID &transID,
                           BSONObjBuilder &builder )
   {
      INT32 rc = SDB_OK ;

      try
      {
         // append node ID component
         builder.append( FIELD_NAME_TRANSACTION_ID_NODEID,
                         (INT32)( transID.getNodeID() ) ) ;
         // append serial number component with necessary tags,
         // e.g. global transaction
         builder.append( FIELD_NAME_TRANSACTION_ID_SN,
                         (INT64)( transID.getGlobSN() ) ) ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to build transaction into BSON format, "
                 "error: %s", e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

   done:
      return rc ;

   error:
      goto done ;
   }

   void dpsAppendFlagString( CHAR * pBuffer, INT32 bufSize,
                                 const CHAR *flagStr )
   {
      if ( 0 != *pBuffer )
      {
         ossStrncat( pBuffer, DPS_STATUS_SEPARATOR,
                     bufSize - ossStrlen( pBuffer ) ) ;
      }
      ossStrncat( pBuffer, flagStr, bufSize - ossStrlen( pBuffer ) ) ;
   }

   void dpsFlags2String( UINT16 flags, CHAR * pBuffer, INT32 bufSize )
   {
      SDB_ASSERT ( pBuffer, "pBuffer can't be NULL" ) ;
      ossMemset ( pBuffer, 0, bufSize ) ;

      // business operation
      if ( OSS_BIT_TEST( flags, DPS_FLG_NON_BS_OP ) )
      {
         dpsAppendFlagString( pBuffer, bufSize, DPS_RECORD_FLAGS_NON_BUSINESSOP ) ;
      }
   }

   INT32 dpsTransIDToBSON( const DPS_TRANS_ID &transID,
                           BSONObjBuilder &builder,
                           const CHAR *fieldName )
   {
      INT32 rc = SDB_OK ;

      SDB_ASSERT( NULL != fieldName, "field name is invalid" ) ;

      // check field name
      PD_CHECK( NULL != fieldName, SDB_INVALIDARG, error, PDERROR,
                "Failed to build transaction ID with field name, field name "
                "is invalid" ) ;

      try
      {
         // sub-object builder
         BSONObjBuilder subBuilder( builder.subobjStart( fieldName ) ) ;

         // build BSON
         rc = dpsTransIDToBSON( transID, subBuilder ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to build transaction ID into "
               "BSON format, rc: %d", rc ) ;

         // finish sub-object builder
         subBuilder.doneFast() ;

      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to build transaction into BSON format, "
                 "error: %s", e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

   done:
      return rc ;

   error:
      goto done ;
   }

   INT32 dpsTransIDFromBSON( const BSONObj &object,
                             DPS_TRANS_ID &transID )
   {
      INT32 rc = SDB_OK ;

      try
      {
         BSONElement element ;

         // get node ID component
         element = object.getField( FIELD_NAME_TRANSACTION_ID_NODEID ) ;
         // node ID component should not be empty
         PD_CHECK( EOO != element.type(), SDB_INVALIDARG, error, PDERROR,
                   "Failed to get field [%s], field does not exist",
                   FIELD_NAME_TRANSACTION_ID_NODEID ) ;
         // node ID component should be an integer
         PD_CHECK( NumberInt == element.type(), SDB_INVALIDARG, error, PDERROR,
                   "Failed to get field [%s], field should be an integer "
                   "value", FIELD_NAME_TRANSACTION_ID_NODEID ) ;

         transID.setNodeID( (DPS_TRANSID_NODEID)( element.numberInt() ) ) ;

         // get serial number component
         element = object.getField( FIELD_NAME_TRANSACTION_ID_SN ) ;
         PD_CHECK( EOO != element.type(), SDB_INVALIDARG, error, PDERROR,
                   "Failed to get field [%s], field does not exist",
                   FIELD_NAME_TRANSACTION_ID_SN ) ;
         // node ID component should be an integer
         PD_CHECK( NumberLong == element.type(), SDB_INVALIDARG, error, PDERROR,
                   "Failed to get field [%s], field should be a long value",
                   FIELD_NAME_TRANSACTION_ID_SN ) ;

         transID.setSN( (DPS_TRANSID_SN)( element.numberLong() ) ) ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to parse transaction from BSON object, "
                 "error: %s", e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   UINT64 dpsTransIDHashMod( const DPS_TRANS_ID &transID )
   {
      return transID.getGlobSN() ;
   }

   UINT32 dpsTransIDHashMod( const DPS_TRANS_ID &transID, UINT32 modSize )
   {
      return (UINT32)( dpsTransIDHashMod( transID ) % (UINT64)modSize ) ;
   }

}
