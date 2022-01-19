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

   Source File Name = dmsCacheHolder.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/

#include "dmsCacheHolder.hpp"
#include "dmsStatUnit.hpp"
#include "dmsStorageUnit.hpp"
#include "dmsMBContext.hpp"
#include "dmsStorageDataCommon.hpp"
#include "pdTrace.hpp"
#include "dmsTrace.hpp"


namespace engine
{
   _dmsCacheHolder::_dmsCacheHolder ( dmsStorageUnit *su )
   {
      SDB_ASSERT( su, "Storage Unit is not valid" ) ;

      _su = su ;
      ossMemset( _pSUCaches, 0, sizeof( _pSUCaches ) ) ;
   }

   _dmsCacheHolder::~_dmsCacheHolder ()
   {
      deleteAllSUCaches() ;
   }


   const CHAR *_dmsCacheHolder::getCSName () const
   {
      return _su->CSName() ;
   }

   UINT32 _dmsCacheHolder::getSUID () const
   {
      return _su->CSID() ;
   }

   UINT32 _dmsCacheHolder::getSULID () const
   {
      return _su->LogicalCSID() ;
   }

   BOOLEAN _dmsCacheHolder::isSysSU () const
   {
      return dmsIsSysCSName( getCSName() ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSCACHEHOLDER_CHKUNIT, "_dmsCacheHolder::checkCacheUnit" )
   BOOLEAN _dmsCacheHolder::checkCacheUnit ( utilSUCacheUnit *pCacheUnit )
   {
      BOOLEAN exists = FALSE ;

      PD_TRACE_ENTRY( SDB__DMSCACHEHOLDER_CHKUNIT ) ;

      switch ( pCacheUnit->getUnitType() )
      {
         case UTIL_SU_CACHE_UNIT_CLSTAT :
         {
            if ( SDB_OK != _checkCollectionStat( (dmsCollectionStat *)pCacheUnit ) )
            {
               PD_LOG( PDWARNING, "Failed to check collection statistics" ) ;
               goto error ;
            }
            exists = TRUE ;
            break ;
         }
         case UTIL_SU_CACHE_UNIT_IXSTAT :
         {
            if ( SDB_OK != _checkIndexStat( (dmsIndexStat *)pCacheUnit , NULL ) )
            {
               PD_LOG( PDWARNING, "Failed to check index statistics" ) ;
               goto error ;
            }
            exists = TRUE ;
            break ;
         }
         case UTIL_SU_CACHE_UNIT_CLPLAN :
            exists = TRUE ;
            break ;
         default :
            break ;
      }

   done :
      PD_TRACE_EXIT( SDB__DMSCACHEHOLDER_CHKUNIT ) ;
      return exists ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSCACHEHOLDER_CRTCACHE, "_dmsCacheHolder::createSUCache" )
   BOOLEAN _dmsCacheHolder::createSUCache ( UINT8 type )
   {
      BOOLEAN created = FALSE ;

      PD_TRACE_ENTRY( SDB__DMSCACHEHOLDER_CRTCACHE ) ;

      if ( type < DMS_CACHE_TYPE_NUM &&
           NULL == _pSUCaches[ type ] )
      {
         switch ( type )
         {
            case DMS_CACHE_TYPE_STAT :
            {
               if ( !isSysSU() )
               {
                  _pSUCaches[ type ] = SDB_OSS_NEW dmsStatCache( this ) ;
               }
               break ;
            }
            case DMS_CACHE_TYPE_PLAN :
            {
               _pSUCaches[ type ] = SDB_OSS_NEW dmsCachedPlanMgr( this ) ;
               break ;
            }
            default :
            {
               SDB_ASSERT( FALSE, "Invalid switch branch" ) ;
               break ;
            }
         }
         if ( _pSUCaches[ type ] )
         {
            created = TRUE ;
         }
      }

      PD_TRACE_EXIT( SDB__DMSCACHEHOLDER_CRTCACHE ) ;

      return created ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSCACHEHOLDER_DELCACHE, "_dmsCacheHolder::deleteSUCache" )
   BOOLEAN _dmsCacheHolder::deleteSUCache ( UINT8 type )
   {
      BOOLEAN deleted = FALSE ;

      PD_TRACE_ENTRY( SDB__DMSCACHEHOLDER_DELCACHE ) ;

      if ( type < DMS_CACHE_TYPE_NUM && NULL != _pSUCaches[ type ] )
      {
         if ( _pSUCaches[ type ] != NULL )
         {
            _pSUCaches[ type ]->clearCacheUnits() ;
            SDB_OSS_DEL _pSUCaches[ type ] ;
            _pSUCaches[ type ] = NULL ;
            deleted = TRUE ;
         }
      }

      PD_TRACE_EXIT( SDB__DMSCACHEHOLDER_DELCACHE ) ;

      return deleted ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSCACHEHOLDER_DELALLCACHES, "_dmsCacheHolder::deleteAllSUCaches" )
   void _dmsCacheHolder::deleteAllSUCaches ()
   {
      PD_TRACE_ENTRY( SDB__DMSCACHEHOLDER_DELALLCACHES ) ;

      for ( UINT8 type = 0 ; type < DMS_CACHE_TYPE_NUM ; type ++ )
      {
         if ( _pSUCaches[ type ] != NULL )
         {
            _pSUCaches[ type ]->clearCacheUnits() ;
            SDB_OSS_DEL _pSUCaches[ type ] ;
            _pSUCaches[ type ] = NULL ;
         }
      }

      PD_TRACE_EXIT( SDB__DMSCACHEHOLDER_DELALLCACHES ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSCACHEHOLDER_CHKCLSTAT, "_dmsCacheHolder::_checkCollectionStat" )
   INT32 _dmsCacheHolder::_checkCollectionStat( dmsCollectionStat *pCollectionStat )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSCACHEHOLDER_CHKCLSTAT ) ;

      SDB_ASSERT( pCollectionStat, "pCollectionStat is invalid" ) ;

      dmsMBContext *mbContext = NULL ;
      const CHAR *pCSName = pCollectionStat->getCSName() ;
      const CHAR *pCLName = pCollectionStat->getCLName() ;
      INDEX_STAT_MAP &indexStats = pCollectionStat->getIndexStats() ;
      INDEX_STAT_ITERATOR iterIdx ;

      BOOLEAN needCheck =
            ( pCollectionStat->getMBID() != UTIL_SU_INVALID_UNITID ) ;

      if ( needCheck )
      {
         PD_CHECK( _su->LogicalCSID() == pCollectionStat->getSULogicalID(),
                   SDB_DMS_CS_NOTEXIST, error, PDWARNING, "Failed to get "
                   "collection space [%s] for statistics", pCSName ) ;
      }
      else
      {
         pCollectionStat->setSULogicalID( _su->LogicalCSID() ) ;
      }

      rc = _su->data()->getMBContext( &mbContext, pCLName, SHARED ) ;
      PD_RC_CHECK( rc, PDWARNING, "Failed to get collection [%s], rc: %d",
                   pCLName, rc ) ;

      if ( needCheck )
      {
         PD_CHECK( mbContext->mbID() == pCollectionStat->getMBID() &&
                   mbContext->clLID() == pCollectionStat->getCLLogicalID(),
                   SDB_DMS_NOTEXIST, error, PDWARNING, "Failed to get "
                   "collection [%s.%s] for statistics", pCSName, pCLName ) ;
      }
      else
      {
         pCollectionStat->setMBID( mbContext->mbID() ) ;
         pCollectionStat->setCLLogicalID( mbContext->clLID() ) ;
      }

      iterIdx = indexStats.begin() ;
      while ( iterIdx != indexStats.end() )
      {
         dmsIndexStat *pIndexStat = iterIdx->second ;
         if ( SDB_OK != _checkIndexStat( pIndexStat, mbContext ) )
         {
            // Remove field statistics reference
            pCollectionStat->removeFieldStat( pIndexStat->getFirstField(),
                                              TRUE ) ;
            // Remove index statistics reference
            iterIdx = indexStats.erase( iterIdx ) ;
            // Delete the index statistics
            SAFE_OSS_DELETE( pIndexStat ) ;
         }
         else
         {
            ++ iterIdx ;
         }
      }

   done :
      if ( mbContext )
      {
         _su->data()->releaseMBContext( mbContext ) ;
      }
      PD_TRACE_EXITRC( SDB__DMSCACHEHOLDER_CHKCLSTAT, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSCACHEHOLDER_CHKIDXSTAT, "_dmsCacheHolder::_checkIndexStat" )
   INT32 _dmsCacheHolder::_checkIndexStat ( dmsIndexStat *pIndexStat,
                                            dmsMBContext *mbContext )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSCACHEHOLDER_CHKIDXSTAT ) ;

