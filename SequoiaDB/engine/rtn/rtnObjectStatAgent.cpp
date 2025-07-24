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

   Source File Name = rtnObjectStatAgent.cpp

   Descriptive Name =

   When/how to use:

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/26/2022  ZHY Initial Draft
          11/24/2022  ZHY Move to rtn module
   Last Changed =

*******************************************************************************/
#include "rtnObjectStatAgent.hpp"
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

   class _rtnObjectStatAgentImpl : public rtnObjectStatAgent
   {
   public:
      class contextImpl : public rtnObjectStatAgent::contextBase
      {
      public:
         contextImpl( IDataManagementService *dms,
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

         virtual ~contextImpl();

      public:
         INT32 open( const BSONObj &matcher,
                     const BSONObj &collectionHint,
                     const BSONObj &indexHint );
         virtual INT32 fetchOne( IExecutor *executor, RTN_CL_STAT_PTR & ) override;

         virtual INT32 fetchBatch( IExecutor *executor,
                                   UINT32 batchSize,
                                   ossPoolVector< RTN_CL_STAT_PTR > & ) override;

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

         std::function< INT32( SINT64, SINT32, rtnContextBuf & ) > _getMoreFunc =
            DEFAULT_GET_MORE_FUNC;

         std::function< INT32( INT32, const INT64 * ) > _killContextFunc =
            DEFAULT_KILL_CONTEXT_FUNC;

      private:
         IDataManagementService *_dms;
         INT64 _clContextID = -1;
         INT64 _indexContextID = -1;
         rtnContextBuf _indexStatBuf;
      };

   public:
      _rtnObjectStatAgentImpl( IDataManagementService *dms ) : _dms( dms ) {}

      _rtnObjectStatAgentImpl(
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

      virtual ~_rtnObjectStatAgentImpl() = default;

   public:
      virtual INT32 getCollectionStatInfo( IExecutor *executor,
                                           const CHAR *clFullName,
                                           BOOLEAN withIndexStatInfo,
                                           RTN_CL_STAT_PTR &clStatPtr ) override;

      virtual INT32 getCollectionStatInfoOnCS( IExecutor *executor,
                                               const CHAR *csName,
                                               std::unique_ptr< contextBase > &pCtx ) override;

      virtual INT32 getAllCollectionStatInfo( IExecutor *executor,
                                              std::unique_ptr< contextBase > &pCtx ) override;

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

      std::function< INT32( SINT64, SINT32, rtnContextBuf & ) > _getMoreFunc =
         DEFAULT_GET_MORE_FUNC;

      std::function< INT32( INT32, const INT64 * ) > _killContextFunc = DEFAULT_KILL_CONTEXT_FUNC;

   private:
      IDataManagementService *_dms = nullptr;
      BSONObj _collectionHint = BSON( "" << DMS_STAT_CL_IDX_NAME );
      BSONObj _indexHint = BSON( "" << DMS_STAT_IDX_IDX_NAME );
   };
   using rtnObjectStatAgentImpl = _rtnObjectStatAgentImpl;

   std::unique_ptr< rtnObjectStatAgent > newRtnObjectStatAgentImpl( IDataManagementService *dms )
   {
      return std::unique_ptr< rtnObjectStatAgent >( SDB_OSS_NEW rtnObjectStatAgentImpl( dms ) );
   }

   std::unique_ptr< rtnObjectStatAgent > newRtnObjectStatAgentImpl(
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
      return std::unique_ptr< rtnObjectStatAgent >(
         SDB_OSS_NEW rtnObjectStatAgentImpl( dms, queryFunc, getMoreFunc, killContextFunc ) );
   }

   INT32 _rtnObjectStatAgentImpl::getCollectionStatInfo( IExecutor *executor,
                                                         const CHAR *clFullName,
                                                         BOOLEAN withIndexStatInfo,
                                                         RTN_CL_STAT_PTR &clStatPtr )
   {
      INT32 rc = SDB_OK;
      clStatPtr.reset();
      CHAR csName[ DMS_COLLECTION_SPACE_NAME_SZ + 1 ] = {};
      CHAR clShortName[ DMS_COLLECTION_NAME_SZ + 1 ] = {};
      BSONObj matcher;
      INT64 clContextID = -1;
      INT64 indexContextID = -1;
      rtnContextBuf contextBuf;
      rc = rtnResolveCollectionName( clFullName, ossStrlen( clFullName ), csName,
                                     DMS_COLLECTION_SPACE_NAME_SZ, clShortName,
                                     DMS_COLLECTION_NAME_SZ );
      PD_RC_CHECK( rc, PDERROR, "failed to resolve collection[%s], rc: %d", clFullName, rc );
      matcher = BSON( RTN_STAT_COLLECTION_SPACE << csName << RTN_STAT_COLLECTION << clShortName );
      rc = _queryCollectionStat( executor, matcher, clContextID );
      PD_RC_CHECK( rc, PDERROR, "failed to fetch collection[%s] statistics, rc: %d", clFullName,
                   rc );
      rc = _getMoreFunc( clContextID, 1, contextBuf );
      if ( SDB_DMS_EOC == rc )
      {
         // no need to delete context because it has been deleted
         clContextID = -1;
         clStatPtr.reset();
         rc = SDB_OK;
         goto done;
      }
      else if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to get query result, contextID[%d], rc", clContextID, rc );
         goto error;
      }
      else
      {
         RTN_CL_STAT_PTR tempPtr = nullptr;
         rc = rtnCollectionStatInfo::buildFromBson( BSONObj( contextBuf.data() ), tempPtr );
         PD_RC_CHECK( rc, PDERROR, "failed to build collection[%s] statistics cache, rc: %d",
                      clFullName, rc );
         contextBuf.release();
         clStatPtr = std::move( tempPtr );

         rc = _queryIndexStat( executor, matcher, indexContextID );
         PD_RC_CHECK( rc, PDERROR, "failed to fetch an index statistics on collection[%s], rc: %d",
                     clFullName, rc );
         while ( true )
         {
            rc = _getMoreFunc( indexContextID, 1, contextBuf );
            if ( SDB_DMS_EOC == rc )
            {
               // no need to delete context because it has been deleted
               indexContextID = -1;
               rc = SDB_OK;
               break;
            }
            else if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "failed to get query result, contextID[%d], rc: %d", clContextID, rc );
               goto error;
            }
            else
            {
               RTN_INDEX_STAT_PTR tempPtr = nullptr;
               rc = rtnIndexStatInfo::buildFromBson( BSONObj( contextBuf.data() ), tempPtr );
               PD_RC_CHECK( rc, PDERROR, "failed to build index statistics cache, rc: %d", rc );
               contextBuf.release();
               try
               {
                  clStatPtr->getIndexStatVec().push_back( tempPtr );
               }
               catch ( std::exception &e )
               {
                  rc = ossException2RC( &e );
                  PD_LOG( PDWARNING, "occur exception: %s", e.what() );
                  goto error;
               }
            }
         }
      }

   done:
      return rc;
   error:
      if ( -1 != clContextID )
      {
         _killContextFunc( 1, &clContextID );
      }
      if ( -1 != indexContextID )
      {
         _killContextFunc( 1, &indexContextID );
      }
      clStatPtr.reset();
      goto done;
   }

   INT32 _rtnObjectStatAgentImpl::getCollectionStatInfoOnCS( IExecutor *executor,
                                                             const CHAR *csName,
                                                             std::unique_ptr< contextBase > &pCtx )
   {
      INT32 rc = SDB_OK;
      BSONObj matcher = BSON( RTN_STAT_COLLECTION_SPACE << csName );
      std::unique_ptr< contextImpl > ctxImpl(
         SDB_OSS_NEW contextImpl( _dms, _queryFunc, _getMoreFunc, _killContextFunc ) );
      PD_CHECK( ctxImpl, SDB_OOM, error, PDERROR, "out of memory" );
      rc = ctxImpl->open( matcher, _collectionHint, _indexHint );
      PD_RC_CHECK( rc, PDERROR, "failed to open resource agent context on cs[%s], rc: %d", csName,
                   rc );
      pCtx = std::move( ctxImpl );
   done:
      return rc;
   error:
      goto done;
   }

   INT32 _rtnObjectStatAgentImpl::getAllCollectionStatInfo( IExecutor *executor,
                                                            std::unique_ptr< contextBase > &pCtx )
   {
      INT32 rc = SDB_OK;
      BSONObj matcher;
      std::unique_ptr< contextImpl > ctxImpl(
         SDB_OSS_NEW contextImpl( _dms, _queryFunc, _getMoreFunc, _killContextFunc ) );
      PD_CHECK( ctxImpl, SDB_OOM, error, PDERROR, "out of memory" );
      rc = ctxImpl->open( matcher, _collectionHint, _indexHint );
      PD_RC_CHECK( rc, PDERROR, "failed to open resource agent context, rc: %d", rc );
      pCtx = std::move( ctxImpl );
   done:
      return rc;
   error:
      goto done;
   }

   INT32 _rtnObjectStatAgentImpl::_queryCollectionStat( IExecutor *executor,
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

   INT32 _rtnObjectStatAgentImpl::_queryIndexStat( IExecutor *executor,
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

   // contextImpl implemente
   INT32 _rtnObjectStatAgentImpl::contextImpl::open( const BSONObj &matcher,
                                                     const BSONObj &collectionHint,
                                                     const BSONObj &indexHint )
   {
      INT32 rc = SDB_OK;
      BSONObj dummy;
      rc = _queryFunc( DMS_STAT_COLLECTION_CL_NAME, dummy, matcher, dummy, collectionHint, 0, 0, -1,
                       _dms, _clContextID, nullptr, 0 );
      PD_RC_CHECK( rc, PDERROR, "Query collection [%s] failed, rc: %d", DMS_STAT_COLLECTION_CL_NAME,
                   rc );
      rc = _queryFunc( DMS_STAT_INDEX_CL_NAME, dummy, matcher, dummy, indexHint, 0, 0, -1, _dms,
                       _indexContextID, nullptr, 0 );
      PD_RC_CHECK( rc, PDERROR, "Query collection [%s] failed, rc: %d", DMS_STAT_INDEX_CL_NAME,
                   rc );
   done:
      return rc;
   error:
      goto done;
   }

   INT32 _rtnObjectStatAgentImpl::contextImpl::fetchOne( IExecutor *executor,
                                                         RTN_CL_STAT_PTR &clStatPtr )
   {
      INT32 rc = SDB_OK;
      clStatPtr.reset();
      rtnContextBuf clStatBuf;
      // get collection statistics
      rc = _getMoreFunc( _clContextID, 1, clStatBuf );
      if ( SDB_DMS_EOC == rc )
      {
         // no need to delete context because it has been deleted
         _clContextID = -1;
         if ( -1 != _indexContextID )
         {
            _killContextFunc( 1, &_indexContextID );
         }
         goto error;
      }
      else if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to get query result, contextID[%d]", _clContextID );
         goto error;
      }
      else
      {
         dmsOpenCLOptions options;
         ossPoolVector< bson::BSONObj > indexes;
         BSONObj collectionStat = BSONObj( clStatBuf.data() );
         const CHAR *csName = collectionStat.getStringField( RTN_STAT_COLLECTION_SPACE );
         const CHAR *clShortName = collectionStat.getStringField( RTN_STAT_COLLECTION );
         CHAR clFullName[ DMS_COLLECTION_FULL_NAME_SZ + 1 ] = {};
         ossSnprintf( clFullName, sizeof( clFullName ), "%s.%s", csName, clShortName );

         rc = rtnCollectionStatInfo::buildFromBson( collectionStat, clStatPtr );
         PD_RC_CHECK( rc, PDERROR, "failed to build collection statistics cache, rc: %d", rc );

         while ( -1 != _indexContextID )
         {
            // if last index stat buffer is empty, get one more from query
            if ( _indexStatBuf.eof() )
            {
               rc = _getMoreFunc( _indexContextID, 1, _indexStatBuf );
               if ( SDB_DMS_EOC == rc )
               {
                  // no need to delete context because it has been deleted
                  _indexContextID = -1;
                  rc = SDB_OK;
                  break;
               }
               else if ( SDB_OK != rc )
               {
                  PD_LOG( PDERROR, "failed to get query result, contextID[%d]", _clContextID );
                  goto error;
               }
            }

            BSONObj indexStat = BSONObj( _indexStatBuf.data() );
            const CHAR *oCsName = indexStat.getStringField( RTN_STAT_COLLECTION_SPACE );
            const CHAR *oClShortName = indexStat.getStringField( RTN_STAT_COLLECTION );
            const CHAR *indexName = indexStat.getStringField( RTN_STAT_IDX_INDEX );
            if ( utilStringView( csName ) == utilStringView( oCsName ) &&
                 utilStringView( clShortName ) == utilStringView( oClShortName ) )
            {
               RTN_INDEX_STAT_PTR indexStatPtr = nullptr;
               rc = rtnIndexStatInfo::buildFromBson( indexStat, indexStatPtr,
                                                     clStatPtr->getCLFullNameSharedPtr() );
               PD_RC_CHECK( rc, PDERROR,
                            "failed to build index statistics from bson, index name[%s]",
                            indexName );
               try
               {
                  clStatPtr->getIndexStatVec().push_back( indexStatPtr );
               }
               catch ( std::exception &e )
               {
                  rc = ossException2RC( &e );
                  PD_LOG( PDWARNING, "occur exception: %s", e.what() );
                  goto error;
               }

               // read index statistics and push back to collection. Release the buffer.
               _indexStatBuf.release();
            }
            // if the names do not match, means that all indexes statistics of current collection
            // has been read. Do not release the buffer for the next collection.
            else
            {
               break;
            }
         }
      }

   done:
      return rc;
   error:
      clStatPtr.reset();
      goto done;
   }

   INT32 _rtnObjectStatAgentImpl::contextImpl::fetchBatch(
      IExecutor *executor,
      UINT32 batchSize,
      ossPoolVector< RTN_CL_STAT_PTR > &clStats )
   {
      INT32 rc = SDB_OK;
      RTN_CL_STAT_PTR clStatPtr = nullptr;
      for ( UINT32 i = 0; i < batchSize ; ++i)
      {
         rc = fetchOne(executor, clStatPtr);
         if ( SDB_DMS_EOC == rc )
         {
            break;
         }
         else
         {
            PD_RC_CHECK( rc, PDERROR, "failed to fetch collection statistics, position: %d, rc: %d",
                         i, rc );
         }
         try
         {
            clStats.push_back( std::move( clStatPtr ) );
         }
         catch ( std::exception &e )
         {
            rc = ossException2RC( &e );
            PD_LOG( PDWARNING, "occur exception: %s", e.what() );
            goto error;
         }
      }
      
      done:
      return rc;
   error:
      clStats.clear();
      goto done;
   }

   _rtnObjectStatAgentImpl::contextImpl::~contextImpl()
   {
      if ( -1 != _clContextID )
      {
         _killContextFunc( 1, &_clContextID );
      }
      if ( -1 != _indexContextID )
      {
         _killContextFunc( 1, &_indexContextID );
      }
   }

} // namespace engine
