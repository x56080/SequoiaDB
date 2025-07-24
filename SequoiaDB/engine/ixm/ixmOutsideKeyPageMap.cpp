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

   Source File Name = ixmOutsideKeyPageMap.cpp

   Descriptive Name = a map of index pages with outside key

   When/how to use: this program may be used on binary and text-formatted
   versions of data management component. This file contains structure for
   dms Reccord ID (RID).

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          08/07/2019  JT  Initial Draft

   Last Changed =

*******************************************************************************/
#include "ixmOutsideKeyPageMap.hpp"
#include "pd.hpp"
#include "pmd.hpp"
#include "dmsCB.hpp"
#include "dmsStorageUnit.hpp"
#include "dmsStorageIndex.hpp"
#include "ixmKey.hpp"

namespace engine
{
   _ixmOutsideKeyPageMap::_ixmOutsideKeyPageMap() { _mapPages.clear(); }

   _ixmOutsideKeyPageMap::~_ixmOutsideKeyPageMap()
   {
      SDB_ASSERT( 0 == _mapPages.size(), "Map must be empty" ) ;
   }

   void  _ixmOutsideKeyPageMap::addItem
   (
      dmsExtentID pageId,
      imxOutsideKey * pKey
   )
   {
      if ( pKey )
      {
         _latch.get() ;

         /// insert
         _mapPages.insert( MAP_OUTKEY_PAGES::value_type( pageId, *pKey ) ) ;

         _latch.release() ;
      }
   }

   void  _ixmOutsideKeyPageMap::rmItem( dmsExtentID pageId )
   {
      _latch.get() ;

      MAP_OUTKEY_PAGES_IT it = _mapPages.find( pageId ) ;
      if ( it != _mapPages.end() )
      {
         it->second.release() ;
         _mapPages.erase( it ) ;
      }

      _latch.release() ;
   }

   BOOLEAN _ixmOutsideKeyPageMap::findItem
   (
      dmsExtentID pageId,
      imxOutsideKey * pKey
   )
   {
      BOOLEAN bResult = FALSE ;

      _latch.get() ;

      MAP_OUTKEY_PAGES_CIT cit = _mapPages.find( pageId ) ;
      if ( cit != _mapPages.end() )
      {
         if ( pKey )
         {
            *pKey = cit->second ;
         }
         bResult = TRUE ;
      }

      _latch.release() ;

      return bResult ;
   }

   BOOLEAN _ixmOutsideKeyPageMap::findItem
   (
      dmsExtentID   pageId,
      BSONObj     & keyObj,
      dmsRecordID & rid,
      UINT32      & indexLID
   )
   {
      BOOLEAN bResult = FALSE ;
      ixmKey  dummy ;

      _latch.get() ;

      MAP_OUTKEY_PAGES_CIT cit = _mapPages.find( pageId ) ;
      if ( cit != _mapPages.end() )
      {
         ixmKey outKey( cit->second._keyObjPtr.get() ) ;
         keyObj   = outKey.toBson() ; 
         rid      = cit->second._rid ;
         indexLID = cit->second._indexLID ;
         bResult  = TRUE ;
         outKey.assign( dummy ) ;
      }

      _latch.release() ;

      return bResult ;
   }


   void  _ixmOutsideKeyPageMap::removePagesOfIndex( UINT32 indexLID )
   {
      MAP_OUTKEY_PAGES_IT it ;

      _latch.get() ;

      for ( it = _mapPages.begin() ; it != _mapPages.end(); ++it )
      {
         if ( indexLID == it->second._indexLID )
         {
            it->second.release() ;
            _mapPages.erase( it ) ;
         }
      }

      _latch.release() ;
   } 


   _ixmCleanupIndexPageJob::_ixmCleanupIndexPageJob
   (
      UINT32 csID,     
      UINT16 mbID,
      UINT32 csLID,
      UINT32 clLID,
      UINT32 indexLID,
      UINT32 indexPage 
   )
   {
      _csID  = csID ;
      _mbID  = mbID ;
      _csLID = csLID ;
      _clLID = clLID ;
      _indexLID  = indexLID ;
      _indexPage = indexPage ;
   }