      BOOLEAN needAllocate = !mbContext ;
      dmsExtentID indexCBExtent = DMS_INVALID_EXTENT ;
      const CHAR *pCSName = pIndexStat->getCSName() ;
      const CHAR *pCLName = pIndexStat->getCLName() ;
      const CHAR *pIndexName = pIndexStat->getIndexName() ;

      BOOLEAN needCheck = ( pIndexStat->getMBID() != UTIL_SU_INVALID_UNITID ) ;

      if ( needCheck )
      {
         PD_CHECK( _su->LogicalCSID() == pIndexStat->getSULogicalID(),
                   SDB_DMS_CS_NOTEXIST, error, PDWARNING, "Failed to get "
                   "collection space [%s] for statistics", pCSName ) ;
      }
      else
      {
         pIndexStat->setSULogicalID( _su->LogicalCSID() ) ;
      }

      if ( !mbContext )
      {
         rc = _su->data()->getMBContext( &mbContext, pCLName, SHARED ) ;
         PD_RC_CHECK( rc, PDWARNING, "Failed to get collection [%s], rc: %d",
                      pCLName, rc ) ;
      }

      if ( needCheck )
      {
         PD_CHECK( mbContext->mbID() == pIndexStat->getMBID() &&
                   mbContext->clLID() == pIndexStat->getCLLogicalID(),
                   SDB_DMS_NOTEXIST, error, PDWARNING, "Failed to get "
                   "collection [%s.%s] for statistics", pCSName, pCLName ) ;
      }
      else
      {
         pIndexStat->setMBID( mbContext->mbID() ) ;
         pIndexStat->setCLLogicalID( mbContext->clLID() ) ;
      }

