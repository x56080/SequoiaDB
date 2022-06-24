/*******************************************************************************

   Copyright (C) 2011-2018 SequoiaDB Ltd.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU Affero General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.

   Source File Name = coordCommandStat.cpp

   Descriptive Name = Coord Commands

   When/how to use: this program may be used on binary and text-formatted
   versions of runtime component. This file contains code logic for
   user command processing on coordinator node.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================

   Last Changed =

*******************************************************************************/

#include "coordCommandStat.hpp"
#include "pmd.hpp"
#include "rtnCB.hpp"
#include "coordQueryOperator.hpp"
#include "msgMessage.hpp"
#include "pdTrace.hpp"
#include "coordTrace.hpp"
#include "clsMainCLMonAggregator.hpp"
#include "monDump.hpp"
#include "dmsStatUnit.hpp"

using namespace bson ;

namespace bson
{
   extern BSONObj staticNull ;
}

namespace engine
{

   /*
      _coordCMDStatisticsBase implement
   */
   _coordCMDStatisticsBase::_coordCMDStatisticsBase()
   {
   }

   _coordCMDStatisticsBase::~_coordCMDStatisticsBase()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( COORD_CMDSTATBASE_EXE, "_coordCMDStatisticsBase::execute" )
   INT32 _coordCMDStatisticsBase::execute( MsgHeader *pMsg,
                                           pmdEDUCB *cb,
                                           INT64 &contextID,
                                           rtnContextBuf *buf )
   {
      INT32 rc = SDB_OK;
      PD_TRACE_ENTRY ( COORD_CMDSTATBASE_EXE ) ;

      SDB_RTNCB *pRtncb                = pmdGetKRCB()->getRTNCB() ;

      // fill default-reply(execute success)
      contextID                        = -1 ;

      coordQueryOperator queryOpr( isReadOnly() ) ;
      rtnContextCoord *pContext = NULL ;
      coordQueryConf queryConf ;
      coordSendOptions sendOpt ;
      queryConf._openEmptyContext = openEmptyContext() ;

      const CHAR *pHint = NULL ;

      // extract request-message
      rc = msgExtractQuery( (const CHAR*)pMsg, NULL, NULL,
                            NULL, NULL, NULL, NULL,
                            NULL, &pHint );
      PD_RC_CHECK ( rc, PDERROR, "Execute failed, failed to parse query "
                    "request, rc: %d", rc ) ;

      try
      {
         BSONObj boHint( pHint ) ;
         //get collection name
         BSONElement ele = boHint.getField( FIELD_NAME_COLLECTION ) ;
         PD_CHECK ( ele.type() == String,
                    SDB_INVALIDARG, error, PDERROR,
                    "Execute failed, failed to get the field(%s)",
                    FIELD_NAME_COLLECTION ) ;
         queryConf._realCLName = ele.str() ;
      }
      catch( std::exception &e )
      {
         PD_RC_CHECK ( rc, PDERROR, "Execute failed, occured unexpected "
                       "error:%s", e.what() ) ;
      }

      if ( 0 == ossStrncmp( queryConf._realCLName.c_str(),
                            CMD_ADMIN_PREFIX SYS_VIRTUAL_CS".",
                            SYS_VIRTUAL_CS_LEN + 1 ) )
      {
         rc = _executeOnVCL( queryConf._realCLName.c_str(), cb, contextID ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Execute on VCL failed, rc: %d", rc ) ;
            goto error ;
         }
      }
      else
      {
         rc = queryOpr.init( _pResource, cb, getTimeout() ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Init query operator failed, rc: %d", rc ) ;
            goto error ;
         }

         rc = queryOpr.queryOrDoOnCL( pMsg, cb, &pContext,
                                      sendOpt, &queryConf, buf ) ;
         PD_RC_CHECK( rc, PDERROR, "Query failed, rc: %d", rc ) ;

         _cataPtr = queryOpr.getCataPtr() ;

         // statistics the result
         rc = generateResult( pContext, cb ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to execute statistics, rc: %d", rc ) ;

         contextID = pContext->contextID() ;
         pContext->reopen() ;
      }

   done:
      PD_TRACE_EXITRC ( COORD_CMDSTATBASE_EXE, rc ) ;
      return rc;
   error:
      if ( pContext )
      {
         pRtncb->contextDelete( pContext->contextID(), cb ) ;
      }
      goto done ;
   }

   INT32 _coordCMDStatisticsBase::_executeOnVCL( const CHAR *pCLName,
                                                 pmdEDUCB *cb,
                                                 INT64 &contextID )
   {
      INT32 rc = SDB_OK ;
      rtnContextCoord *pContext = NULL ;
      SDB_RTNCB *pRtncb = pmdGetKRCB()->getRTNCB() ;
      rtnQueryOptions defaultOptions ;

      if ( 0 != ossStrcmp( pCLName, CMD_ADMIN_PREFIX SYS_CL_SESSION_INFO ) )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      rc = pRtncb->contextNew( RTN_CONTEXT_COORD,
                               (rtnContext**)&pContext,
                               contextID, cb ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Create context failed, rc: %d", rc ) ;
         goto error ;
      }

      rc = pContext->open( defaultOptions, FALSE ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Open context failed, rc: %d", rc ) ;
         goto error ;
      }

      rc = generateVCLResult( pCLName, pContext, cb ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Generate VCL result failed, rc: %d", rc ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      if ( contextID != -1 )
      {
         pRtncb->contextDelete( contextID , cb ) ;
         contextID = -1 ;
      }
      goto done ;
   }

   INT32 _coordCMDStatisticsBase::generateVCLResult( const CHAR *pCLName,
                                                     rtnContext *pContext,
                                                     pmdEDUCB *cb )
   {
      return SDB_OK ;
   }

   /*
      _coordCMDGetIndexes implement
   */
   COORD_IMPLEMENT_CMD_AUTO_REGISTER( _coordCMDGetIndexes,
                                      CMD_NAME_GET_INDEXES,
                                      TRUE ) ;
   _coordCMDGetIndexes::_coordCMDGetIndexes()
   {
   }

   _coordCMDGetIndexes::~_coordCMDGetIndexes()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( COORD_GETINDEX_GENRESULT, "_coordCMDGetIndexes::generateResult" )
   INT32 _coordCMDGetIndexes::generateResult( rtnContext *pContext,
                                              pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( COORD_GETINDEX_GENRESULT ) ;

      CoordIndexMap indexMap ;
      rtnContextBuf buffObj ;

      // get index from all nodes
      while( TRUE )
      {
         rc = pContext->getMore( 1, buffObj, cb ) ;
         if ( rc )
         {
            if ( SDB_DMS_EOC == rc )
            {
               rc = SDB_OK ;
               break ;
            }
            else
            {
               PD_LOG ( PDERROR, "Failed to get index data, rc: %d", rc ) ;
               goto error ;
            }
         }

         try
         {
            BSONObj boTmp( buffObj.data() ) ;
            BSONObj boIndexDef ;
            BSONElement ele ;
            string strIndexName ;
            CoordIndexMap::iterator iter ;

            ele = boTmp.getField( IXM_FIELD_NAME_INDEX_DEF ) ;
            PD_CHECK ( ele.type() == Object, SDB_INVALIDARG, error,
                       PDERROR, "Failed to get the field(%s)",
                       IXM_FIELD_NAME_INDEX_DEF ) ;

            boIndexDef = ele.embeddedObject() ;
            ele = boIndexDef.getField( IXM_NAME_FIELD ) ;
            PD_CHECK ( ele.type() == String, SDB_INVALIDARG, error,
                       PDERROR, "Failed to get the field(%s)",
                       IXM_NAME_FIELD ) ;

            strIndexName = ele.valuestr() ;
            iter = indexMap.find( strIndexName ) ;
            if ( indexMap.end() == iter )
            {
               indexMap[ strIndexName ] = boTmp.getOwned() ;
            }
            else
            {
               // check the index
               BSONObjIterator newIter( boIndexDef ) ;
               BSONObj boOldDef ;

               ele = iter->second.getField( IXM_FIELD_NAME_INDEX_DEF ) ;
               PD_CHECK ( ele.type() == Object, SDB_INVALIDARG, error,
                          PDERROR, "Failed to get the field(%s)",
                          IXM_FIELD_NAME_INDEX_DEF ) ;
               boOldDef = ele.embeddedObject() ;

               BSONElement beTmp1, beTmp2 ;
               while( newIter.more() )
               {
                  beTmp1 = newIter.next() ;
                  if ( 0 == ossStrcmp( beTmp1.fieldName(), "_id" ) )
                  {
                     continue ;
                  }
                  beTmp2 = boOldDef.getField( beTmp1.fieldName() ) ;
                  if ( 0 != beTmp1.woCompare( beTmp2 ) )
                  {
                     PD_LOG( PDWARNING, "Corrupted index(name:%s, define1:%s, "
                             "define2:%s)", strIndexName.c_str(),
                             boIndexDef.toString().c_str(),
                             boOldDef.toString().c_str() ) ;
                     break ;
                  }
               }
            }
         }
         catch ( std::exception &e )
         {
            PD_RC_CHECK( rc, PDERROR, "Failed to get index, occured unexpected"
                         "error:%s", e.what() ) ;
         }
      }

      {
         CoordIndexMap::iterator iterMap = indexMap.begin();
         while( iterMap != indexMap.end() )
         {
            rc = pContext->append( iterMap->second ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to get index, append the data "
                         "failed(rc=%d)", rc ) ;
            ++iterMap ;
         }
      }

   done:
      PD_TRACE_EXITRC ( COORD_GETINDEX_GENRESULT, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   /*
      _coordCMDGetCount implement
   */
   COORD_IMPLEMENT_CMD_AUTO_REGISTER( _coordCMDGetCount,
                                      CMD_NAME_GET_COUNT,
                                      TRUE ) ;
   _coordCMDGetCount::_coordCMDGetCount()
   {
   }

   _coordCMDGetCount::~_coordCMDGetCount()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( COORD_GETCOUNT_GENRESULT, "_coordCMDGetCount::generateResult" )
   INT32 _coordCMDGetCount::generateResult( rtnContext *pContext,
                                            pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK;
      PD_TRACE_ENTRY ( COORD_GETCOUNT_GENRESULT ) ;

      SINT64 totalCount = 0 ;
      rtnContextBuf buffObj ;

      while( TRUE )
      {
         rc = pContext->getMore( 1, buffObj, cb ) ;
         if ( rc )
         {
            if ( SDB_DMS_EOC == rc )
            {
               rc = SDB_OK ;
               break ;
            }
            else
            {
               PD_LOG ( PDERROR, "Failed to generate count result"
                        "get data failed, rc: %d", rc ) ;
               goto error ;
            }
         }

         try
         {
            BSONObj boTmp( buffObj.data() ) ;
            BSONElement beTotal = boTmp.getField( FIELD_NAME_TOTAL ) ;
            PD_CHECK( beTotal.isNumber(), SDB_INVALIDARG, error,
                      PDERROR, "Failed to get the field(%s)",
                      FIELD_NAME_TOTAL ) ;
            totalCount += beTotal.number() ;
         }
         catch ( std::exception &e )
         {
            rc = SDB_SYS ;
            PD_RC_CHECK( rc, PDERROR, "Failed to generate count result,"
                         "occured unexpected error:%s", e.what() );
         }
      }

      try
      {
         rc = pContext->append( BSON( FIELD_NAME_TOTAL << totalCount ) ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to generate count result,"
                      "append the data failed, rc: %d", rc ) ;
      }
      catch ( std::exception &e )
      {
         rc = SDB_SYS ;
         PD_RC_CHECK( rc, PDERROR, "Failed to generate count result,"
                      "occured unexpected error:%s", e.what() ) ;
      }

   done:
      PD_TRACE_EXITRC ( COORD_GETCOUNT_GENRESULT, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   INT32 _coordCMDGetCount::generateVCLResult( const CHAR *pCLName,
                                               rtnContext *pContext,
                                               pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      rc = pContext->append( BSON( FIELD_NAME_TOTAL << (INT64)1 ) ) ;
      return rc ;
   }

   /*
      _coordCMDGetDatablocks implement
   */
   COORD_IMPLEMENT_CMD_AUTO_REGISTER( _coordCMDGetDatablocks,
                                      CMD_NAME_GET_DATABLOCKS,
                                      TRUE ) ;
   _coordCMDGetDatablocks::_coordCMDGetDatablocks()
   {
   }

   _coordCMDGetDatablocks::~_coordCMDGetDatablocks()
   {
   }

   INT32 _coordCMDGetDatablocks::generateResult( rtnContext *pContext,
                                                 pmdEDUCB *cb )
   {
      // don't merge data, do nothing
      return SDB_OK ;
   }

   /*
      _coordCMDGetQueryMeta implement
   */
   COORD_IMPLEMENT_CMD_AUTO_REGISTER( _coordCMDGetQueryMeta,
                                      CMD_NAME_GET_QUERYMETA,
                                      TRUE ) ;
   _coordCMDGetQueryMeta::_coordCMDGetQueryMeta()
   {
   }

   _coordCMDGetQueryMeta::~_coordCMDGetQueryMeta()
   {
   }

   /*
      _coordCMDGetCollectionDetail implement
   */
   COORD_IMPLEMENT_CMD_AUTO_REGISTER( _coordCMDGetCollectionDetail,
                                      CMD_NAME_GET_CL_DETAIL,
                                      TRUE ) ;
   _coordCMDGetCollectionDetail::_coordCMDGetCollectionDetail()
   {
   }

   _coordCMDGetCollectionDetail::~_coordCMDGetCollectionDetail()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( COORD_GET_CL_DETAIL_GENRESULT, "_coordCMDGetCollectionDetail::generateResult" )
   INT32 _coordCMDGetCollectionDetail::generateResult( rtnContext *pContext,
                                                       pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK;
      PD_TRACE_ENTRY ( COORD_GET_CL_DETAIL_GENRESULT ) ;

      ossPoolMap< ossPoolString, clsMainCLMonInfo* > groupName2MainCLInfo ;
      ossPoolMap< ossPoolString, clsMainCLMonInfo* >::iterator iter ;
      ossPoolMap< ossPoolString, ossPoolString > groupName2NodeName ;
      ossPoolMap< ossPoolString, ossPoolString >::iterator nameIter ;

      clsMainCLMonInfo *pNewInfo = NULL ;
      BSONObjBuilder builder ;

      try
      {
         rtnContextBuf buffObj ;
         CHAR clFullName[ DMS_COLLECTION_FULL_NAME_SZ + 1 ] = { 0 } ;
         CHAR *separator = NULL ;

         if ( !_cataPtr->isMainCL() )
         {
            rc = SDB_OK ;
            goto done ;
         }

         // Aggregate the main collection detail. One group one record.
         while ( TRUE )
         {
            rc = pContext->getMore( 1, buffObj, cb ) ;
            if ( SDB_DMS_EOC == rc )
            {
               rc = SDB_OK ;
               break ;
            }
            PD_RC_CHECK( rc, PDERROR, "Failed to get more detail, rc: %d", rc ) ;

            BSONObj boTmp( buffObj.data() ) ;
            BSONObj boDetails = boTmp.getField( FIELD_NAME_DETAILS ).Obj() ;
            ossPoolString groupName ;
            ossPoolString nodeName ;

            monCollection subMonCL ;
            BSONObjIterator it( boDetails ) ;
            INT32 i = 0 ;
            while ( it.more() )
            {
               BSONObj obj = it.next().embeddedObject() ;
               groupName = obj.getField( FIELD_NAME_GROUPNAME ).valuestrsafe() ;
               nodeName = obj.getField( FIELD_NAME_NODE_NAME ).valuestrsafe() ;
               detailedInfo info ;
               rc = monDetailObj2Info( obj, info ) ;
               PD_RC_CHECK( rc, PDERROR,
                            "Failed to convert detail BSON to struct, rc: %d", rc );
               subMonCL._details[i] = info ;
               ++i ;
            }

            iter = groupName2MainCLInfo.find( groupName ) ;
            if ( iter == groupName2MainCLInfo.end() )
            {
               pNewInfo = SDB_OSS_NEW clsMainCLMonInfo( _cataPtr->getCatalogSet(), 0 ) ;
               if ( NULL == pNewInfo )
               {
                  rc = SDB_OOM ;
                  PD_RC_CHECK( rc, PDERROR, "No memory to store main cl info" ) ;
               }
               pNewInfo->append( subMonCL ) ;
               groupName2NodeName[ groupName ] = nodeName ;
               groupName2MainCLInfo[ groupName ] = pNewInfo ;
               pNewInfo = NULL ;
            }
            else
            {
               iter->second->append( subMonCL ) ;
            }
         }

         ossStrcpy( clFullName, _cataPtr->getName() ) ;
         separator = ossStrchr( clFullName, '.' ) ;

         for ( iter = groupName2MainCLInfo.begin() ;
               iter != groupName2MainCLInfo.end() ;
               ++iter )
         {
            builder.reset() ;
            builder.append( FIELD_NAME_NAME, clFullName ) ;
            builder.append( FIELD_NAME_UNIQUEID, (INT64) _cataPtr->clUniqueID() ) ;

            *separator = '\0' ;
            builder.append( FIELD_NAME_COLLECTIONSPACE, clFullName ) ;
            *separator = '.' ;

            BSONArrayBuilder subBuilder(
                  builder.subarrayStart( FIELD_NAME_DETAILS ) ) ;
            monCollection mainMonCL ;
            iter->second->get( mainMonCL ) ;
            MON_CL_DETAIL_MAP::iterator monIt = mainMonCL._details.begin() ;
            nameIter = groupName2NodeName.find( iter->first ) ;

            while ( monIt != mainMonCL._details.end() )
            {
               BSONObjBuilder detailBuilder( subBuilder.subobjStart() ) ;
               // append location info
               detailBuilder.append( FIELD_NAME_NODE_NAME, nameIter->second ) ;
               detailBuilder.append( FIELD_NAME_GROUPNAME, nameIter->first ) ;
               // details
               rc = monDetailInfo2Obj( monIt->second, monIt->first, detailBuilder ) ;
               PD_RC_CHECK( rc, PDERROR, "Failed to convert cl detail to BSON "
                            "object, rc: %d", rc ) ;
               detailBuilder.done() ;
               ++monIt ;
            }
            subBuilder.done() ;

            rc = pContext->append( builder.done() ) ;
            PD_RC_CHECK( rc, PDERROR,
                         "Failed to append cl detail to context, rc: %d", rc ) ;
         }
      }
      catch ( std::bad_alloc &ba )
      {
         rc = SDB_OOM ;
         PD_RC_CHECK( rc, PDERROR, "Out of memory when generating result" ) ;
      }
      catch ( std::exception &e )
      {
         rc = SDB_SYS ;
         PD_RC_CHECK( rc, PDERROR, "Failed to generate detail result. "
                      "Occured unexpected error:%s", e.what() ) ;
      }

   done:
      if ( pNewInfo )
      {
         SDB_OSS_DEL pNewInfo ;
      }
      for ( iter = groupName2MainCLInfo.begin() ;
            iter != groupName2MainCLInfo.end() ;
            ++iter )
      {
         SDB_OSS_DEL iter->second ;
      }
      PD_TRACE_EXITRC ( COORD_GET_CL_DETAIL_GENRESULT, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   /*
      Assistant structure to handle index statistics aggregation.
   */
   class _coordIndexStat : public utilPooledObject
   {
      public:
         static INT32 merge( const _coordIndexStat &from, _coordIndexStat &to ) ;

      public:
         _coordIndexStat() ;

         ~_coordIndexStat() {}

         INT32 fromBSON( const bson::BSONObj &obj ) ;

         INT32 toBSON( bson::BSONObj &obj ) ;

         BOOLEAN inited()
         {
            return ( _index[0] != 0 ) ;
         }

         void setCollectionFullName( const CHAR *fullName ) ;

      private:
         CHAR           _collection[ DMS_COLLECTION_FULL_NAME_SZ + 1 ] ;
         CHAR           _index[ IXM_INDEX_NAME_SIZE + 1 ] ;
         BOOLEAN        _isUnique ;
         bson::BSONObj  _keyPattern ;
         CHAR           _statTimestamp[ OSS_TIMESTAMP_STRING_LEN + 1 ] ;
         UINT32         _indexLevels ;
         UINT32         _indexPages ;
         bson::BSONObj  _distinctValNum ;
         bson::BSONObj  _minValue ;
         bson::BSONObj  _maxValue ;
         UINT64         _nullRecords ;
         UINT64         _undefRecords ;
         UINT64         _sampleRecords ;
         UINT64         _totalRecords ;
   } ;

   _coordIndexStat::_coordIndexStat()
   {
      ossMemset( _collection, 0, sizeof( _collection ) ) ;
      ossMemset( _index, 0, sizeof( _index ) ) ;
      _isUnique = FALSE ;
      ossMemset( _statTimestamp, 0, sizeof( _statTimestamp ) ) ;
      _indexLevels = 0 ;
      _indexPages = 0 ;
      _nullRecords = 0 ;
      _undefRecords = 0 ;
      _sampleRecords = 0 ;
      _totalRecords = 0 ;
   }

   void _coordIndexStat::setCollectionFullName( const CHAR *fullName )
   {
      ossStrncpy( _collection, fullName, sizeof( _collection ) ) ;
   }

   INT32 _coordIndexStat::fromBSON( const BSONObj &obj )
   {
      INT32 rc = SDB_OK ;
      BSONObjIterator iter( obj ) ;
      INT32 nullFrac = 0 ;
      INT32 undefFrac = 0 ;

      try
      {
         while ( iter.more() )
         {
            BSONElement ele = iter.next() ;
            if ( 0 == ossStrcmp( ele.fieldName(), FIELD_NAME_COLLECTION ) )
            {
               PD_CHECK( String == ele.type(), SDB_INVALIDARG, error, PDERROR,
                         "Field '" FIELD_NAME_COLLECTION "' must be string" ) ;
               ossStrncpy( _collection, ele.valuestr(),
                           sizeof( _collection ) - 1 ) ;
            }
            else if ( 0 == ossStrcmp( ele.fieldName(), FIELD_NAME_STAT_TIMESTAMP ) )
            {
               PD_CHECK( String == ele.type(), SDB_INVALIDARG, error, PDERROR,
                         "Field '" FIELD_NAME_STAT_TIMESTAMP "' must be string" ) ;
               ossStrncpy( _statTimestamp, ele.valuestr(),
                           sizeof( _statTimestamp ) - 1 ) ;
            }
            else if ( 0 == ossStrcmp( ele.fieldName(), FIELD_NAME_INDEX ) )
            {
               PD_CHECK( String == ele.type(), SDB_INVALIDARG, error, PDERROR,
                         "Field '" FIELD_NAME_INDEX "' must be string" ) ;
               ossStrncpy( _index, ele.valuestr(), sizeof( _index ) - 1 ) ;
            }
            else if ( 0 == ossStrcmp( ele.fieldName(), FIELD_NAME_TOTAL_IDX_LEVELS ) )
            {
               PD_CHECK( ele.isNumber(), SDB_INVALIDARG, error, PDERROR,
                         "Field '" FIELD_NAME_TOTAL_IDX_LEVELS "' must be number" ) ;
               _indexLevels = ele.numberInt() ;
            }
            else if ( 0 == ossStrcmp( ele.fieldName(), FIELD_NAME_TOTAL_INDEX_PAGES ) )
            {
               PD_CHECK( ele.isNumber(), SDB_INVALIDARG, error, PDERROR,
                         "Field '" FIELD_NAME_TOTAL_INDEX_PAGES "' must be number" ) ;
               _indexPages = ele.numberInt() ;
            }
            else if ( 0 == ossStrcmp( ele.fieldName(), IXM_FIELD_NAME_UNIQUE1 ) )
            {
               PD_CHECK( Bool == ele.type(), SDB_INVALIDARG, error, PDERROR,
                         "Field '" IXM_FIELD_NAME_UNIQUE1 "' must be bool" ) ;
               _isUnique = ele.boolean() ;
            }
            else if ( 0 == ossStrcmp( ele.fieldName(), FIELD_NAME_KEY_PATTERN ) )
            {
               PD_CHECK( Object == ele.type(), SDB_INVALIDARG, error, PDERROR,
                         "Field '" FIELD_NAME_KEY_PATTERN "' must be object" ) ;
               _keyPattern = ele.embeddedObject().getOwned() ;
            }
            else if ( 0 == ossStrcmp( ele.fieldName(), FIELD_NAME_SAMPLE_RECORDS ) )
            {
               PD_CHECK( ele.isNumber(), SDB_INVALIDARG, error, PDERROR,
                         "Field '" FIELD_NAME_SAMPLE_RECORDS "' must be number" ) ;
               _sampleRecords = ele.numberLong() ;
            }
            else if ( 0 == ossStrcmp( ele.fieldName(), FIELD_NAME_TOTAL_RECORDS ) )
            {
               PD_CHECK( ele.isNumber(), SDB_INVALIDARG, error, PDERROR,
                         "Field '" FIELD_NAME_TOTAL_RECORDS "' must be number" ) ;
               _totalRecords = ele.numberLong() ;
            }
            else if ( 0 == ossStrcmp( ele.fieldName(), FIELD_NAME_DISTINCT_VAL_NUM ) )
            {
               PD_CHECK( Array == ele.type(), SDB_INVALIDARG, error, PDERROR,
                         "Field '" FIELD_NAME_DISTINCT_VAL_NUM "' must be array" ) ;
               _distinctValNum = ele.embeddedObject().getOwned() ;
            }
            else if ( 0 == ossStrcmp( ele.fieldName(), FIELD_NAME_MIN_VALUE ) )
            {
               if ( Object == ele.type() )
               {
                  _minValue = ele.embeddedObject().getOwned() ;
               }
               else if ( jstNULL == ele.type() )
               {
                  _minValue = staticNull ;
               }
               else
               {
                  rc = SDB_INVALIDARG ;
                  PD_LOG( PDERROR, "Field '" FIELD_NAME_MIN_VALUE "' must be "
                          "object or null" ) ;
                  goto error ;
               }
            }
            else if ( 0 == ossStrcmp( ele.fieldName(), FIELD_NAME_MAX_VALUE ) )
            {
               if ( Object == ele.type() )
               {
                  _maxValue = ele.embeddedObject().getOwned() ;
               }
               else if ( jstNULL == ele.type() )
               {
                  _maxValue = staticNull ;
               }
               else
               {
                  rc = SDB_INVALIDARG ;
                  PD_LOG( PDERROR, "Field '" FIELD_NAME_MAX_VALUE "' must be "
                          "object or null" ) ;
                  goto error ;
               }
            }
            else if ( 0 == ossStrcmp( ele.fieldName(), FIELD_NAME_NULL_FRAC ) )
            {
               PD_CHECK( ele.isNumber(), SDB_INVALIDARG, error, PDERROR,
                         "Field '" FIELD_NAME_NULL_FRAC "' must be number" ) ;
               nullFrac = ele.numberInt() ;
            }
            else if ( 0 == ossStrcmp( ele.fieldName(), FIELD_NAME_UNDEF_FRAC ) )
            {
               PD_CHECK( ele.isNumber(), SDB_INVALIDARG, error, PDERROR,
                         "Field '" FIELD_NAME_UNDEF_FRAC "' must be number" ) ;
               undefFrac = ele.numberInt() ;
            }
         }
      }
      catch (std::exception &e)
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Unexpected exception occured: %s", e.what() ) ;
         goto error ;
      }

      _nullRecords = ( _sampleRecords * nullFrac ) / DMS_STAT_FRACTION_SCALE ;
      _undefRecords = ( _sampleRecords * undefFrac ) / DMS_STAT_FRACTION_SCALE ;
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _coordIndexStat::toBSON( BSONObj &obj )
   {
      INT32 rc = SDB_OK ;
      try
      {
         BSONObjBuilder ob ;
         ob.append( FIELD_NAME_COLLECTION, _collection ) ;
         ob.append( FIELD_NAME_INDEX, _index ) ;
         ob.appendBool( IXM_FIELD_NAME_UNIQUE1, _isUnique ) ;
         ob.append( FIELD_NAME_KEY_PATTERN, _keyPattern ) ;
         ob.append( FIELD_NAME_TOTAL_IDX_LEVELS, _indexLevels ) ;
         ob.append( FIELD_NAME_TOTAL_INDEX_PAGES, _indexPages ) ;
         ob.appendArray( FIELD_NAME_DISTINCT_VAL_NUM, _distinctValNum ) ;

         if ( !_minValue.equal( staticNull ) )
         {
            ob.append( FIELD_NAME_MIN_VALUE, _minValue ) ;
         }
         else
         {
            ob.appendNull( FIELD_NAME_MIN_VALUE ) ;
         }

         if ( !_maxValue.equal( staticNull ) )
         {
            ob.append( FIELD_NAME_MAX_VALUE, _maxValue ) ;
         }
         else
         {
            ob.appendNull( FIELD_NAME_MAX_VALUE ) ;
         }

         INT32 nullFrac = 0 ;
         INT32 undefFrac = 0 ;
         if ( _sampleRecords > 0 )
         {
            nullFrac =
                  ( _nullRecords * DMS_STAT_FRACTION_SCALE ) / _sampleRecords ;
            undefFrac =
                  ( _undefRecords * DMS_STAT_FRACTION_SCALE ) / _sampleRecords ;
         }
         ob.append( FIELD_NAME_NULL_FRAC, nullFrac ) ;
         ob.append( FIELD_NAME_UNDEF_FRAC, undefFrac ) ;

         ob.append( FIELD_NAME_SAMPLE_RECORDS, ( INT64 )_sampleRecords ) ;
         ob.append( FIELD_NAME_TOTAL_RECORDS, ( INT64 )_totalRecords ) ;
         ob.append( FIELD_NAME_STAT_TIMESTAMP, _statTimestamp ) ;
         obj = ob.obj() ;
      }
      catch ( std::bad_alloc &ba )
      {
         rc = SDB_OOM ;
         PD_LOG( PDERROR, "No memory to build BSON object" ) ;
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   // Here we merge two statistics info into one. However, some field of it
   // can't easily be counted. We'll choose the max or min instead.
   INT32 _coordIndexStat::merge( const _coordIndexStat &from, _coordIndexStat &to )
   {
      INT32 rc = SDB_OK ;
      try
      {
         // For "Collection", "Unique", "KeyPattern" and
         // "Index", they always be the same. Ignore them.
         // For "GroupName" and "NodeName", they are useless. Ignore them.

         // For "MaxValue" and "MinValue", we get the global max one and min one.
         if ( from._maxValue > to._maxValue )
         {
            to._maxValue = from._maxValue.getOwned() ;
         }
         if ( !from._minValue.equal( staticNull ) &&
              ( to._minValue.equal( staticNull ) ||
                from._minValue < to._minValue ) )
         {
            to._minValue = from._minValue.getOwned() ;
         }

         // For "StatTimestamp", we get the newest one.
         if ( ossStrcmp( from._statTimestamp, to._statTimestamp ) > 0 )
         {
            ossStrncpy( to._statTimestamp, from._statTimestamp,
                        sizeof( to._statTimestamp ) - 1 ) ;
         }

         // For "TotalIndexLevels", we get the max one.
         if ( from._indexLevels > to._indexLevels )
         {
            to._indexLevels = from._indexLevels ;
         }

         // For "NullFrac" and "UndefFrac", we count the total null records,
         // and finally calculate the fraction again.
         to._nullRecords += from._nullRecords ;
         to._undefRecords += from._undefRecords ;

         // For "TotalIndexPages", "SampleRecords", "TotalRecords", and
         // "DistinctValNum", we count the total.
         to._sampleRecords += from._sampleRecords ;
         to._totalRecords += from._totalRecords ;
         to._indexPages += from._indexPages ;
         {
            BSONObjIterator toIt( to._distinctValNum ) ;
            BSONObjIterator fromIt( from._distinctValNum ) ;
            while ( toIt.more() && fromIt.more() )
            {
               BSONElement toEle = toIt.next() ;
               BSONElement fromEle = fromIt.next() ;
               INT64 total = toEle.numberLong() + fromEle.numberLong() ;
               SDB_ASSERT( toEle.type() == NumberLong,
                           "DistinctValNum must be INT64" ) ;
               INT64 *pNum = (INT64 *) const_cast<char *>( toEle.value() ) ;
               *pNum = total ;
            }
         }
      }
      catch ( std::bad_alloc &ba )
      {
         rc = SDB_OOM ;
         PD_LOG( PDERROR, "No memory to merge index statistics" ) ;
         goto error ;
      }
      catch ( std::exception &e )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Unexpected exception occured: %e", e.what() ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   /*
      _coordCMDGetIndexStat implement
   */
   COORD_IMPLEMENT_CMD_AUTO_REGISTER( _coordCMDGetIndexStat,
                                      CMD_NAME_GET_INDEX_STAT,
                                      TRUE ) ;
   _coordCMDGetIndexStat::_coordCMDGetIndexStat()
   {
   }

   _coordCMDGetIndexStat::~_coordCMDGetIndexStat()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( COORD_GET_INDEX_STAT_GENRESULT, "_coordCMDGetIndexStat::generateResult" )
   INT32 _coordCMDGetIndexStat::generateResult( rtnContext *pContext,
                                                pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK;
      PD_TRACE_ENTRY ( COORD_GET_INDEX_STAT_GENRESULT ) ;

      rtnContextBuf buffObj ;
      _coordIndexStat resStat ;
      BSONObj obj ;

      // Aggregate all statistics into one.
      while ( TRUE )
      {
         _coordIndexStat newStat ;
         rc = pContext->getMore( 1, buffObj, cb ) ;
         if ( SDB_DMS_EOC == rc )
         {
            rc = SDB_OK ;
            break ;
         }
         PD_RC_CHECK( rc, PDERROR, "Failed to get more detail, rc: %d", rc ) ;

         {
            BSONObj boTmp( buffObj.data() ) ;
            rc = newStat.fromBSON( boTmp ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to initialize statistics "
                         "from BSON, rc: %d", rc ) ;
         }

         if ( resStat.inited() )
         {
            rc = _coordIndexStat::merge( newStat, resStat ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to merge index statistics, "
                         "rc: %d", rc ) ;
         }
         else
         {
            resStat = newStat ;
         }
      }

      if ( !resStat.inited() ) // Nothing was returned.
      {
         goto done ;
      }

      if ( _cataPtr->isMainCL() )
      {
         resStat.setCollectionFullName( _cataPtr->getName() ) ;
      }

      rc = resStat.toBSON( obj ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to convert index statistics to BSON, rc: %d", rc ) ;
      rc = pContext->append( obj ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to append cl detail to context, rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC ( COORD_GET_INDEX_STAT_GENRESULT, rc ) ;
      return rc ;
   error:
      goto done ;
   }
}

