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

   Source File Name = ixmOutsideKeyPageMap.hpp

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
#ifndef IXM_OUTSIDE_KEY_PAGE_MAP__
#define IXM_OUTSIDE_KEY_PAGE_MAP__

#include "dms.hpp"
#include "ossMemPool.hpp"
#include "ossLatch.hpp"
#include "utilPooledAutoPtr.hpp"
#include "utilLightJobBase.hpp"

namespace engine
{
   typedef _utilPooledAutoPtr _ixmKeyObjPtr ;
   class _imxOutsideKey : public SDBObject
   {
   public:
      _ixmKeyObjPtr _keyObjPtr ;
      dmsRecordID   _rid ;
      UINT32        _indexLID ;
      UINT16        _mbID ;
      UINT32        _clLID ;


      OSS_INLINE _imxOutsideKey& operator= ( const _imxOutsideKey &rhs )
      {
         _keyObjPtr = rhs._keyObjPtr ;
         _rid       = rhs._rid ;
         _indexLID  = rhs._indexLID ;
         _mbID      = rhs._mbID ;
         _clLID     = rhs._clLID ;
         return *this ;
      }
      OSS_INLINE void release()
      {
         _keyObjPtr.release() ;
      }
   } ;
   typedef _imxOutsideKey imxOutsideKey ;

   typedef ossPoolMap< dmsExtentID, imxOutsideKey > MAP_OUTKEY_PAGES;
   typedef MAP_OUTKEY_PAGES::iterator               MAP_OUTKEY_PAGES_IT ;
   typedef MAP_OUTKEY_PAGES::const_iterator         MAP_OUTKEY_PAGES_CIT ;

   class _ixmOutsideKeyPageMap : public SDBObject
   {
   public:
      _ixmOutsideKeyPageMap() ;
      ~_ixmOutsideKeyPageMap() ;

      void    addItem ( dmsExtentID pageId, imxOutsideKey * pKey ) ;
      BOOLEAN findItem( dmsExtentID pageId, imxOutsideKey * pKey ) ;
      BOOLEAN findItem( dmsExtentID   pageId,
                        BSONObj     & keyObj,
                        dmsRecordID & rid,
                        UINT32      & indexLID ) ;
      void    rmItem( dmsExtentID pageId ) ;
      void    removePagesOfIndex( UINT32 indexLID ) ;
      void    dupOutsideKeyPageMap( MAP_OUTKEY_PAGES & mapCopy )
              {
                 _latch.get() ;
                 mapCopy = _mapPages ;
                 _latch.release() ;
              }

   private:
      ossSpinXLatch     _latch ;
      MAP_OUTKEY_PAGES  _mapPages ;
   } ;
   typedef _ixmOutsideKeyPageMap ixmOutsideKeyPageMap ;


   /*
      _ixmCleanupIndexPageJob define
   */
   class _ixmCleanupIndexPageJob : public _utilLightJob
   {
      public:
         _ixmCleanupIndexPageJob( UINT32 csID,     UINT16 mbID,
                                  UINT32 csLID,    UINT32 clLID,
                                  UINT32 indexLID, UINT32 indexPage ) ;

         virtual ~_ixmCleanupIndexPageJob() ;

         virtual const CHAR*     name() const ;
         virtual INT32           doit( IExecutor *pExe,
                                       UTIL_LJOB_DO_RESULT &result,
                                       UINT64 &sleepTime ) ;

      protected:
         UINT32            _csID ;
         UINT16            _mbID ;
         UINT32            _csLID ;
         UINT32            _clLID ;
         UINT32            _indexLID ;
         UINT32            _indexPage ;
   } ;
   typedef _ixmCleanupIndexPageJob ixmCleanupIndexPageJob ;

   void ixmStartAsyncCleanupIndexPage( UINT32 csID,     UINT16 mdID,
                                       UINT32 csLID,    UINT32 clLID,
                                       UINT32 indexLID, UINT32 indexPage ) ;
}

#endif /* IXM_OUTSIDE_KEY_PAGE_MAP__ */

