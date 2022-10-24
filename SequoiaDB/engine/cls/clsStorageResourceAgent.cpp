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

   Source File Name = clsStorageResourceAgent.cpp

   Descriptive Name =

   When/how to use:

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/26/2022  ZHY Initial Draft

   Last Changed =

*******************************************************************************/

#include "clsStorageResourceAgent.hpp"
#include "clsIndexInfo.hpp"
#include "dmsCB.hpp"
#include "ossLikely.hpp"
#include "rtn.hpp"
#include "rtnCB.hpp"
#include "utilStringView.hpp"
#include <exception>
#include <functional>
#include <memory>

namespace engine
{
   auto DEFAULT_QUERY_FUNC = []( const CHAR *pCollectionName,
                                 const BSONObj &selector,
                                 const BSONObj &matcher,
                                 const BSONObj &orderBy,
                                 const BSONObj &hint,
                                 SINT32 flags,
                                 SINT64 numToSkip,
                                 SINT64 numToReturn,
                                 IDataManagementService *dms,
                                 SINT64 &contextID,
                                 rtnContextPtr *ppContext,
                                 BOOLEAN enablePrefetch ) -> INT32 {
      pmdEDUCB *cb = pmdGetThreadEDUCB();
      SDB_DMSCB *dmsCB = dynamic_cast< SDB_DMSCB * >( dms );
      SDB_RTNCB *rtnCB = pmdGetKRCB()->getRTNCB();
      SDB_ASSERT( cb && dmsCB && rtnCB, "can not be nullptr" );
      return rtnQuery( pCollectionName, selector, matcher, orderBy, hint, flags, cb, numToSkip,
                       numToReturn, dmsCB, rtnCB, contextID, ppContext, enablePrefetch );
   };

   auto DEFAULT_GET_MORE_FUNC =
      []( SINT64 contextID, SINT32 maxNumToReturn, rtnContextBuf &buffObj ) -> INT32 {
      pmdEDUCB *cb = pmdGetThreadEDUCB();
      SDB_RTNCB *rtnCB = pmdGetKRCB()->getRTNCB();
      SDB_ASSERT( cb && rtnCB, "can not be nullptr" );
      return rtnGetMore( contextID, maxNumToReturn, buffObj, cb, rtnCB );
   };

   auto DEFAULT_KILL_CONTEXT_FUNC = []( INT32 numContexts, const INT64 *pContextIDs ) -> INT32 {
      pmdEDUCB *cb = pmdGetThreadEDUCB();
      SDB_RTNCB *rtnCB = pmdGetKRCB()->getRTNCB();
      return rtnKillContexts( numContexts, pContextIDs, cb, rtnCB );
   };

   class _clsStorageResourceAgentImpl : public clsStorageResourceAgent
   {
   public:
      _clsStorageResourceAgentImpl( IDataManagementService *dms ) : _dms( dms ) {}

      _clsStorageResourceAgentImpl(
         IDataManagementService *dms,
         const std::function< INT32( const CHAR *,
                                     const BSONObj &,
                                     const BSONObj &,
                                     const BSONObj &,
                                     const BSONObj &,
                                     SINT32,
                                     SINT64,
                                     SINT64,
                                     IDataManagementService *,
                                     SINT64 &,
                                     rtnContextPtr *,
                                     BOOLEAN ) > &queryFunc,
         const std::function< INT32( SINT64, SINT32, rtnContextBuf & ) > &getMoreFunc,
         const std::function< INT32( INT32, const INT64 * ) > &killContextFunc )
      : _queryFunc( queryFunc )
      , _getMoreFunc( getMoreFunc )
      , _killContextFunc( killContextFunc )
      , _dms( dms )
      {
      }

   public:
      virtual INT32 getIndexInfo( IExecutor *executor,
                                  const CHAR *clFullName,
                                  const CHAR *indexName,
                                  BOOLEAN withStat,
                                  CLS_INDEX_INFO_PTR &infoPtr ) override;

      virtual INT32 getIndexInfo( IExecutor *executor,
                                  utilCLUniqueID clUID,
                                  utilIdxInnerID indexInnerID,
                                  BOOLEAN withStat,
                                  CLS_INDEX_INFO_PTR &infoPtr ) override;

      virtual INT32 getIndexInfoSet( IExecutor *executor,
                                     const CHAR *clFullName,
                                     BOOLEAN withStat,
                                     CLS_INDEX_INFO_SET_PTR &indexSetPtr ) override;

      virtual INT32 getIndexInfoSet( IExecutor *executor,
                                     utilCLUniqueID clUID,
                                     BOOLEAN withStat,
                                     CLS_INDEX_INFO_SET_PTR &indexSetPtr ) override;

      virtual INT32 getCLMetaCache( IExecutor *executor,
                                    const CHAR *clFullName,
                                    BOOLEAN withCLStat,
                                    BOOLEAN withIndexInfoSet,
                                    BOOLEAN withIndexStat,
                                    clsCLMetaCachePtr &clCachePtr ) override;

      virtual INT32 getCLMetaCache( IExecutor *executor,
                                    utilCLUniqueID clUID,
                                    BOOLEAN withCLStat,
                                    BOOLEAN withIndexInfoSet,
                                    BOOLEAN withIndexStat,
                                    clsCLMetaCachePtr &clCachePtr ) override;

      virtual INT32 getCLStat( IExecutor *executor,
                               const CHAR *clFullName,
                               CLS_CL_STAT_PTR &clStatPtr ) override;

      virtual INT32 getCLStat( IExecutor *executor,
                               utilCLUniqueID clUID,
                               CLS_CL_STAT_PTR &clStatPtr ) override;

   private:
      INT32 _queryCollectionStat( IExecutor *executor, const BSONObj &matcher, INT64 &contextID );
      INT32 _queryIndexStat( IExecutor *executor, const BSONObj &matcher, INT64 &contextID );

   private:
      std::function< INT32( const CHAR *,
                            const BSONObj &,
                            const BSONObj &,
                            const BSONObj &,
                            const BSONObj &,
                            SINT32,
                            SINT64,
                            SINT64,
                            IDataManagementService *,
                            SINT64 &,
                            rtnContextPtr *,
                            BOOLEAN ) >
         _queryFunc = DEFAULT_QUERY_FUNC;

