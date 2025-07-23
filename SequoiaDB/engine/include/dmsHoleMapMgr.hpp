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

   Source File Name = dmsHoleMapMgr.hpp

   Descriptive Name =

   When/how to use: N/A

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          12/01/2023  Tangtao Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef DMS_HOLEMAPMGR_HPP_
#define DMS_HOLEMAPMGR_HPP_

#include "dmsStorageBase.hpp"


using namespace std ;
using namespace bson ;

namespace engine
{

   #define DMS_HMM_EYECATCHER          "SDBHMM"

   class _dmsHoleMapManagementExtent : public _utilBitmapBase
   {
   public:
      _dmsHoleMapManagementExtent( CHAR* mask, UINT32 pageNum )
      {
         _size = pageNum ;
         _freeSize = pageNum ;

         // size of bitmap buffer ( in bytes )
         _bitmapSize = pageNum >> UTIL_BITMAP_UNIT_LOG2SIZE ;
         _bitmap = (UINT8*)mask ;
      }
      ~_dmsHoleMapManagementExtent()
      {
      }
   } ;
   typedef _dmsHoleMapManagementExtent dmsHoleMapManagementExtent ;
   typedef dmsHoleMapManagementExtent dmsHME ;


   /* hole map manager */
   class _dmsHoleMapMgr : public _ossMmapFile, public IHoleMapMgr
   {
      public:
         _dmsHoleMapMgr( const CHAR *pSuFileName,
                         dmsStorageInfo *pInfo ) ;
         ~_dmsHoleMapMgr() ;
         const CHAR*    getSuFileName() const ;
         INT32    open( const CHAR *pPath ) ;
         BOOLEAN  needRebuild() const
         {
            return _needRebuild ;
         }
         INT32    renameStorage( const CHAR *csName,
                                 const CHAR *suFileName ) ;
         INT32    removeStorage() ;
         void     closeStorage() ;

         virtual INT32  resetHoleMapMask( INT32 type ) ;
         virtual INT32  clearHoleMapMask( INT32 type, dmsExtentID &foundPage,
                                          INT32 &numPages, INT64 *pOffset = NULL,
                                          INT64 *pLenght = NULL ) ;
         virtual INT32  setHoleMapMask( INT32 type, dmsExtentID &foundPage,
                                        INT32 &numPages, INT64 *pOffset = NULL,
                                        INT64 *pLenght = NULL ) ;
         virtual INT32  setHoleMapMask( INT32 type,
                                        const ossPoolVector<_dmsSMESpaceNode> &freePages,
                                        ossPoolVector<_dmsFileSpaceNode> &offsetVec ) ;
         virtual void   flushHME( BOOLEAN sync ) ;

      private:
         INT32    _openHoleMap( BOOLEAN createNew = FALSE ) ;
         INT32    _refreshHoleMap() ;
         INT32    _initializeHMMgr( UINT64 size ) ;
         void     _initHeader ( dmsStorageUnitHeader *pHeader ) ;
         UINT32   _getSuPageSize( INT32 type ) ;
         INT32    _getHME( INT32 type, dmsHME **pHME ) ;

      private:
         ossSpinSLatch              _HMMgrMutex ;
         dmsStorageUnitHeader       *_dmsHeader ;     // 64KB
         dmsStorageInfo             *_pStorageInfo ;
         BOOLEAN                    _needRebuild ;
         CHAR                       _suFileName[ ( DMS_SU_NAME_SZ + 15 ) + 1 ] ;
         CHAR                       _fullPathName[ OSS_MAX_PATHSIZE + 1 ] ;

         UINT64                     _maxHoleNum ;
         UINT64                     _maxLobHoleNum ;
         ossPoolMap<INT32,UINT32>   _suPageSize ;
         ossPoolMap<INT32,dmsHME*>  _HME ;

   } ;
   typedef _dmsHoleMapMgr dmsHoleMapMgr ;


}

#endif //DMS_HOLEMAPMGR_HPP_