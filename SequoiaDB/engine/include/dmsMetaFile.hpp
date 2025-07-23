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

   Source File Name = dmsMetaFile.hpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of OSS component. This file contains declare for data types used in
   SequoiaDB.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          11/11/2024  XJH Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef DMSMETAFILE_H_
#define DMSMETAFILE_H_

#include "core.hpp"
#include "oss.hpp"
#include "ossIO.hpp"
#include "ossUtil.hpp"
#include "ossLatch.hpp"
#include "ossAtomic.hpp"
#include "ossMemPool.hpp"
#include "dms.hpp"
#include "monDMS.hpp"
#include "utilArray.hpp"

using namespace std ;

namespace engine
{
   #define DMS_METAFILE_NAME_POSIX              "meta"

   #define DMS_METAFILE_HEADER_EYECATCHER       "DMSMETAH"
   #define DMS_METAFILE_HEADER_EYECATCHER_LEN   8
   #define DMS_METAFILE_HEADER_LEN              (1 * 1024)

   #define DMS_METAFILE_VERSION1                (1)

#pragma pack(4)

   /*
      _dmsMetaFileHeader define
   */
   struct _dmsMetaFileHeader
   {
      CHAR     _eyeCatcher[ DMS_METAFILE_HEADER_EYECATCHER_LEN ] ;
      UINT32   _version ;
      UINT32   _checkSum ;
      UINT64   _updateTime ;

      /// MME
      UINT32   _collectionNum ;
      /// SME
      UINT32   _dataSMEItemNum ;
      UINT32   _idxSMEItemNum ;
      UINT32   _lobSMEItemNum ;

      CHAR     _padding [ DMS_METAFILE_HEADER_LEN - 40 ] ;

      _dmsMetaFileHeader ()
      {
         ossMemcpy( _eyeCatcher, DMS_METAFILE_HEADER_EYECATCHER,
                    DMS_METAFILE_HEADER_EYECATCHER_LEN) ;
         _version = DMS_METAFILE_VERSION1 ;
         _checkSum = 0 ;
         _updateTime = 0 ;

         _collectionNum = 0 ;
         _dataSMEItemNum = 0 ;
         _idxSMEItemNum = 0 ;
         _lobSMEItemNum = 0 ;

         ossMemset( _padding, 0, sizeof(_padding) ) ;

         SDB_ASSERT( sizeof(_dmsMetaFileHeader) == DMS_METAFILE_HEADER_LEN,
                     "DMS meta file header size must be 64K" ) ;
      }

      void reset()
      {
         _checkSum = 0 ;
         _updateTime = 0 ;
         _collectionNum = 0 ;
         _dataSMEItemNum = 0 ;
         _idxSMEItemNum = 0 ;
         _lobSMEItemNum = 0 ;
      }
   } ;

   typedef _dmsMetaFileHeader dmsMetaFileHeader ;

   /*
      File struct :

      |    Header ( 64K )   |
      |   dmsMBItem   | ... |          /// _collectionNum * dmsMBItem
      | DATA SME ITEM | ... |          /// _dataSMEItemNum * SMEItem
      | IDX SME ITEM  | ... |          /// _idxSMEItemNum * SMEItem
      | LOB SME ITEM  | ... |          /// _lobSMEItemNum * SMEItem
      |   Check Sum ( 4B )  |          /// End check sum
   */

   /*
      _dmsMBItem define
   */
   struct _dmsMBItem
   {
      UINT16      _mbID ;
      UINT8       _uniqueIdxNum ;
      UINT8       _textIdxNum ;
      UINT8       _globIdxNum ;
      UINT8       _pad1 ;
      UINT16      _pad2 ;

      _dmsMBItem( UINT16 mbID = 0,
                  UINT8 uniqueIdxNum = 0,
                  UINT8 textIdxNum = 0,
                  UINT8 globIdxNum = 0 )
      {
         _mbID = mbID ;
         _uniqueIdxNum = uniqueIdxNum ;
         _textIdxNum = textIdxNum ;
         _globIdxNum = globIdxNum ;
         _pad1 = 0 ;
         _pad2 = 0 ;
      }

      void setInfo( UINT8 uniqueIdxNum, UINT8 textIdxNum, UINT8 globIdxNum )
      {
         _uniqueIdxNum = uniqueIdxNum ;
         _textIdxNum = textIdxNum ;
         _globIdxNum = globIdxNum ;
      }

      bool operator< ( const _dmsMBItem &rhs ) const
      {
         return _mbID < rhs._mbID ;
      }

      bool operator== ( const _dmsMBItem &rhs ) const
      {
         return _mbID == rhs._mbID ? true : false ;
      }
   } ;
   typedef _dmsMBItem dmsMBItem ;