   _ixmCleanupIndexPageJob::~_ixmCleanupIndexPageJob()
   {
   }

   const CHAR* _ixmCleanupIndexPageJob::name() const
   {
      return "Cleanup index page" ;
   }

   INT32 _ixmCleanupIndexPageJob::doit( IExecutor *pExe,
                                        UTIL_LJOB_DO_RESULT &result,
                                        UINT64 &sleepTime )
   {
      static SDB_DMSCB  *pDmsCB = sdbGetDMSCB() ;

      INT32 rcTmp = SDB_OK ;
      dmsStorageUnit *su = NULL ;
      dmsMBContext *pContext = NULL ;
      sleepTime = 30000000 ;   /// 30 second

      su = pDmsCB->suLock( _csID ) ;
      if ( !su || su->LogicalCSID() != _csLID )
      {
         /// cs not exist
         result = UTIL_LJOB_DO_FINISH ;
         goto done ;
      }

      if ( SDB_OK != su->data()->getMBContext( &pContext, _mbID,
                                               _clLID, _clLID ) )
      {
         /// cl not exist
         result = UTIL_LJOB_DO_FINISH ;
         goto done ;
      }

      rcTmp = pContext->mbLock( SHARED ) ;
      if ( SDB_TIMEOUT == rcTmp )
      {
         /// wait next time
         result = UTIL_LJOB_DO_CONT ;
         goto done ;
      }
      else if ( rcTmp )
      {
         /// cl not exist 
         result = UTIL_LJOB_DO_FINISH ;
         goto done ;
      }

      /// clean up outside key by splitting index page
      rcTmp = su->index()->cleanUpIndexPage( pContext,(pmdEDUCB*)pExe,
                                             _indexLID, _indexPage ) ;
      if ( SDB_OK == rcTmp )
      {
         result = UTIL_LJOB_DO_FINISH ;
      }
      else
      {
         /// wait next time
         result = UTIL_LJOB_DO_CONT ;
      }
   done:
      if ( pContext )
      {
         su->data()->releaseMBContext( pContext ) ;
      }
      if ( su )
      {
         pDmsCB->suUnlock( su->CSID(), SHARED ) ;
      }
      return SDB_OK ;
   }


   void ixmStartAsyncCleanupIndexPage( UINT32 csID,     UINT16 mbID,
                                       UINT32 csLID,    UINT32 clLID,
                                       UINT32 indexLID, UINT32 indexPage )
   {
      INT32 rc = SDB_OK ;
      ixmCleanupIndexPageJob *pJob = NULL ;

      pJob = SDB_OSS_NEW ixmCleanupIndexPageJob( csID,     mbID,
                                                 csLID,    clLID,
                                                 indexLID, indexPage ) ;
      if ( !pJob )
      {
         PD_LOG( PDWARNING, "Alloc ixmCleanupIndexPageJob(CSID:%u, CLID:%u, "
                 "CSLID:%u, CLLID:%u, IndexLID:%u, IndexPage:%u) failed",
                 csID, mbID, csLID, clLID, indexLID, indexPage ) ;
      }
      else
      {
         rc = pJob->submit( TRUE ) ;
         if ( rc )
         {
            PD_LOG( PDWARNING,
                    "Submit ixmCleanupIndexPageJob(CSID:%u, CLID:%u, "
                    "CSLID:%u, CLLID:%u, IndexLID:%u, IndexPage:%u) failed, "
                    "rc: %d",
                    csID, mbID, csLID, clLID, indexLID, indexPage, rc ) ;
         }
#if defined (_DEBUG)
         else
         {
            PD_LOG( PDDEBUG,
                    "Submitted ixmCleanupIndexPageJob(CSID:%u, CLID:%u, "
                    "CSLID:%u, CLLID:%u, IndexLID:%u, IndexPage:%u) ",
                    csID, mbID, csLID, clLID, indexLID, indexPage ) ;
         }
#endif
      }
   }

}

