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

   Source File Name = dmsStorageLobData.hpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          17/07/2014  YW Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef DMS_STORAGELOBDATA_HPP_
#define DMS_STORAGELOBDATA_HPP_

#include "dmsLobDef.hpp"
#include "ossIO.hpp"
#include "dmsStorageBase.hpp"
#include "pmdEDU.hpp"
#include "utilCache.hpp"

namespace engine
{
   #define DMS_LOBD_FLAG_NULL       0x00000000
   #define DMS_LOBD_FLAG_DIRECT     0x00000001
   #define DMS_LOBD_FLAG_SPARSE     0x00000002

   #define DMS_LOBD_EYECATCHER            "SDBLOBD"
   #define DMS_LOBD_EYECATCHER_LEN        8

   /*
      _dmsStorageLobData define
   */
   class _dmsStorageLobData : public utilCachFileBase
   {
   public:
      _dmsStorageLobData( const CHAR *fileName,
                          BOOLEAN enableSparse,
                          BOOLEAN useDirectIO ) ;
      virtual ~_dmsStorageLobData() ;

      void     enableSparse( BOOLEAN sparse ) ;

   /// Base class functions
   public:
      virtual const CHAR*     getFileName() const ;

      virtual INT32  prepareWrite( INT32 pageID,
                                   const CHAR *pData,
                                   UINT32 len,
                                   UINT32 offset,
                                   IExecutor *cb ) ;

      virtual INT32  write( INT32 pageID, const CHAR *pData,
                            UINT32 len, UINT32 offset,
                            UINT32 newestMask,
                            IExecutor *cb ) ;

      virtual INT32  prepareRead( INT32 pageID,
                                  CHAR *pData,
                                  UINT32 len,
                                  UINT32 offset,
                                  IExecutor *cb ) ;

      virtual INT32  read( INT32 pageID, CHAR *pData,
                           UINT32 len, UINT32 offset,
                           UINT32 &readLen,
                           IExecutor *cb ) ;

      virtual INT64 pageID2Offset( INT32 pageID,
                                   UINT32 pageOffset = 0 ) const
      {
         return getSeek( pageID, pageOffset ) ;
      }

      virtual INT32  offset2PageID( INT64 offset,
                                    UINT32 *pageOffset = NULL ) const
      {
         return getPageID( offset, pageOffset ) ;
      }

      virtual INT32  writeRaw( INT64 offset, const CHAR *pData,
                               UINT32 len, IExecutor *cb,
                               BOOLEAN isAligned,
                               UINT32 newestMask = UTIL_WRITE_NEWEST_BOTH ) ;

      virtual INT32  readRaw( INT64 offset, UINT32 len,
                              CHAR *buf, UINT32 &readLen,
                              IExecutor *cb,
                              BOOLEAN isAligned ) ;

   public:
      OSS_INLINE INT64 getFileSz() const
      {
         return _fileSz ;
      }

      OSS_INLINE INT64 getDataSz() const
      {
         return getFileSz() - sizeof( _dmsStorageUnitHeader ) ;
      }

      OSS_INLINE BOOLEAN isDirectIO() const
      {
         return OSS_BIT_TEST( _flags, DMS_LOBD_FLAG_DIRECT ) ? TRUE : FALSE ;
      }

      OSS_INLINE BOOLEAN isEnableSparse() const
      {
         return OSS_BIT_TEST( _flags, DMS_LOBD_FLAG_SPARSE ) ? TRUE : FALSE ;
      }

      OSS_INLINE UINT32 pageSize() const { return _pageSz ; }
      OSS_INLINE UINT32 pageSizeSquareRoot() const { return _logarithmic ; }

      OSS_INLINE UINT32 getSegmentSize() const
      {
         SDB_ASSERT( _segmentSize > 0, "Invalid segment size of lobd" ) ;
         return _segmentSize ;
      }

      OSS_INLINE UINT32 segmentPages() const { return _segmentPages ; }
      OSS_INLINE UINT32 segmentPagesSquareRoot() const { return _segmentPagesSquare ; }

      INT32 open( const CHAR *path,
                  BOOLEAN createNew,
                  UINT32 lobdSegmentSize,
                  UINT32 totalDataPages,
                  const dmsStorageInfo &info,
                  _pmdEDUCB *cb ) ;

      INT32 rename( const CHAR *csName,
                    const CHAR *suFileName,
                    _pmdEDUCB *cb ) ;

      BOOLEAN isOpened() const ;

      INT32 close() ;

      INT32 remove() ;

      INT32 extend( INT64 len ) ;
      INT32 postExtend( UINT32 totalDataPages, INT32 result ) ;

      INT32 flush() ;

      INT32 freeCache( DMS_LOB_PAGEID startPageID, UINT32 pageNum ) ;

   private:
      INT32 _initFileHeader( const dmsStorageInfo &info,
                             _pmdEDUCB *cb ) ;

      INT32 _validateFile( const dmsStorageInfo &info,
                           _pmdEDUCB *cb ) ;

      INT32 _getFileHeader( _dmsStorageUnitHeader &header,
                            _pmdEDUCB *cb ) ;

      INT32 _writeFileHeader( const _dmsStorageUnitHeader &header,
                              _pmdEDUCB *cb ) ;

      OSS_INLINE SINT64 getSeek( DMS_LOB_PAGEID page,
                                 UINT32 offset ) const
      {
         if ( page < 0 )
         {
            return (INT64)offset ;
         }
         else
         {
            INT64 seek = page ;
            return ( seek << _logarithmic ) +
                   sizeof( _dmsStorageUnitHeader ) + offset ;
         }
      }

      OSS_INLINE INT32  getPageID( INT64 offset,
                                   UINT32 *pageOffset ) const
      {
         INT32 pageID = -1 ;
         if ( offset < (INT64)sizeof( _dmsStorageUnitHeader ) )
         {
            if ( pageOffset )
            {
               *pageOffset = ( UINT32 )offset ;
            }
         }
         else
         {
            offset -= sizeof( _dmsStorageUnitHeader ) ;
            pageID = offset >> _logarithmic ;
            if ( pageOffset )
            {
               *pageOffset = offset & ( _pageSz - 1 ) ;
            }
         }
         return pageID ;
      }

      INT32 _checkAndFixFileSize( UINT32 totalDataPages ) ;

      INT32 _extend( INT64 len ) ;

      INT32 _postOpen( INT32 cause ) ;

      void  _releaseFullPath() ;

   private:
      CHAR              _fileName[ DMS_SU_FILENAME_SZ + 1 ] ;
      UINT32            _fullPathSize ;
      CHAR             *_fullPath ;
      OSSFILE           _file ;
      INT64             _fileSz ;
      UINT32            _pageSz ;
      UINT32            _logarithmic ;
      UINT32            _flags ;
      UINT32            _segmentSize ;
      UINT32            _segmentPages ;
      UINT32            _segmentPagesSquare ;

   } ;
   typedef class _dmsStorageLobData dmsStorageLobData ;
}

#endif // DMS_STORAGELOBDATA_HPP_