      std::function< INT32( SINT64, SINT32, rtnContextBuf & ) > _getMoreFunc = DEFAULT_GET_MORE_FUNC;

      std::function< INT32( INT32, const INT64 * ) > _killContextFunc = DEFAULT_KILL_CONTEXT_FUNC;

   private:
      IDataManagementService *_dms = nullptr;
      BSONObj _collectionHint = BSON( "" << DMS_STAT_CL_IDX_NAME );
      BSONObj _indexHint = BSON( "" << DMS_STAT_IDX_IDX_NAME );
   };
   using clsStorageResourceAgentImpl = _clsStorageResourceAgentImpl;

   std::unique_ptr< clsStorageResourceAgent > newClsStorageResourceAgentImpl(
      IDataManagementService *dms )
   {
      return std::unique_ptr< clsStorageResourceAgent >(
         SDB_OSS_NEW clsStorageResourceAgentImpl( dms ) );
   }

   std::unique_ptr< clsStorageResourceAgent > newClsStorageResourceAgentImpl(
      IDataManagementService *dms,
      const std::function< INT32( const CHAR *,
                                  const BSONObj &,
                                  const BSONObj &,
                                  const BSONObj &,
                                  const BSONObj &,
                                  SINT32,
                                  SINT64,
                                  SINT64,
                                  IDataManagementService *,
                                  SINT64 &,
                                  rtnContextPtr *,
                                  BOOLEAN ) > &queryFunc,
      const std::function< INT32( SINT64, SINT32, rtnContextBuf & ) > &getMoreFunc,
      const std::function< INT32( INT32, const INT64 * ) > &killContextFunc )
   {
      return std::unique_ptr< clsStorageResourceAgent >(
         SDB_OSS_NEW clsStorageResourceAgentImpl( dms, queryFunc, getMoreFunc, killContextFunc ) );
   }

