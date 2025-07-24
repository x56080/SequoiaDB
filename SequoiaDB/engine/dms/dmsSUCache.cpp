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

   Source File Name = dmsSUCache.cpp

   Descriptive Name = DMS Storage Unit Caches

   When/how to use: this program may be used on binary and text-formatted
   versions of data management component. This file contains code logic for
   management of storage unit caches.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          07/10/2017  HGM Initial Draft

   Last Changed =

*******************************************************************************/
#include "dmsSUCache.hpp"
#include "dmsCachedPlanUnit.hpp"
#include "optCommon.hpp"
#include "pdTrace.hpp"
#include "dmsTrace.hpp"

namespace engine
{

   /*
      _dmsStatCache implement
    */
   _dmsStatCache::_dmsStatCache ( IDmsSUCacheHolder *pHolder )
   : dmsSUCache( DMS_CACHE_TYPE_STAT, UTIL_SU_CACHE_UNIT_CLSTAT, pHolder )
   {
   }

   _dmsStatCache::~_dmsStatCache ()
   {
   }

   /*
      _dmsCachedPlanMgr implement
    */
   _dmsCachedPlanMgr::_dmsCachedPlanMgr ( IDmsSUCacheHolder *pHolder )
   : dmsSUCache( DMS_CACHE_TYPE_PLAN, UTIL_SU_CACHE_UNIT_CLPLAN, pHolder ),
     _cacheBitmap( 0 ),
     _paramInvalidBitmap(),
     _mainCLInvalidBitmap()
   {
      _setBucketModulo() ;
   }

   _dmsCachedPlanMgr::~_dmsCachedPlanMgr ()
   {
   }

   INT32 _dmsCachedPlanMgr::createCLCachedPlanUnit ( UINT16 mbID )
   {
      // Create new cached plan status for this collection
      dmsCLCachedPlanUnit *pCachedPlanUnit =
            SDB_OSS_NEW dmsCLCachedPlanUnit( mbID, 0 ) ;
      if ( NULL != pCachedPlanUnit &&
           !addCacheUnit( pCachedPlanUnit, TRUE, FALSE ) )
      {
         // Failed to add unit, the unit should be deleted
         SAFE_OSS_DELETE( pCachedPlanUnit ) ;
      }
      return SDB_OK ;
   }

   INT32 _dmsCachedPlanMgr::resizeBitmaps ( UINT32 bucketNum )
   {
      if ( bucketNum <= OPT_PLAN_MAX_CACHE_BUCKETS )
      {
         _cacheBitmap.resize( bucketNum ) ;
         _setBucketModulo() ;
      }
      return SDB_OK ;
   }

   void _dmsCachedPlanMgr::_setBucketModulo ()
   {
      if ( _cacheBitmap.getSize() > 0 )
      {
         _bucketModulo = _cacheBitmap.getSize() - 1 ;
      }
      else
      {
         _bucketModulo = 0 ;
      }
   }

}