      rc = _su->index()->getIndexCBExtent( mbContext, pIndexName, indexCBExtent ) ;
      PD_RC_CHECK( rc, PDWARNING, "Failed to get index [%s], rc: %d",
                   pIndexName, rc ) ;

      {
         ixmIndexCB indexCB ( indexCBExtent, _su->index(), NULL ) ;

         PD_CHECK( indexCB.isInitialized(),
                   SDB_DMS_INIT_INDEX, error, PDWARNING,
                   "Index [%s] is invalid", pIndexName ) ;
         PD_CHECK( indexCB.getFlag() == IXM_INDEX_FLAG_NORMAL,
                   SDB_IXM_UNEXPECTED_STATUS, error, PDDEBUG,
                   "Index [%s] is not normal status",pIndexName ) ;

         if ( needCheck )
         {
            PD_CHECK( pIndexStat->getIndexLogicalID() == indexCB.getLogicalID(),
                      SDB_IXM_NOTEXIST, error, PDWARNING,
                      "Logical ID of index [%s] are not matched", pIndexName ) ;
         }
         else
         {
            pIndexStat->setIndexLogicalID( indexCB.getLogicalID() ) ;
         }

         PD_CHECK( 0 == pIndexStat->getKeyPattern().woCompare(
                               indexCB.keyPattern(), BSONObj(), TRUE ),
                   SDB_IXM_NOTEXIST, error, PDWARNING,
                   "Keys of index [%s] are not matched", pIndexName ) ;
      }

   done :
      if ( needAllocate && mbContext )
      {
         _su->data()->releaseMBContext( mbContext ) ;
      }
      PD_TRACE_EXITRC( SDB__DMSCACHEHOLDER_CHKIDXSTAT, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   }// namespace engine