   INT32 _clsStorageResourceAgentImpl::getIndexInfo( IExecutor *executor,
                                                     const CHAR *clFullName,
                                                     const CHAR *indexName,
                                                     BOOLEAN withStat,
                                                     CLS_INDEX_INFO_PTR &infoPtr )
   {
      INT32 rc = SDB_OK;
      infoPtr.reset();
      ossPoolVector< BSONObj > indexes;
      dmsOpenCLOptions options;
      DATA_COLLECTION_PTR cl;
      INT64 contextID = -1;
      rc = _dms->openCL( executor, clFullName, options, cl );
      if ( OSS_UNLIKELY( SDB_OK != rc ) )
      {
         PD_LOG( PDERROR, "failed to open collection" );
         goto error;
      }
      rc = cl->listIndex( executor, indexes );
      if ( OSS_UNLIKELY( SDB_OK != rc ) )
      {
         PD_LOG( PDERROR, "failed to get index meta data" );
         goto error;
      }
      try
      {
         auto isNameEqual = [ &, indexName ]( const BSONObj &obj ) -> BOOLEAN {
            return utilStringView( indexName ) ==
                   utilStringView( obj.getStringField( IXM_NAME_FIELD ) );
         };
         ossPoolVector< BSONObj >::iterator found =
            std::find_if( indexes.begin(), indexes.end(), isNameEqual );
         if ( withStat )
         {
            CHAR csName[ DMS_COLLECTION_SPACE_NAME_SZ + 1 ] = {};
            CHAR clShortName[ DMS_COLLECTION_NAME_SZ + 1 ] = {};
            BSONObj matcher;
            rc = rtnResolveCollectionName( clFullName, ossStrlen( clFullName ), csName,
                                           DMS_COLLECTION_SPACE_NAME_SZ, clShortName,
                                           DMS_COLLECTION_NAME_SZ );
            PD_RC_CHECK( rc, PDERROR, "failed to resolve collection[%s], rc: %d", clFullName, rc );
            matcher =
               BSON( DMS_STAT_COLLECTION_SPACE << csName << DMS_STAT_COLLECTION << clShortName
                                               << DMS_STAT_IDX_INDEX << indexName );
            rc = _queryIndexStat( executor, matcher, contextID );
            PD_RC_CHECK( rc, PDERROR, "failed to query statistics" );
            rtnContextBuf contextBuf;
            rc = _getMoreFunc( contextID, 1, contextBuf );
            PD_RC_CHECK( rc, PDERROR, "failed to get query result, contextID[%d]", contextID );
            CLS_INDEX_STAT_PTR indexStatPtr = nullptr;
            rc = clsIndexStat::buildFromBson( BSONObj( contextBuf.data() ), indexStatPtr );
            PD_RC_CHECK( rc, PDERROR, "failed to build index statistics cache" );
            rc = clsIndexInfo::buildIndexInfoFromBson( *found, infoPtr );
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "failed to build index info from bson" );
               goto error;
            }
            infoPtr->setStat( indexStatPtr );
         }
         else
         {
            rc = clsIndexInfo::buildIndexInfoFromBson( *found, infoPtr );
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "failed to build index info from bson" );
               goto error;
            }
         }
      }
      catch ( std::exception &e )
      {
         PD_LOG( PDWARNING, "occur exception: %s", e.what() );
         rc = ossException2RC( &e );
         goto error;
      }

   done:
      if ( cl )
      {
         cl->close();
      }
      if ( -1 != contextID )
      {
         _killContextFunc( 1, &contextID );
      }
      return rc;
   error:
      infoPtr.reset();
      goto done;
   }

   INT32 _clsStorageResourceAgentImpl::getIndexInfo( IExecutor *executor,
                                                     utilCLUniqueID clUID,
                                                     utilIdxInnerID indexInnerID,
                                                     BOOLEAN withStat,
                                                     CLS_INDEX_INFO_PTR &infoPtr )
   {
      INT32 rc = SDB_OK;
      infoPtr.reset();
      ossPoolVector< BSONObj > indexes;
      dmsOpenCLOptions options;
      DATA_COLLECTION_PTR cl;
      INT64 contextID = -1;
      BSONObj meta;
      rc = _dms->openCL( executor, clUID, options, cl );
      if ( OSS_UNLIKELY( SDB_OK != rc ) )
      {
         PD_LOG( PDERROR, "failed to open collection" );
         goto error;
      }
      rc = cl->listIndex( executor, indexes );
      if ( OSS_UNLIKELY( SDB_OK != rc ) )
      {
         PD_LOG( PDERROR, "failed to get index meta data" );
         goto error;
      }
      rc = cl->getMetaData( executor, meta );
      if ( OSS_UNLIKELY( SDB_OK != rc ) )
      {
         PD_LOG( PDERROR, "failed to get collection meta data" );
         goto error;
      }
      try
      {
         const CHAR *clFullName = meta.getField( FIELD_NAME_NAME ).valuestrsafe();
         auto isInnerIDEqual = [ &, indexInnerID ]( const BSONObj &obj ) -> BOOLEAN {
            return indexInnerID == static_cast< UINT32 >( obj.getField( IXM_INNERID_FIELD ).Int() );
         };
         ossPoolVector< BSONObj >::iterator found =
            std::find_if( indexes.begin(), indexes.end(), isInnerIDEqual );
         const CHAR *indexName = found->getStringField( IXM_NAME_FIELD );
         if ( withStat )
         {
            CHAR csName[ DMS_COLLECTION_SPACE_NAME_SZ + 1 ] = {};
            CHAR clShortName[ DMS_COLLECTION_NAME_SZ + 1 ] = {};
            BSONObj matcher;
            rc = rtnResolveCollectionName( clFullName, ossStrlen( clFullName ), csName,
                                           DMS_COLLECTION_SPACE_NAME_SZ, clShortName,
                                           DMS_COLLECTION_NAME_SZ );
            PD_RC_CHECK( rc, PDERROR, "failed to resolve collection[%s], rc: %d", clFullName, rc );
            matcher =
               BSON( DMS_STAT_COLLECTION_SPACE << csName << DMS_STAT_COLLECTION << clShortName
                                               << DMS_STAT_IDX_INDEX << indexName );
            rc = _queryIndexStat( executor, matcher, contextID );
            PD_RC_CHECK( rc, PDERROR, "failed to query statistics" );
            rtnContextBuf contextBuf;
            rc = _getMoreFunc( contextID, 1, contextBuf );
            PD_RC_CHECK( rc, PDERROR, "failed to get query result, contextID[%d]", contextID );
            CLS_INDEX_STAT_PTR indexStatPtr = nullptr;
            rc = clsIndexStat::buildFromBson( BSONObj( contextBuf.data() ), indexStatPtr );
            PD_RC_CHECK( rc, PDERROR, "failed to build index statistics cache" );
            rc = clsIndexInfo::buildIndexInfoFromBson( *found, infoPtr );
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "failed to build index info from bson" );
               goto error;
            }
            infoPtr->setStat( indexStatPtr );
         }
         else
         {
            rc = clsIndexInfo::buildIndexInfoFromBson( *found, infoPtr );
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "failed to build index info from bson" );
               goto error;
            }
         }
      }
      catch ( std::exception &e )
      {
         PD_LOG( PDWARNING, "occur exception: %s", e.what() );
         rc = ossException2RC( &e );
         goto error;
      }

   done:
      if ( cl )
      {
         cl->close();
      }
      if ( -1 != contextID )
      {
         _killContextFunc( 1, &contextID );
      }
      return rc;
   error:
      infoPtr.reset();
      goto done;
   }

   INT32 _clsStorageResourceAgentImpl::getIndexInfoSet( IExecutor *executor,
                                                        const CHAR *clFullName,
                                                        BOOLEAN withStat,
                                                        CLS_INDEX_INFO_SET_PTR &indexSetPtr )
   {
      INT32 rc = SDB_OK;
      indexSetPtr.reset();
      ossPoolVector< BSONObj > indexes;
      dmsOpenCLOptions options;
      DATA_COLLECTION_PTR cl;
      INT64 contextID = -1;
      rc = _dms->openCL( executor, clFullName, options, cl );
      if ( OSS_UNLIKELY( SDB_OK != rc ) )
      {
         PD_LOG( PDERROR, "failed to open collection" );
         goto error;
      }
      rc = cl->listIndex( executor, indexes );
      if ( OSS_UNLIKELY( SDB_OK != rc ) )
      {
         PD_LOG( PDERROR, "failed to get index meta data" );
         goto error;
      }
      try
      {
         CLS_INDEX_INFO_SET_PTR tempInfoSetPtr = makeSharedPtrFromPool< clsIndexInfoSet >();
         if ( !tempInfoSetPtr )
         {
            PD_LOG( PDERROR, "out of memory" );
            rc = SDB_OOM;
            goto error;
         }
         ossPoolVector< CLS_INDEX_INFO_PTR > &vec = tempInfoSetPtr->getVec();
         if ( withStat )
         {
            CHAR csName[ DMS_COLLECTION_SPACE_NAME_SZ + 1 ] = {};
            CHAR clShortName[ DMS_COLLECTION_NAME_SZ + 1 ] = {};
            BSONObj matcher;
            rc = rtnResolveCollectionName( clFullName, ossStrlen( clFullName ), csName,
                                           DMS_COLLECTION_SPACE_NAME_SZ, clShortName,
                                           DMS_COLLECTION_NAME_SZ );
            PD_RC_CHECK( rc, PDERROR, "failed to resolve collection[%s], rc: %d", clFullName, rc );
            matcher =
               BSON( DMS_STAT_COLLECTION_SPACE << csName << DMS_STAT_COLLECTION << clShortName );
            rc = _queryIndexStat( executor, matcher, contextID );
            PD_RC_CHECK( rc, PDERROR, "failed to query statistics" );
            rtnContextBuf contextBuf;
            while ( TRUE )
            {
               rc = _getMoreFunc( contextID, 1, contextBuf );
               if ( SDB_DMS_EOC == rc )
               {
                  // no need to delete context because it has been deleted
                  contextID = -1;
                  rc = SDB_OK;
                  break;
               }
               PD_RC_CHECK( rc, PDWARNING, "Get more failed, rc: %d", rc );
               CLS_INDEX_STAT_PTR indexStatPtr = nullptr;
               BSONObj statObj = BSONObj( contextBuf.data() );
               const CHAR *indexName = statObj.getField( DMS_STAT_IDX_INDEX ).valuestrsafe();
               rc = clsIndexStat::buildFromBson( statObj, indexStatPtr );
               PD_RC_CHECK( rc, PDERROR, "failed to build index statistics cache" );
               auto isNameEqual = [ &, indexName ]( const BSONObj &obj ) -> BOOLEAN {
                  return utilStringView( indexName ) ==
                         utilStringView( obj.getStringField( IXM_NAME_FIELD ) );
               };
               ossPoolVector< BSONObj >::iterator found =
                  std::find_if( indexes.begin(), indexes.end(), isNameEqual );
               if ( found != indexes.end() )
               {
                  CLS_INDEX_INFO_PTR infoPtr = nullptr;
                  rc = clsIndexInfo::buildIndexInfoFromBson( *found, infoPtr );
                  if ( SDB_OK != rc )
                  {
                     PD_LOG( PDERROR, "failed to build index info from bson" );
                     goto error;
                  }
                  infoPtr->setStat( indexStatPtr );
                  vec.push_back( std::move( infoPtr ) );
                  indexes.erase( found );
               }
               // else do nothing
            }
         }
         if ( !indexes.empty() )
         {
            for ( ossPoolVector< BSONObj >::iterator it = indexes.begin(); it != indexes.end();
                  ++it )
            {
               CLS_INDEX_INFO_PTR infoPtr = nullptr;
               rc = clsIndexInfo::buildIndexInfoFromBson( *it, infoPtr );
               if ( SDB_OK != rc )
               {
                  PD_LOG( PDERROR, "failed to build index info from bson" );
                  goto error;
               }
               vec.push_back( std::move( infoPtr ) );
            }
         }
         indexSetPtr = tempInfoSetPtr;
      }
      catch ( std::exception &e )
      {
         PD_LOG( PDWARNING, "occur exception: %s", e.what() );
         rc = ossException2RC( &e );
         goto error;
      }
   done:
      if ( cl )
      {
         cl->close();
      }
      if ( -1 != contextID )
      {
         _killContextFunc( 1, &contextID );
      }
      return rc;
   error:
      indexSetPtr.reset();
      goto done;
   }

   INT32 _clsStorageResourceAgentImpl::getIndexInfoSet( IExecutor *executor,
                                                        utilCLUniqueID clUID,
                                                        BOOLEAN withStat,
                                                        CLS_INDEX_INFO_SET_PTR &infoSetPtr )
   {
      INT32 rc = SDB_OK;
      infoSetPtr.reset();
      ossPoolVector< BSONObj > indexes;
      dmsOpenCLOptions options;
      DATA_COLLECTION_PTR cl;
      INT64 contextID = -1;
      BSONObj meta;
      rc = _dms->openCL( executor, clUID, options, cl );
      if ( OSS_UNLIKELY( SDB_OK != rc ) )
      {
         PD_LOG( PDERROR, "failed to open collection" );
         goto error;
      }
      rc = cl->listIndex( executor, indexes );
      if ( OSS_UNLIKELY( SDB_OK != rc ) )
      {
         PD_LOG( PDERROR, "failed to get index meta data" );
         goto error;
      }
      rc = cl->getMetaData( executor, meta );
      if ( OSS_UNLIKELY( SDB_OK != rc ) )
      {
         PD_LOG( PDERROR, "failed to get collection meta data" );
         goto error;
      }
      try
      {
         const CHAR *clFullName = meta.getField( FIELD_NAME_NAME ).valuestrsafe();
         CLS_INDEX_INFO_SET_PTR tempInfoSetPtr = makeSharedPtrFromPool< clsIndexInfoSet >();
         if ( !tempInfoSetPtr )
         {
            PD_LOG( PDERROR, "out of memory" );
            rc = SDB_OOM;
            goto error;
         }
         ossPoolVector< CLS_INDEX_INFO_PTR > &vec = tempInfoSetPtr->getVec();
         if ( withStat )
         {
            rtnContextBuf contextBuf;
            CHAR csName[ DMS_COLLECTION_SPACE_NAME_SZ + 1 ] = {};
            CHAR clShortName[ DMS_COLLECTION_NAME_SZ + 1 ] = {};
            BSONObj matcher;
            rc = rtnResolveCollectionName( clFullName, ossStrlen( clFullName ), csName,
                                           DMS_COLLECTION_SPACE_NAME_SZ, clShortName,
                                           DMS_COLLECTION_NAME_SZ );
            PD_RC_CHECK( rc, PDERROR, "failed to resolve collection[%s], rc: %d", clFullName, rc );
            matcher =
               BSON( DMS_STAT_COLLECTION_SPACE << csName << DMS_STAT_COLLECTION << clShortName );
            rc = _queryIndexStat( executor, matcher, contextID );
            PD_RC_CHECK( rc, PDERROR, "failed to query statistics" );
            while ( TRUE )
            {
               rc = _getMoreFunc( contextID, 1, contextBuf );
               if ( SDB_DMS_EOC == rc )
               {
                  // no need to delete context because it has been deleted
                  contextID = -1;
                  rc = SDB_OK;
                  break;
               }
               PD_RC_CHECK( rc, PDWARNING, "Get more failed, rc: %d", rc );
               CLS_INDEX_STAT_PTR indexStatPtr = nullptr;
               BSONObj statObj = BSONObj( contextBuf.data() );
               const CHAR *indexName = statObj.getField( DMS_STAT_IDX_INDEX ).valuestrsafe();
               rc = clsIndexStat::buildFromBson( statObj, indexStatPtr );
               PD_RC_CHECK( rc, PDERROR, "failed to build index statistics cache" );
               auto isNameEqual = [ &, indexName ]( const BSONObj &obj ) -> BOOLEAN {
                  return utilStringView( indexName ) ==
                         utilStringView( obj.getStringField( IXM_NAME_FIELD ) );
               };
               ossPoolVector< BSONObj >::iterator found =
                  std::find_if( indexes.begin(), indexes.end(), isNameEqual );
               if ( found != indexes.end() )
               {
                  CLS_INDEX_INFO_PTR infoPtr = nullptr;
                  rc = clsIndexInfo::buildIndexInfoFromBson( *found, infoPtr );
                  if ( SDB_OK != rc )
                  {
                     PD_LOG( PDERROR, "failed to build index info from bson" );
                     goto error;
                  }
                  infoPtr->setStat( indexStatPtr );
                  vec.push_back( std::move( infoPtr ) );
                  indexes.erase( found );
               }
               // else do nothing
            }
         }
         if ( !indexes.empty() )
         {
            for ( ossPoolVector< BSONObj >::iterator it = indexes.begin(); it != indexes.end();
                  ++it )
            {
               CLS_INDEX_INFO_PTR infoPtr = nullptr;
               rc = clsIndexInfo::buildIndexInfoFromBson( *it, infoPtr );
               if ( SDB_OK != rc )
               {
                  PD_LOG( PDERROR, "failed to build index info from bson" );
                  goto error;
               }
               vec.push_back( std::move( infoPtr ) );
            }
         }
         infoSetPtr = tempInfoSetPtr;
      }
      catch ( std::exception &e )
      {
         PD_LOG( PDWARNING, "occur exception: %s", e.what() );
         rc = ossException2RC( &e );
         goto error;
      }
   done:
      if ( cl )
      {
         cl->close();
      }
      if ( -1 != contextID )
      {
         _killContextFunc( 1, &contextID );
      }
      return rc;
   error:
      infoSetPtr.reset();
      goto done;
   }

   INT32 _clsStorageResourceAgentImpl::getCLMetaCache( IExecutor *executor,
                                                       const CHAR *clFullName,
                                                       BOOLEAN withCLStat,
                                                       BOOLEAN withIndexInfoSet,
                                                       BOOLEAN withIndexStat,
                                                       clsCLMetaCachePtr &clCachePtr )
   {
      INT32 rc = SDB_OK;
      clCachePtr.reset();
      dmsOpenCLOptions options;
      DATA_COLLECTION_PTR cl;
      BSONObj meta;
      utilCLUniqueID clUID = UTIL_UNIQUEID_NULL;
      CLS_CL_STAT_PTR clStatPtr = CLS_DEFAULT_CL_STAT;
      CLS_INDEX_INFO_SET_PTR infoSetPtr = nullptr;
      INT64 clContextID = -1;
      INT64 indexContextID = -1;
      rc = _dms->openCL( executor, clFullName, options, cl );
      if ( OSS_UNLIKELY( SDB_OK != rc ) )
      {
         PD_LOG( PDERROR, "failed to open collection[%s]", clFullName );
         goto error;
      }
      rc = cl->getMetaData( executor, meta );
      if ( OSS_UNLIKELY( SDB_OK != rc ) )
      {
         PD_LOG( PDERROR, "failed to get collection[%s] meta data", clFullName );
         goto error;
      }
      clUID = meta.getField( FIELD_NAME_CL_UNIQUEID ).Long();
      if ( withCLStat )
      {
         rtnContextBuf contextBuf;
         CHAR csName[ DMS_COLLECTION_SPACE_NAME_SZ + 1 ] = {};
         CHAR clShortName[ DMS_COLLECTION_NAME_SZ + 1 ] = {};
         BSONObj matcher;
         rc = rtnResolveCollectionName( clFullName, ossStrlen( clFullName ), csName,
                                        DMS_COLLECTION_SPACE_NAME_SZ, clShortName,
                                        DMS_COLLECTION_NAME_SZ );
         PD_RC_CHECK( rc, PDERROR, "failed to resolve collection[%s], rc: %d", clFullName, rc );
         matcher =
            BSON( DMS_STAT_COLLECTION_SPACE << csName << DMS_STAT_COLLECTION << clShortName );
         rc = _queryCollectionStat( executor, matcher, clContextID );
         PD_RC_CHECK( rc, PDERROR, "failed to fetch collection[%s] statistics", clFullName );
         rc = _getMoreFunc( clContextID, 1, contextBuf );
         PD_RC_CHECK( rc, PDERROR, "failed to get query result, contextID[%d]", clContextID );
         CLS_CL_STAT_PTR tempPtr = nullptr;
         rc = clsCLStat::buildFromBson( BSONObj( contextBuf.data() ), tempPtr );
         PD_RC_CHECK( rc, PDERROR, "failed to build index statistics cache" );
         clStatPtr = tempPtr;
      }
      if ( withIndexInfoSet )
      {
         try
         {
            ossPoolVector< BSONObj > indexes;
            CLS_INDEX_INFO_SET_PTR tempInfoSetPtr = makeSharedPtrFromPool< clsIndexInfoSet >();
            if ( !tempInfoSetPtr )
            {
               PD_LOG( PDERROR, "out of memory" );
               rc = SDB_OOM;
               goto error;
            }
            ossPoolVector< CLS_INDEX_INFO_PTR > &vec = tempInfoSetPtr->getVec();
            rc = cl->listIndex( executor, indexes );
            if ( OSS_UNLIKELY( SDB_OK != rc ) )
            {
               PD_LOG( PDERROR, "failed to get index meta data" );
               goto error;
            }
            if ( withIndexStat )
            {
               rtnContextBuf contextBuf;
               CHAR csName[ DMS_COLLECTION_SPACE_NAME_SZ + 1 ] = {};
               CHAR clShortName[ DMS_COLLECTION_NAME_SZ + 1 ] = {};
               BSONObj matcher;
               rc = rtnResolveCollectionName( clFullName, ossStrlen( clFullName ), csName,
                                              DMS_COLLECTION_SPACE_NAME_SZ, clShortName,
                                              DMS_COLLECTION_NAME_SZ );
               PD_RC_CHECK( rc, PDERROR, "failed to resolve collection[%s], rc: %d", clFullName,
                            rc );
               matcher =
                  BSON( DMS_STAT_COLLECTION_SPACE << csName << DMS_STAT_COLLECTION << clShortName );
               rc = _queryIndexStat( executor, matcher, indexContextID );
               PD_RC_CHECK( rc, PDERROR, "failed to query statistics" );

               while ( TRUE )
               {
                  rc = _getMoreFunc( indexContextID, 1, contextBuf );
                  if ( SDB_DMS_EOC == rc )
                  {
                     // no need to delete context because it has been deleted
                     indexContextID = -1;
                     rc = SDB_OK;
                     break;
                  }
                  PD_RC_CHECK( rc, PDWARNING, "Get more failed, rc: %d", rc );
                  CLS_INDEX_STAT_PTR indexStatPtr = nullptr;
                  BSONObj statObj = BSONObj( contextBuf.data() );
                  const CHAR *indexName = statObj.getField( DMS_STAT_IDX_INDEX ).valuestrsafe();
                  if ( !( *indexName ) )
                  {
                     PD_LOG( PDERROR, "index name can not be null" );
                     rc = SDB_SYS;
                     goto error;
                  }
                  rc = clsIndexStat::buildFromBson( statObj, indexStatPtr );
                  PD_RC_CHECK( rc, PDERROR, "failed to build index statistics cache" );
                  auto isNameEqual = [ &, indexName ]( const BSONObj &obj ) -> BOOLEAN {
                     return utilStringView( indexName ) ==
                            utilStringView( obj.getStringField( IXM_NAME_FIELD ) );
                  };
                  ossPoolVector< BSONObj >::iterator found =
                     std::find_if( indexes.begin(), indexes.end(), isNameEqual );
                  if ( found != indexes.end() )
                  {
                     CLS_INDEX_INFO_PTR infoPtr = nullptr;
                     rc = clsIndexInfo::buildIndexInfoFromBson( *found, infoPtr );
                     if ( SDB_OK != rc )
                     {
                        PD_LOG( PDERROR, "failed to build index info from bson" );
                        goto error;
                     }
                     infoPtr->setStat( indexStatPtr );
                     vec.push_back( std::move( infoPtr ) );
                     indexes.erase( found );
                  }
                  // else do nothing
               }
            }
            if ( !indexes.empty() )
            {
               for ( ossPoolVector< BSONObj >::iterator it = indexes.begin(); it != indexes.end();
                     ++it )
               {
                  CLS_INDEX_INFO_PTR infoPtr = nullptr;
                  rc = clsIndexInfo::buildIndexInfoFromBson( *it, infoPtr );
                  if ( SDB_OK != rc )
                  {
                     PD_LOG( PDERROR, "failed to build index info from bson" );
                     goto error;
                  }
                  vec.push_back( std::move( infoPtr ) );
               }
            }
            infoSetPtr = tempInfoSetPtr;
         }
         catch ( std::exception &e )
         {
            PD_LOG( PDWARNING, "occur exception: %s", e.what() );
            rc = ossException2RC( &e );
            goto error;
         }
      }
      clCachePtr = makeSharedPtrFromPool< clsCLMetaCache >();
      if ( !clCachePtr )
      {
         rc = SDB_OOM;
         PD_LOG( PDERROR, "out of memory" );
         goto error;
      }
      rc = clCachePtr->init( clFullName, clUID, clStatPtr, std::move( infoSetPtr ) );
      PD_RC_CHECK( rc, PDERROR, "failed to initialize collection[%s] meta cache", clFullName );
   done:
      if ( cl )
      {
         cl->close();
      }
      if ( -1 != clContextID )
      {
         _killContextFunc( 1, &clContextID );
      }
      if ( -1 != indexContextID )
      {
         _killContextFunc( 1, &indexContextID );
      }
      return rc;
   error:
      clCachePtr.reset();
      goto done;
   }

   INT32 _clsStorageResourceAgentImpl::getCLMetaCache( IExecutor *executor,
                                                       utilCLUniqueID clUID,
                                                       BOOLEAN withCLStat,
                                                       BOOLEAN withIndexInfoSet,
                                                       BOOLEAN withIndexStat,
                                                       clsCLMetaCachePtr &clCachePtr )
   {
      INT32 rc = SDB_OK;
      clCachePtr.reset();
      ossPoolVector< BSONObj > indexes;
      dmsOpenCLOptions options;
      DATA_COLLECTION_PTR cl;
      BSONObj meta;
      const CHAR *clFullName;
      CLS_CL_STAT_PTR clStatPtr = nullptr;
      CLS_INDEX_INFO_SET_PTR infoSetPtr = nullptr;
      INT64 clContextID = -1;
      INT64 indexContextID = -1;
      rc = _dms->openCL( executor, clUID, options, cl );
      if ( OSS_UNLIKELY( SDB_OK != rc ) )
      {
         PD_LOG( PDERROR, "failed to open collection" );
         goto error;
      }
      rc = cl->listIndex( executor, indexes );
      if ( OSS_UNLIKELY( SDB_OK != rc ) )
      {
         PD_LOG( PDERROR, "failed to get index meta data" );
         goto error;
      }
      rc = cl->getMetaData( executor, meta );
      if ( OSS_UNLIKELY( SDB_OK != rc ) )
      {
         PD_LOG( PDERROR, "failed to get collection meta data" );
         goto error;
      }
      clFullName = meta.getField( FIELD_NAME_NAME ).valuestrsafe();
      if ( !*clFullName )
      {
         PD_LOG( PDERROR, "failed to parse cl full name from bson" );
         rc = SDB_SYS;
         goto error;
      }
      if ( withCLStat )
      {
         CHAR csName[ DMS_COLLECTION_SPACE_NAME_SZ + 1 ] = {};
         CHAR clShortName[ DMS_COLLECTION_NAME_SZ + 1 ] = {};
         BSONObj matcher;
         rtnContextBuf contextBuf;
         rc = rtnResolveCollectionName( clFullName, ossStrlen( clFullName ), csName,
                                        DMS_COLLECTION_SPACE_NAME_SZ, clShortName,
                                        DMS_COLLECTION_NAME_SZ );
         PD_RC_CHECK( rc, PDERROR, "failed to resolve collection[%s], rc: %d", clFullName, rc );
         matcher =
            BSON( DMS_STAT_COLLECTION_SPACE << csName << DMS_STAT_COLLECTION << clShortName );
         rc = _queryCollectionStat( executor, matcher, clContextID );
         PD_RC_CHECK( rc, PDERROR, "failed to fetch collection[%s] statistics", clFullName );
         rc = _getMoreFunc( clContextID, 1, contextBuf );
         PD_RC_CHECK( rc, PDERROR, "failed to get query result, contextID[%d]", clContextID );
         CLS_CL_STAT_PTR tempPtr = nullptr;
         rc = clsCLStat::buildFromBson( BSONObj( contextBuf.data() ), tempPtr );
         PD_RC_CHECK( rc, PDERROR, "failed to build index statistics cache" );
         clStatPtr = tempPtr;
      }
      try
      {
         ossPoolVector< BSONObj > indexes;
         CLS_INDEX_INFO_SET_PTR tempInfoSetPtr = makeSharedPtrFromPool< clsIndexInfoSet >();
         if ( !tempInfoSetPtr )
         {
            PD_LOG( PDERROR, "out of memory" );
            rc = SDB_OOM;
            goto error;
         }
         ossPoolVector< CLS_INDEX_INFO_PTR > &vec = tempInfoSetPtr->getVec();
         rc = cl->listIndex( executor, indexes );
         if ( OSS_UNLIKELY( SDB_OK != rc ) )
         {
            PD_LOG( PDERROR, "failed to get index meta data" );
            goto error;
         }
         if ( withIndexStat )
         {
            CHAR csName[ DMS_COLLECTION_SPACE_NAME_SZ + 1 ] = {};
            CHAR clShortName[ DMS_COLLECTION_NAME_SZ + 1 ] = {};
            BSONObj matcher;
            rc = rtnResolveCollectionName( clFullName, ossStrlen( clFullName ), csName,
                                           DMS_COLLECTION_SPACE_NAME_SZ, clShortName,
                                           DMS_COLLECTION_NAME_SZ );
            PD_RC_CHECK( rc, PDERROR, "failed to resolve collection[%s], rc: %d", clFullName, rc );
            matcher =
               BSON( DMS_STAT_COLLECTION_SPACE << csName << DMS_STAT_COLLECTION << clShortName );
            rc = _queryIndexStat( executor, matcher, indexContextID );
            PD_RC_CHECK( rc, PDERROR, "failed to query statistics" );
            while ( TRUE )
            {
               rtnContextBuf contextBuf;
               rc = _getMoreFunc( indexContextID, 1, contextBuf );
               if ( SDB_DMS_EOC == rc )
               {
                  // no need to delete context because it has been deleted
                  indexContextID = -1;
                  rc = SDB_OK;
                  break;
               }
               PD_RC_CHECK( rc, PDWARNING, "Get more failed, rc: %d", rc );
               CLS_INDEX_STAT_PTR indexStatPtr = nullptr;
               BSONObj statObj = BSONObj( contextBuf.data() );
               const CHAR *indexName = statObj.getField( DMS_STAT_IDX_INDEX ).valuestrsafe();
               rc = clsIndexStat::buildFromBson( statObj, indexStatPtr );
               PD_RC_CHECK( rc, PDERROR, "failed to build index statistics cache" );
               auto isNameEqual = [ &, indexName ]( const BSONObj &obj ) -> BOOLEAN {
                  return utilStringView( indexName ) ==
                         utilStringView( obj.getStringField( IXM_NAME_FIELD ) );
               };
               ossPoolVector< BSONObj >::iterator found =
                  std::find_if( indexes.begin(), indexes.end(), isNameEqual );
               if ( found != indexes.end() )
               {
                  CLS_INDEX_INFO_PTR infoPtr = nullptr;
                  rc = clsIndexInfo::buildIndexInfoFromBson( *found, infoPtr );
                  if ( SDB_OK != rc )
                  {
                     PD_LOG( PDERROR, "failed to build index info from bson" );
                     goto error;
                  }
                  infoPtr->setStat( indexStatPtr );
                  vec.push_back( std::move( infoPtr ) );
                  indexes.erase( found );
               }
               // else do nothing
            }
         }
         if ( !indexes.empty() )
         {
            for ( ossPoolVector< BSONObj >::iterator it = indexes.begin(); it != indexes.end();
                  ++it )
            {
               CLS_INDEX_INFO_PTR infoPtr = nullptr;
               rc = clsIndexInfo::buildIndexInfoFromBson( *it, infoPtr );
               if ( SDB_OK != rc )
               {
                  PD_LOG( PDERROR, "failed to build index info from bson" );
                  goto error;
               }
               vec.push_back( std::move( infoPtr ) );
            }
         }
         infoSetPtr = tempInfoSetPtr;
      }
      catch ( std::exception &e )
      {
         PD_LOG( PDWARNING, "occur exception: %s", e.what() );
         rc = ossException2RC( &e );
         goto error;
      }
      clCachePtr = makeSharedPtrFromPool< clsCLMetaCache >();
      if ( !clCachePtr )
      {
         rc = SDB_OOM;
         PD_LOG( PDERROR, "out of memory" );
         goto error;
      }
      rc = clCachePtr->init( clFullName, clUID, clStatPtr, std::move( infoSetPtr ) );
      PD_RC_CHECK( rc, PDERROR, "failed to initialize collection[%s] meta cache", clFullName );
   done:
      if ( cl )
      {
         cl->close();
      }
      if ( -1 != clContextID )
      {
         _killContextFunc( 1, &clContextID );
      }
      if ( -1 != indexContextID )
      {
         _killContextFunc( 1, &indexContextID );
      }
      return rc;
   error:
      clCachePtr.reset();
      goto done;
   }

   INT32 _clsStorageResourceAgentImpl::getCLStat( IExecutor *executor,
                                                  const CHAR *clFullName,
                                                  CLS_CL_STAT_PTR &clStatPtr )
   {
      INT32 rc = SDB_OK;
      clStatPtr.reset();
      BSONObj matcher;
      CHAR csName[ DMS_COLLECTION_SPACE_NAME_SZ + 1 ] = {};
      CHAR clShortName[ DMS_COLLECTION_NAME_SZ + 1 ] = {};
      INT64 contextID = -1;
      rtnContextBuf contextBuf;
      try
      {
         rc = rtnResolveCollectionName( clFullName, ossStrlen( clFullName ), csName,
                                        DMS_COLLECTION_SPACE_NAME_SZ, clShortName,
                                        DMS_COLLECTION_NAME_SZ );
         PD_RC_CHECK( rc, PDERROR, "failed to resolve collection[%s], rc: %d", clFullName, rc );
         matcher =
            BSON( DMS_STAT_COLLECTION_SPACE << csName << DMS_STAT_COLLECTION << clShortName );
         rc = _queryCollectionStat( executor, matcher, contextID );
         PD_RC_CHECK( rc, PDERROR, "failed to fetch collection statistics" );
         rc = _getMoreFunc( contextID, 1, contextBuf );
         PD_RC_CHECK( rc, PDERROR, "failed to get query result, contextID[%d]", contextID );
         CLS_CL_STAT_PTR tempPtr = nullptr;
         rc = clsCLStat::buildFromBson( BSONObj( contextBuf.data() ), tempPtr );
         PD_RC_CHECK( rc, PDERROR, "failed to build index statistics cache" );
         clStatPtr = std::move( tempPtr );
      }
      catch ( std::exception &e )
      {
         PD_LOG( PDWARNING, "occur exception: %s", e.what() );
         rc = ossException2RC( &e );
         goto error;
      }

   done:
      if ( -1 != contextID )
      {
         _killContextFunc( 1, &contextID );
      }
      return rc;
   error:
      clStatPtr.reset();
      goto done;
   }

   INT32 _clsStorageResourceAgentImpl::getCLStat( IExecutor *executor,
                                                  utilCLUniqueID clUID,
                                                  CLS_CL_STAT_PTR &clStatPtr )
   {
      INT32 rc = SDB_OK;
      clStatPtr.reset();
      dmsOpenCLOptions options;
      DATA_COLLECTION_PTR cl;
      BSONObj meta;
      INT64 contextID = -1;
      rtnContextBuf contextBuf;
      rc = _dms->openCL( executor, clUID, options, cl );
      if ( OSS_UNLIKELY( SDB_OK != rc ) )
      {
         PD_LOG( PDERROR, "failed to open collection" );
         goto error;
      }
      rc = cl->getMetaData( executor, meta );
      if ( OSS_UNLIKELY( SDB_OK != rc ) )
      {
         PD_LOG( PDERROR, "failed to get collection meta data" );
         goto error;
      }
      try
      {
         const CHAR *clFullName = meta.getField( FIELD_NAME_NAME ).valuestrsafe();
         CHAR csName[ DMS_COLLECTION_SPACE_NAME_SZ + 1 ] = {};
         CHAR clShortName[ DMS_COLLECTION_NAME_SZ + 1 ] = {};
         BSONObj matcher;
         rc = rtnResolveCollectionName( clFullName, ossStrlen( clFullName ), csName,
                                        DMS_COLLECTION_SPACE_NAME_SZ, clShortName,
                                        DMS_COLLECTION_NAME_SZ );
         PD_RC_CHECK( rc, PDERROR, "failed to resolve collection[%s], rc: %d", clFullName, rc );
         matcher =
            BSON( DMS_STAT_COLLECTION_SPACE << csName << DMS_STAT_COLLECTION << clShortName );
         rc = _queryCollectionStat( executor, matcher, contextID );
         PD_RC_CHECK( rc, PDERROR, "failed to fetch collection statistics" );
         rc = _getMoreFunc( contextID, 1, contextBuf );
         PD_RC_CHECK( rc, PDERROR, "failed to get query result, contextID[%d]", contextID );
         CLS_CL_STAT_PTR tempPtr = nullptr;
         rc = clsCLStat::buildFromBson( BSONObj( contextBuf.data() ), tempPtr );
         PD_RC_CHECK( rc, PDERROR, "failed to build index statistics cache" );
         clStatPtr = std::move( tempPtr );
      }
      catch ( std::exception &e )
      {
         PD_LOG( PDWARNING, "occur exception: %s", e.what() );
         rc = ossException2RC( &e );
         goto error;
      }
   done:
      if ( -1 != contextID )
      {
         _killContextFunc( 1, &contextID );
      }
      return rc;
   error:
      clStatPtr.reset();
      goto done;
   }

   INT32 _clsStorageResourceAgentImpl::_queryCollectionStat( IExecutor *executor,
                                                             const BSONObj &matcher,
                                                             INT64 &contextID )
   {
      INT32 rc = SDB_OK;
      BSONObj dummy;
      contextID = -1;
      rc = _queryFunc( DMS_STAT_COLLECTION_CL_NAME, dummy, matcher, dummy, _collectionHint, 0, 0,
                       -1, _dms, contextID, nullptr, 0 );
      PD_RC_CHECK( rc, PDWARNING, "Query collection [%s] failed, rc: %d",
                   DMS_STAT_COLLECTION_CL_NAME, rc );

   done:
      return rc;
   error:
      goto done;
   }

   INT32 _clsStorageResourceAgentImpl::_queryIndexStat( IExecutor *executor,
                                                        const BSONObj &matcher,
                                                        INT64 &contextID )
   {
      INT32 rc = SDB_OK;
      BSONObj dummy;
      contextID = -1;
      rc = _queryFunc( DMS_STAT_INDEX_CL_NAME, dummy, matcher, dummy, _indexHint, 0, 0, -1, _dms,
                       contextID, nullptr, 0 );
      PD_RC_CHECK( rc, PDWARNING, "Query collection [%s] failed, rc: %d", DMS_STAT_INDEX_CL_NAME,
                   rc );

   done:
      return rc;
   error:
      goto done;
   }
} // namespace engine