   /*
      SMEItem : UINT64 ( hi : start, lo : size )
   */

#pragma pack()

   #define DMS_META_FILE_NAMESIZE            ( DMS_COLLECTION_SPACE_NAME_SZ + 15 )

   /*
      _dmsCLIndexCache define
   */
   struct _dmsCLIndexCache : public SDBObject
   {
      vector<monIndex>           _vecIndex ;
      BOOLEAN                    _isCached ;
      UINT32                     _memSize ;
      UINT64                     _lastAccessTick ;
      ossSpinSLatch              _latch ;

      _dmsCLIndexCache()
      {
         _isCached = FALSE ;
         _memSize = 0 ;
         _lastAccessTick = 0 ;
      }
   } ;
   typedef _dmsCLIndexCache dmsCLIndexCache ;

   /*
      _dmsMetaFile define
   */
   class _dmsMetaFile : public SDBObject
   {
      typedef _utilSparseArray< dmsCLIndexCache, DMS_MME_SLOTS >     ARRAY_INDEXCACHE  ;

   public:
      _dmsMetaFile( ossAtomic64 *pTotalCacheMem = NULL ) ;
      ~_dmsMetaFile() ;

      void lock() ;
      void unlock() ;

   public:
      INT32 init( const CHAR *parentDir, const CHAR *csName ) ;
      INT32 rename( const CHAR *newCSName ) ;
      void  invalidate( BOOLEAN force = FALSE, BOOLEAN needLock = TRUE ) ;
      void  remove() ;

      void  enable() ;
      void  disable() ;

      BOOLEAN isValid() const ;

      UINT16 beginMBID() const ;
      UINT16 nextMBID( UINT16 pos ) const ;
      BOOLEAN getMBItem( UINT16 mbID, dmsMBItem &mbItem ) const ;

      UINT64 beginSMEItem( UINT32 fileType ) const ;
      UINT64 nextSMEItem( UINT32 fileType, UINT64 pos ) const ;

      /*
         Make sure is invalidate before
      */
      INT32  addMBItem( const dmsMBItem &mbItem ) ;
      INT32  addSMEItem( UINT32 fileType, UINT64 smeItem ) ;
      INT32  writeDone( const CHAR *csName ) ;

      BOOLEAN checkSize() const ;

      /*
         Index cache
      */
      void     invalidateAllIndexCache() ;
      void     invalidateOutOfDataCache( UINT64 expiredMs ) ;
      void     invalidateIndexCache( UINT16 mbID,
                                     const CHAR *csName = NULL,
                                     const CHAR *clName = NULL ) ;

      INT32    getIndexCache( UINT16 mbID,
                              MON_IDX_LIST &indexes,
                              BOOLEAN &isCacheValid ) ;

      INT32    getIndexCache( UINT16 mbID,
                              const CHAR *pIndexName,
                              monIndex &index,
                              BOOLEAN &isCacheValid ) ;

      INT32    pushIndexCache( UINT16 mbID,
                               const MON_IDX_LIST &indexes,
                               const CHAR *csName = NULL,
                               const CHAR *clName = NULL ) ;

      UINT64   getOldestAccessTick() const ;

   private:
      const SET_UINT64* _getSMEByType( UINT32 fileType ) const ;
      INT32 _parseData( const CHAR *pData, UINT32 buffSize ) ;
      INT32 _fillData( CHAR *pData, UINT32 buffSize ) ;
      void  _clear() ;
      INT32 _restore( BOOLEAN &isValid ) ;
      UINT32 _calcCheckSum( const CHAR *pData, UINT32 len ) ;

   private:
      CHAR               _szFileName[ DMS_META_FILE_NAMESIZE + 1 ] ;
      _OSS_FILE          _file ;
      string             _path ;
      dmsMetaFileHeader  _header ;
      BOOLEAN            _invalidateStatus ;
      INT32              _errorRC ;
      BOOLEAN            _enabled ;

      ossPoolSet<dmsMBItem> _setMBID ;
      SET_UINT64         _setDataSME ;
      SET_UINT64         _setIdxSME ;
      SET_UINT64         _setLobSME ;

      ARRAY_INDEXCACHE  _arrayIndexCache ;
      ossAtomic32       _cachedIndexCLNum ;

      ossAtomic64      *_pTotalCacheMem ;

      ossSpinXLatch     _mtx ;
   } ;

   typedef _dmsMetaFile dmsMetaFile ;

   /*
      Global function
   */
   ossAtomic64* dmsGetTotalIndexMemSize() ;

}

#endif // DMSMETAFILE_H_

