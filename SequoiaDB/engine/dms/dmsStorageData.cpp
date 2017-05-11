/*******************************************************************************


   Copyright (C) 2011-2014 SequoiaDB Ltd.

   This program is free software: you can redistribute it and/or modify
   it under the term of the GNU Affero General Public License, version 3,
   as published by the Free Software Foundation.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warrenty of
   MARCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program. If not, see <http://www.gnu.org/license/>.

   Source File Name = dmsStorageData.cpp

   Descriptive Name = Data Management Service Storage Unit Header

   When/how to use: this program may be used on binary and text-formatted
   versions of data management component. This file contains structure for
   DMS storage unit and its methods.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          14/08/2013  XJH Initial Draft

   Last Changed =

*******************************************************************************/

#include "dmsStorageData.hpp"
#include "dmsStorageIndex.hpp"
#include "dmsStorageLob.hpp"
#include "pmd.hpp"
#include "dpsTransCB.hpp"
#include "dpsOp2Record.hpp"
#include "mthModifier.hpp"
#include "ixm.hpp"
#include "pdTrace.hpp"
#include "dmsTrace.hpp"
#include "pd.hpp"
#include "utilCompressor.hpp"

using namespace bson ;

namespace engine
{

   #define DMS_MB_FLAG_FREE_STR                       "Free"
   #define DMS_MB_FLAG_USED_STR                       "Used"
   #define DMS_MB_FLAG_DROPED_STR                     "Dropped"
   #define DMS_MB_FLAG_OFFLINE_REORG_STR              "Offline Reorg"
   #define DMS_MB_FLAG_ONLINE_REORG_STR               "Online Reorg"
   #define DMS_MB_FLAG_LOAD_STR                       "Load"
   #define DMS_MB_FLAG_OFFLINE_REORG_SHADOW_COPY_STR  "Shadow Copy"
   #define DMS_MB_FLAG_OFFLINE_REORG_TRUNCATE_STR     "Truncate"
   #define DMS_MB_FLAG_OFFLINE_REORG_COPY_BACK_STR    "Copy Back"
   #define DMS_MB_FLAG_OFFLINE_REORG_REBUILD_STR      "Rebuild"
   #define DMS_MB_FLAG_LOAD_LOAD_STR                  "Load"
   #define DMS_MB_FLAG_LOAD_BUILD_STR                 "Build"
   #define DMS_MB_FLAG_UNKNOWN                        "Unknown"

   #define DMS_STATUS_SEPARATOR                       " | "

   static void appendFlagString( CHAR * pBuffer, INT32 bufSize,
                                 const CHAR *flagStr )
   {
      if ( 0 != *pBuffer )
      {
         ossStrncat( pBuffer, DMS_STATUS_SEPARATOR,
                     bufSize - ossStrlen( pBuffer ) ) ;
      }
      ossStrncat( pBuffer, flagStr, bufSize - ossStrlen( pBuffer ) ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__MBFLAG2STRING, "mbFlag2String" )
   void mbFlag2String( UINT16 flag, CHAR * pBuffer, INT32 bufSize )
   {
      PD_TRACE_ENTRY ( SDB__MBFLAG2STRING ) ;
      SDB_ASSERT ( pBuffer, "pBuffer can't be NULL" ) ;
      ossMemset ( pBuffer, 0, bufSize ) ;
      // Free
      if ( DMS_IS_MB_FREE ( flag ) )
      {
         appendFlagString( pBuffer, bufSize, DMS_MB_FLAG_FREE_STR ) ;
         goto done ;
      }

      // Used
      if ( OSS_BIT_TEST ( flag, DMS_MB_FLAG_USED ) )
      {
         appendFlagString( pBuffer, bufSize, DMS_MB_FLAG_USED_STR ) ;
         OSS_BIT_CLEAR( flag, DMS_MB_FLAG_USED ) ;
      }
      // Dropped
      if ( OSS_BIT_TEST ( flag, DMS_MB_FLAG_DROPED ) )
      {
         appendFlagString( pBuffer, bufSize, DMS_MB_FLAG_DROPED_STR ) ;
         OSS_BIT_CLEAR( flag, DMS_MB_FLAG_DROPED ) ;
      }

      // Offline Reorg
      if ( OSS_BIT_TEST ( flag, DMS_MB_FLAG_OFFLINE_REORG ) )
      {
         appendFlagString( pBuffer, bufSize, DMS_MB_FLAG_OFFLINE_REORG_STR ) ;
         OSS_BIT_CLEAR( flag, DMS_MB_FLAG_OFFLINE_REORG ) ;

         // Shadow Copy
         if ( OSS_BIT_TEST ( flag, DMS_MB_FLAG_OFFLINE_REORG_SHADOW_COPY ) )
         {
            appendFlagString( pBuffer, bufSize,
                              DMS_MB_FLAG_OFFLINE_REORG_SHADOW_COPY_STR ) ;
            OSS_BIT_CLEAR( flag, DMS_MB_FLAG_OFFLINE_REORG_SHADOW_COPY ) ;
         }
         // Truncate
         if ( OSS_BIT_TEST ( flag, DMS_MB_FLAG_OFFLINE_REORG_TRUNCATE ) )
         {
            appendFlagString( pBuffer, bufSize,
                              DMS_MB_FLAG_OFFLINE_REORG_TRUNCATE_STR ) ;
            OSS_BIT_CLEAR( flag, DMS_MB_FLAG_OFFLINE_REORG_TRUNCATE ) ;
         }
         // Copy Back
         if ( OSS_BIT_TEST ( flag, DMS_MB_FLAG_OFFLINE_REORG_COPY_BACK ) )
         {
            appendFlagString( pBuffer, bufSize,
                              DMS_MB_FLAG_OFFLINE_REORG_COPY_BACK_STR ) ;
            OSS_BIT_CLEAR( flag, DMS_MB_FLAG_OFFLINE_REORG_COPY_BACK ) ;
         }
         // Rebuild
         if ( OSS_BIT_TEST ( flag, DMS_MB_FLAG_OFFLINE_REORG_REBUILD ) )
         {
            appendFlagString( pBuffer, bufSize,
                              DMS_MB_FLAG_OFFLINE_REORG_REBUILD_STR ) ;
            OSS_BIT_CLEAR( flag, DMS_MB_FLAG_OFFLINE_REORG_REBUILD ) ;
         }
      }
      // Online Reorg
      if ( OSS_BIT_TEST ( flag, DMS_MB_FLAG_ONLINE_REORG ) )
      {
         appendFlagString( pBuffer, bufSize, DMS_MB_FLAG_ONLINE_REORG_STR ) ;
         OSS_BIT_CLEAR( flag, DMS_MB_FLAG_ONLINE_REORG ) ;
      }
      // load
      if ( OSS_BIT_TEST ( flag, DMS_MB_FLAG_LOAD ) )
      {
         appendFlagString( pBuffer, bufSize, DMS_MB_FLAG_LOAD_LOAD_STR ) ;
         OSS_BIT_CLEAR( flag, DMS_MB_FLAG_LOAD ) ;

         // load
         if ( OSS_BIT_TEST ( flag, DMS_MB_FLAG_LOAD_LOAD ) )
         {
            appendFlagString( pBuffer, bufSize, DMS_MB_FLAG_LOAD_LOAD_STR ) ;
            OSS_BIT_CLEAR( flag, DMS_MB_FLAG_LOAD_LOAD ) ;
         }
         // load build
         if ( OSS_BIT_TEST ( flag, DMS_MB_FLAG_LOAD_BUILD ) )
         {
            appendFlagString( pBuffer, bufSize, DMS_MB_FLAG_LOAD_BUILD_STR ) ;
            OSS_BIT_CLEAR( flag, DMS_MB_FLAG_LOAD_BUILD ) ;
         }
      }

      // Test other bits
      if ( flag )
      {
         appendFlagString( pBuffer, bufSize, DMS_MB_FLAG_UNKNOWN ) ;
      }
   done :
      PD_TRACE2 ( SDB__MBFLAG2STRING,
                  PD_PACK_USHORT ( flag ),
                  PD_PACK_STRING ( pBuffer ) ) ;
      PD_TRACE_EXIT ( SDB__MBFLAG2STRING ) ;
   }

   #define DMS_MB_ATTR_COMPRESSED_STR                        "Compressed"
   #define DMS_MB_ATTR_NOIDINDEX_STR                         "NoIDIndex"
   // PD_TRACE_DECLARE_FUNCTION ( SDB__MBATTR2STRING, "mbAttr2String" )
   void mbAttr2String( UINT32 attributes, CHAR * pBuffer, INT32 bufSize )
   {
      PD_TRACE_ENTRY ( SDB__MBATTR2STRING ) ;
      SDB_ASSERT ( pBuffer, "pBuffer can't be NULL" ) ;
      ossMemset ( pBuffer, 0, bufSize ) ;

      if ( OSS_BIT_TEST ( attributes, DMS_MB_ATTR_COMPRESSED ) )
      {
         appendFlagString( pBuffer, bufSize, DMS_MB_ATTR_COMPRESSED_STR ) ;
         OSS_BIT_CLEAR( attributes, DMS_MB_ATTR_COMPRESSED ) ;
      }
      if ( OSS_BIT_TEST ( attributes, DMS_MB_ATTR_NOIDINDEX ) )
      {
         appendFlagString( pBuffer, bufSize, DMS_MB_ATTR_NOIDINDEX_STR ) ;
         OSS_BIT_CLEAR( attributes, DMS_MB_ATTR_NOIDINDEX ) ;
      }

      // Test other bits
      if ( attributes )
      {
         appendFlagString( pBuffer, bufSize, DMS_MB_FLAG_UNKNOWN ) ;
      }
      PD_TRACE2 ( SDB__MBATTR2STRING,
                  PD_PACK_UINT ( attributes ),
                  PD_PACK_STRING ( pBuffer ) ) ;
      PD_TRACE_EXIT ( SDB__MBATTR2STRING ) ;
   }

   /*
      _dmsMBContext implement
   */
   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSMBCONTEXT, "_dmsMBContext::_dmsMBContext" )
   _dmsMBContext::_dmsMBContext ()
   {
      PD_TRACE_ENTRY ( SDB__DMSMBCONTEXT ) ;
      _reset () ;
      PD_TRACE_EXIT ( SDB__DMSMBCONTEXT ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSMBCONTEXT_DESC, "_dmsMBContext::~_dmsMBContext" )
   _dmsMBContext::~_dmsMBContext ()
   {
      PD_TRACE_ENTRY ( SDB__DMSMBCONTEXT_DESC ) ;
      _reset () ;
      PD_TRACE_EXIT ( SDB__DMSMBCONTEXT_DESC ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSMBCONTEXT__RESET, "_dmsMBContext::_reset" )
   void _dmsMBContext::_reset ()
   {
      PD_TRACE_ENTRY ( SDB__DMSMBCONTEXT__RESET ) ;
      _mb            = NULL ;
      _mbStat        = NULL ;
      _latch         = NULL ;
      _clLID         = DMS_INVALID_CLID ;
      _mbID          = DMS_INVALID_MBID ;
      _mbLockType    = -1 ;
      _resumeType    = -1 ;
      PD_TRACE_EXIT ( SDB__DMSMBCONTEXT__RESET ) ;
   }

   string _dmsMBContext::toString() const
   {
      stringstream ss ;
      ss << "dms-mb-context[" ;
      if ( _mb )
      {
         ss << "Name: " ;
         ss << _mb->_collectionName ;
         ss << ", " ;
      }
      ss << "ID: " << _mbID ;
      ss << ", LID: " << _clLID ;
      ss << ", LockType: " << _mbLockType ;
      ss << ", ResumeType: " << _resumeType ;

      ss << " ]" ;

      return ss.str() ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSMBCONTEXT_PAUSE, "_dmsMBContext::pause" )
   INT32 _dmsMBContext::pause()
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__DMSMBCONTEXT_PAUSE ) ;
      if ( SHARED == _mbLockType || EXCLUSIVE == _mbLockType )
      {
         _resumeType = _mbLockType ;
         rc = mbUnlock() ;
      }
      PD_TRACE_EXITRC ( SDB__DMSMBCONTEXT_PAUSE, rc ) ;
      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSMBCONTEXT_RESUME, "_dmsMBContext::resume" )
   INT32 _dmsMBContext::resume()
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__DMSMBCONTEXT_RESUME ) ;
      if ( SHARED == _resumeType || EXCLUSIVE == _resumeType )
      {
         INT32 lockType = _resumeType ;
         _resumeType = -1 ;
         rc = mbLock( lockType ) ;
      }
      PD_TRACE_EXITRC ( SDB__DMSMBCONTEXT_RESUME, rc ) ;
      return rc ;
   }

   /*
      _dmsRecordRW implement
   */
   _dmsRecordRW::_dmsRecordRW()
   :_ptr( NULL )
   {
      _pData = NULL ;
   }

   _dmsRecordRW::~_dmsRecordRW()
   {
   }

   BOOLEAN _dmsRecordRW::isEmpty() const
   {
      return _pData ? FALSE : TRUE ;
   }

   _dmsRecordRW _dmsRecordRW::derive( const dmsRecordID &rid )
   {
      if ( _pData )
      {
         return _pData->record2RW( rid, _rw.getCollectionID() ) ;
      }
      return _dmsRecordRW() ;
   }

   _dmsRecordRW _dmsRecordRW::deriveNext()
   {
      if ( _ptr && DMS_INVALID_OFFSET != _ptr->_nextOffset )
      {
         return _pData->record2RW( dmsRecordID( _rid._extent,
                                                _ptr->_nextOffset ),
                                   _rw.getCollectionID() ) ;
      }
      return _dmsRecordRW() ;
   }

   _dmsRecordRW _dmsRecordRW::derivePre()
   {
      if ( _ptr && DMS_INVALID_OFFSET != _ptr->_previousOffset )
      {
         return _pData->record2RW( dmsRecordID( _rid._extent,
                                                _ptr->_previousOffset ),
                                   _rw.getCollectionID() ) ;
      }
      return _dmsRecordRW() ;
   }

   _dmsRecordRW _dmsRecordRW::deriveOverflow()
   {
      if ( _ptr && _ptr->isOvf() )
      {
         return _pData->record2RW( _ptr->getOvfRID(), _rw.getCollectionID() ) ;
      }
      return _dmsRecordRW() ;
   }

   void _dmsRecordRW::setNothrow( BOOLEAN nothrow )
   {
      _rw.setNothrow( nothrow ) ;
   }

   BOOLEAN _dmsRecordRW::isNothrow() const
   {
      return _rw.isNothrow() ;
   }

   const dmsRecord* _dmsRecordRW::readPtr( UINT32 len )
   {
      if ( !_ptr )
      {
         std::string text = "Point is NULL: " ;
         text += toString() ;

         if ( isNothrow() )
         {
            PD_LOG( PDERROR, "Exception: %s", text.c_str() ) ;
            pdSetLastError( SDB_SYS ) ;
            return NULL ;
         }
         throw pdGeneralException( SDB_SYS, text ) ;
      }
      if ( 0 == len )
      {
         len = _ptr->getSize() ;
      }
      if ( len > DMS_RECORD_MAX_SZ )
      {
         std::string text = "Record size is grater than max record size: " ;
         text += toString() ;

         if ( isNothrow() )
         {
            PD_LOG( PDERROR, "Exception: %s", text.c_str() ) ;
            pdSetLastError( SDB_DMS_CORRUPTED_EXTENT ) ;
            return NULL ;
         }
         throw pdGeneralException( SDB_DMS_CORRUPTED_EXTENT, text ) ;
      }
      return (const dmsRecord*)_rw.readPtr( _rid._offset, len ) ;
   }

   dmsRecord* _dmsRecordRW::writePtr( UINT32 len )
   {
      if ( !_ptr )
      {
         std::string text = "Point is NULL: " ;
         text += toString() ;

         if ( isNothrow() )
         {
            PD_LOG( PDERROR, "Exception: %s", text.c_str() ) ;
            pdSetLastError( SDB_SYS ) ;
            return NULL ;
         }
         throw pdGeneralException( SDB_SYS, text ) ;
      }
      if ( 0 == len )
      {
         len = _ptr->getSize() ;
      }
      if ( len > DMS_RECORD_MAX_SZ )
      {
         std::string text = "Record size is grater than max record size: " ;
         text += toString() ;

         if ( isNothrow() )
         {
            PD_LOG( PDERROR, "Exception: %s", text.c_str() ) ;
            pdSetLastError( SDB_DMS_CORRUPTED_EXTENT ) ;
            return NULL ;
         }
         throw pdGeneralException( SDB_DMS_CORRUPTED_EXTENT, text ) ;
      }
      return (dmsRecord*)_rw.writePtr( _rid._offset, len ) ;
   }

   std::string _dmsRecordRW::toString() const
   {
      std::stringstream ss ;
      ss << "RecordRW(" << _rw.getCollectionID()
         << "," << _rid._extent << "," << _rid._offset << ")" ;
      return ss.str() ;
   }

   void _dmsRecordRW::_doneAddr()
   {
      BOOLEAN oldThrow = _rw.isNothrow() ;
      /// Need to read the overflow rid
      UINT32 len = DMS_RECORD_METADATA_SZ + sizeof( dmsRecordID ) ;
      _ptr = (const dmsRecord*)_rw.readPtr( _rid._offset, len ) ;
      /// Restore nothrow
      _rw.setNothrow( oldThrow ) ;
   }

   /*
      Local static variable define
   */
   static BSONObj s_idKeyObj = BSON( IXM_FIELD_NAME_KEY <<
                                     BSON( DMS_ID_KEY_NAME << 1 ) <<
                                     IXM_FIELD_NAME_NAME << IXM_ID_KEY_NAME
                                     << IXM_FIELD_NAME_UNIQUE <<
                                     true << IXM_FIELD_NAME_V << 0 <<
                                     IXM_FIELD_NAME_ENFORCED << true ) ;


   /*
      _dmsStorageData implement
   */
   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSTORAGEDATA, "_dmsStorageData::_dmsStorageData" )
   _dmsStorageData::_dmsStorageData ( const CHAR *pSuFileName,
                                      dmsStorageInfo *pInfo )
   :_dmsStorageBase( pSuFileName, pInfo )
   {
      PD_TRACE_ENTRY ( SDB__DMSSTORAGEDATA ) ;
      _dmsMME           = NULL ;
      _pIdxSU           = NULL ;
      _pLobSU           = NULL ;
      _logicalCSID      = 0 ;
      _CSID             = DMS_INVALID_SUID ;
      _mmeSegID         = 0 ;
      PD_TRACE_EXIT ( SDB__DMSSTORAGEDATA ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSTORAGEDATA_DESC, "_dmsStorageData::~_dmsStorageData" )
   _dmsStorageData::~_dmsStorageData ()
   {
      PD_TRACE_ENTRY ( SDB__DMSSTORAGEDATA_DESC ) ;
      _collectionNameMapCleanup() ;

      vector<dmsMBContext*>::iterator it = _vecContext.begin() ;
      while ( it != _vecContext.end() )
      {
         SDB_OSS_DEL (*it) ;
         ++it ;
      }
      _vecContext.clear() ;

      _pIdxSU = NULL ;
      _pLobSU = NULL ;

      PD_TRACE_EXIT ( SDB__DMSSTORAGEDATA_DESC ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSTORAGEDATA_SYNCMEMTOMMAP, "_dmsStorageData::syncMemToMmap" )
   void _dmsStorageData::syncMemToMmap ()
   {
      PD_TRACE_ENTRY ( SDB__DMSSTORAGEDATA_SYNCMEMTOMMAP ) ;
      // write total count to disk
      for ( UINT32 i = 0 ; i < DMS_MME_SLOTS ; ++i )
      {
         if ( DMS_IS_MB_INUSE ( _dmsMME->_mbList[i]._flag ) )
         {
            if ( _dmsMME->_mbList[i]._totalRecords !=
                 _mbStatInfo[i]._totalRecords )
            {
               _dmsMME->_mbList[i]._totalRecords =
                  _mbStatInfo[i]._totalRecords ;
            }
            if ( _dmsMME->_mbList[i]._totalDataPages !=
                 _mbStatInfo[i]._totalDataPages )
            {
               _dmsMME->_mbList[i]._totalDataPages =
                  _mbStatInfo[i]._totalDataPages ;
            }
            if ( _dmsMME->_mbList[i]._totalIndexPages !=
                 _mbStatInfo[i]._totalIndexPages )
            {
               _dmsMME->_mbList[i]._totalIndexPages =
                  _mbStatInfo[i]._totalIndexPages ;
            }
            if ( _dmsMME->_mbList[i]._totalDataFreeSpace !=
                 _mbStatInfo[i]._totalDataFreeSpace )
            {
               _dmsMME->_mbList[i]._totalDataFreeSpace =
                  _mbStatInfo[i]._totalDataFreeSpace ;
            }
            if ( _dmsMME->_mbList[i]._totalIndexFreeSpace !=
                 _mbStatInfo[i]._totalIndexFreeSpace )
            {
               _dmsMME->_mbList[i]._totalIndexFreeSpace =
                  _mbStatInfo[i]._totalIndexFreeSpace ;
            }
            if ( _dmsMME->_mbList[i]._totalLobPages !=
                 _mbStatInfo[i]._totalLobPages )
            {
               _dmsMME->_mbList[i]._totalLobPages =
                  _mbStatInfo[i]._totalLobPages ;
            }
            if ( _dmsMME->_mbList[i]._totalLobs !=
                 _mbStatInfo[i]._totalLobs )
            {
               _dmsMME->_mbList[i]._totalLobs =
                  _mbStatInfo[i]._totalLobs ;
            }
            if ( _dmsMME->_mbList[i]._lastCompressRatio !=
                 _mbStatInfo[i]._lastCompressRatio )
            {
               _dmsMME->_mbList[i]._lastCompressRatio =
                  _mbStatInfo[i]._lastCompressRatio ;
            }
            if ( _dmsMME->_mbList[i]._totalDataLen !=
                 _mbStatInfo[i]._totalDataLen )
            {
               _dmsMME->_mbList[i]._totalDataLen =
                  _mbStatInfo[i]._totalDataLen ;
            }
            if ( _dmsMME->_mbList[i]._totalOrgDataLen !=
                 _mbStatInfo[i]._totalOrgDataLen )
            {
               _dmsMME->_mbList[i]._totalOrgDataLen =
                  _mbStatInfo[i]._totalOrgDataLen ;
            }
            if ( _dmsMME->_mbList[i]._commitLSN !=
                 _mbStatInfo[i]._lastLSN.peek() )
            {
               _dmsMME->_mbList[i]._commitLSN =
                  _mbStatInfo[i]._lastLSN.peek() ;
            }
            if ( _dmsMME->_mbList[i]._idxCommitLSN !=
                 _mbStatInfo[i]._idxLastLSN.peek() )
            {
               _dmsMME->_mbList[i]._idxCommitLSN =
                  _mbStatInfo[i]._idxLastLSN.peek() ;
            }
            if ( _dmsMME->_mbList[i]._lobCommitLSN !=
                 _mbStatInfo[i]._lobLastLSN.peek() )
            {
               _dmsMME->_mbList[i]._lobCommitLSN =
                  _mbStatInfo[i]._lobLastLSN.peek() ;
            }
         }
      }
      PD_TRACE_EXIT ( SDB__DMSSTORAGEDATA_SYNCMEMTOMMAP ) ;
   }

   INT32 _dmsStorageData::flushMME( BOOLEAN sync )
   {
      syncMemToMmap() ;
      return flushSegment( _mmeSegID, sync ) ;
   }

   void _dmsStorageData::_attach( _dmsStorageIndex * pIndexSu )
   {
      SDB_ASSERT( pIndexSu, "Index su can't be NULL" ) ;
      _pIdxSU = pIndexSu ;
   }

   void _dmsStorageData::_detach ()
   {
      _pIdxSU = NULL ;
   }

   void _dmsStorageData::_attachLob( _dmsStorageLob * pLobSu )
   {
      SDB_ASSERT( pLobSu, "Lob su can't be NULL" ) ;
      _pLobSU = pLobSu ;
   }

   void _dmsStorageData::_detachLob()
   {
      _pLobSU = NULL ;
   }

   UINT64 _dmsStorageData::_dataOffset ()
   {
      return ( DMS_MME_OFFSET + DMS_MME_SZ ) ;
   }

   const CHAR* _dmsStorageData::_getEyeCatcher () const
   {
      return DMS_DATASU_EYECATCHER ;
   }

   UINT32 _dmsStorageData::_curVersion () const
   {
      return DMS_DATASU_CUR_VERSION ;
   }

   INT32 _dmsStorageData::_checkVersion( dmsStorageUnitHeader * pHeader )
   {
      INT32 rc = SDB_OK ;

      if ( pHeader->_version > _curVersion() )
      {
         PD_LOG( PDERROR, "Incompatible version: %u", pHeader->_version ) ;
         rc = SDB_DMS_INCOMPATIBLE_VERSION ;
      }
      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSTORAGEDATA__ONCREATE, "_dmsStorageData::_onCreate" )
   INT32 _dmsStorageData::_onCreate( OSSFILE * file, UINT64 curOffSet )
   {
      INT32 rc          = SDB_OK ;
      _dmsMME           = NULL ;

      PD_TRACE_ENTRY ( SDB__DMSSTORAGEDATA__ONCREATE ) ;
      SDB_ASSERT( DMS_MME_OFFSET == curOffSet, "Offset is not MME offset" ) ;

      _dmsMME = SDB_OSS_NEW dmsMetadataManagementExtent ;
      if ( !_dmsMME )
      {
         PD_LOG ( PDSEVERE, "Failed to allocate memory to for dmsMME" ) ;
         PD_LOG ( PDSEVERE, "Requested memory: %d bytes", DMS_MME_SZ ) ;
         rc = SDB_OOM ;
         goto error ;
      }
      _initializeMME () ;

      rc = _writeFile ( file, (CHAR *)_dmsMME, DMS_MME_SZ ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Failed to write to file duirng SU init, rc: %d",
                  rc ) ;
         goto error ;
      }
      SDB_OSS_DEL _dmsMME ;
      _dmsMME = NULL ;

   done:
      PD_TRACE_EXITRC ( SDB__DMSSTORAGEDATA__ONCREATE, rc ) ;
      return rc ;
   error:
      if ( _dmsMME )
      {
         SDB_OSS_DEL _dmsMME ;
         _dmsMME = NULL ;
      }
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSTORAGEDATA__INITCOMPRESSORENTRY, "_dmsStorageData::_initCompressorEntry" )
   INT32 _dmsStorageData::_initCompressorEntry( UINT16 mbID )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__DMSSTORAGEDATA__INITCOMPRESSORENTRY ) ;
      const dmsDictExtent *dictExtent = NULL ;
      dmsExtentID dictExtID = _dmsMME->_mbList[mbID]._dictExtentID ;
      UTIL_COMPRESSOR_TYPE type =
         ( UTIL_COMPRESSOR_TYPE )_dmsMME->_mbList[mbID]._compressorType ;
      utilCompressor *compressor = NULL ;

      dmsCompressorGuard compGuard( &_compressorEntry[mbID], EXCLUSIVE ) ;

      /*
       * If the compression type is lzw and the dictionary has not been created,
       * return directly.
       */
      if ( UTIL_COMPRESSOR_LZW == type && DMS_INVALID_EXTENT == dictExtID )
      {
         goto done ;
      }

      compressor = getCompressorByType( type ) ;
      SDB_ASSERT( compressor, "compressor pointer should not be NULL" ) ;
      if ( !compressor )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "Failed to get compressor for collection[%s], "
                 "type: %d, rc: %d", _dmsMME->_mbList[mbID]._collectionName,
                 type, rc ) ;
         goto error ;
      }
      _compressorEntry[mbID].setCompressor( compressor ) ;

      if ( UTIL_COMPRESSOR_LZW == type )
      {
         dmsExtRW rw = extent2RW( dictExtID, mbID ) ;
         rw.setNothrow( TRUE ) ;
         dictExtent = rw.readPtr<dmsDictExtent>() ;
         if ( !dictExtent || !dictExtent->validate( mbID ) )
         {
            PD_LOG( PDERROR, "Dictionary extent is invalid. Extent id: %d",
                    dictExtID ) ;
            rc = SDB_DMS_CORRUPTED_EXTENT ;
            goto error ;
         }

         _compressorEntry[mbID].setDictionary(
            (const utilDictHandle)( beginFixedAddr( dictExtID,
                                                    dictExtent->_blockSize ) +
                                    DMS_DICTEXTENT_HEADER_SZ ) ) ;
      }
   done:
      PD_TRACE_EXITRC( SDB__DMSSTORAGEDATA__INITCOMPRESSORENTRY, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSTORAGEDATA__ONMAPMETA, "_dmsStorageData::_onMapMeta" )
   INT32 _dmsStorageData::_onMapMeta( UINT64 curOffSet )
   {
      INT32 rc = SDB_OK ;
      BOOLEAN upgradeDictInfo = FALSE ;
      BOOLEAN needFlush = FALSE ;

      PD_TRACE_ENTRY ( SDB__DMSSTORAGEDATA__ONMAPMETA ) ;
      // MME, 4MB
      _mmeSegID = ossMmapFile::segmentSize() ;
      rc = map ( DMS_MME_OFFSET, DMS_MME_SZ, (void**)&_dmsMME ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Failed to map MME: %s", getSuFileName() ) ;
         goto error ;
      }

      if ( _dmsHeader->_version < DMS_COMPRESSION_ENABLE_VER )
      {
         upgradeDictInfo = TRUE ;
      }

      // load collection names in the SU
      for ( UINT16 i = 0 ; i < DMS_MME_SLOTS ; i++ )
      {
         _mbStatInfo[i]._lastWriteTick = ~0 ;
         _mbStatInfo[i]._commitFlag.init( 1 ) ;

         if ( DMS_IS_MB_INUSE ( _dmsMME->_mbList[i]._flag ) )
         {
            _collectionNameInsert ( _dmsMME->_mbList[i]._collectionName, i ) ;

            _mbStatInfo[i]._totalRecords = _dmsMME->_mbList[i]._totalRecords ;
            _mbStatInfo[i]._totalDataPages =
               _dmsMME->_mbList[i]._totalDataPages ;
            _mbStatInfo[i]._totalIndexPages =
               _dmsMME->_mbList[i]._totalIndexPages ;
            _mbStatInfo[i]._totalDataFreeSpace =
               _dmsMME->_mbList[i]._totalDataFreeSpace ;
            _mbStatInfo[i]._totalIndexFreeSpace =
               _dmsMME->_mbList[i]._totalIndexFreeSpace ;
            _mbStatInfo[i]._totalLobPages =
               _dmsMME->_mbList[i]._totalLobPages ;
            _mbStatInfo[i]._totalLobs =
               _dmsMME->_mbList[i]._totalLobs ;
            _mbStatInfo[i]._lastCompressRatio =
               _dmsMME->_mbList[i]._lastCompressRatio ;
            _mbStatInfo[i]._totalDataLen =
               _dmsMME->_mbList[i]._totalDataLen ;
            _mbStatInfo[i]._totalOrgDataLen =
               _dmsMME->_mbList[i]._totalOrgDataLen ;
            /*
             * The following branch is for using newer program(SequoiaDB 2.0 or
             * later) with data of elder versions(Before 2.0). As dictionary
             * compression is enabled in 2.0, the data needs to upgrade,
             * because dictionary information such as dictionary extent id is
             * added in MB.
             * As for now, we do not change the version number in dms header, so
             * the second part of the following if statement condition is
             * needed, to avoid changing the dictionary related ids every time.
             * That would be done only the first time after upgrading.
             */
            if ( upgradeDictInfo && ( 0 == _dmsMME->_mbList[i]._dictExtentID ) )
            {
               _dmsMME->_mbList[i]._dictExtentID = DMS_INVALID_EXTENT ;
            }

            /*
             * In version before 2.0, the byte _compressorType is taking now was
             * set to 0. But in the new version, 0 means using snappy to
             * compress. So during the upgrading, set the _comrpessorType to 255
             * ( UTIL_COMPRESSOR_INVALID).
             */
            if ( upgradeDictInfo
                 && !OSS_BIT_TEST( _dmsMME->_mbList[i]._attributes,
                                   DMS_MB_ATTR_COMPRESSED )
                 && ( UTIL_COMPRESSOR_INVALID
                      != _dmsMME->_mbList[i]._compressorType ) )
            {
               _dmsMME->_mbList[i]._compressorType = UTIL_COMPRESSOR_INVALID ;
            }

            /*
               Check the collection is valid
            */
            if ( !isCrashed() )
            {
               if ( 0 == _dmsMME->_mbList[i]._commitFlag )
               {
                  /// upgrade from the old version( _commitLSN = 0 )
                  if ( 0 == _dmsMME->_mbList[i]._commitLSN )
                  {
                     _dmsMME->_mbList[i]._commitLSN =
                        _pStorageInfo->_curLSNOnStart ;
                  }
                  _dmsMME->_mbList[i]._commitFlag = 1 ;
                  needFlush = TRUE ;
               }
               _mbStatInfo[i]._commitFlag.init( 1 ) ;
            }
            else
            {
               _mbStatInfo[i]._commitFlag.init( _dmsMME->_mbList[i]._commitFlag ) ;
            }
            _mbStatInfo[i]._isCrash = ( 0 == _mbStatInfo[i]._commitFlag.peek() ) ?
                                      TRUE : FALSE ;
            /// lsn
            _mbStatInfo[i]._lastLSN.init( _dmsMME->_mbList[i]._commitLSN ) ;
         }
      }

      if ( needFlush )
      {
         flushMME( isSyncDeep() ) ;
      }

   done:
      PD_TRACE_EXITRC ( SDB__DMSSTORAGEDATA__ONMAPMETA, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSTORAGEDATA__ONOPENED, "_dmsStorageData::_onOpened" )
   INT32 _dmsStorageData::_onOpened()
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( SDB__DMSSTORAGEDATA__ONOPENED ) ;
      /* Initialize compressor entries for collections. */
      for ( UINT16 i = 0 ; i < DMS_MME_SLOTS ; ++i )
      {
         if ( DMS_IS_MB_INUSE ( _dmsMME->_mbList[i]._flag ) &&
              OSS_BIT_TEST( _dmsMME->_mbList[i]._attributes,
                            DMS_MB_ATTR_COMPRESSED ) )
         {
            rc = _initCompressorEntry( i ) ;
            PD_RC_CHECK( rc, PDERROR,
                         "Failed to initialize compressor entry for "
                         "collection: %s, rc = %d",
                         _dmsMME->_mbList[i]._collectionName, rc ) ;
         }
      }

   done:
      PD_TRACE_EXITRC( SDB__DMSSTORAGEDATA__ONOPENED, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSTORAGEDATA__ONCLOSED, "_dmsStorageData::_onClosed" )
   void _dmsStorageData::_onClosed ()
   {
      PD_TRACE_ENTRY ( SDB__DMSSTORAGEDATA__ONCLOSED ) ;
      /// ensure static info will be flushed to file.
      /// do it here first.
      syncMemToMmap() ;

      PD_TRACE_EXIT ( SDB__DMSSTORAGEDATA__ONCLOSED ) ;
   }

   INT32 _dmsStorageData::_onFlushDirty( BOOLEAN force, BOOLEAN sync )
   {
      for ( UINT16 i = 0 ; i < DMS_MME_SLOTS ; ++i )
      {
         _mbStatInfo[i]._commitFlag.init( 1 ) ;
      }
      return SDB_OK ;
   }

   INT32 _dmsStorageData::_onMarkHeaderValid( UINT64 &lastLSN,
                                              BOOLEAN sync,
                                              UINT64 lastTime )
   {
      INT32 rc = SDB_OK ;
      BOOLEAN needFlush = FALSE ;
      UINT64 tmpLSN = 0 ;
      UINT32 tmpCommitFlag = 0 ;

      for ( UINT16 i = 0 ; i < DMS_MME_SLOTS ; ++i )
      {
         if ( DMS_IS_MB_INUSE ( _dmsMME->_mbList[i]._flag ) &&
              _mbStatInfo[i]._commitFlag.peek() )
         {
            tmpLSN = _mbStatInfo[i]._lastLSN.peek() ;
            tmpCommitFlag = _mbStatInfo[i]._isCrash ? 0 :
               _mbStatInfo[i]._commitFlag.peek() ;

            if ( tmpLSN != _dmsMME->_mbList[i]._commitLSN ||
                 tmpCommitFlag != _dmsMME->_mbList[i]._commitFlag )
            {
               _dmsMME->_mbList[i]._commitLSN = tmpLSN ;
               _dmsMME->_mbList[i]._commitTime = lastTime ;
               _dmsMME->_mbList[i]._commitFlag = tmpCommitFlag ;
               needFlush = TRUE ;
            }

            /// update last lsn
            if ( (UINT64)~0 == lastLSN ||
                 ( (UINT64)~0 != tmpLSN && lastLSN < tmpLSN ) )
            {
               lastLSN = tmpLSN ;
            }
         }
      }

      if ( needFlush )
      {
         rc = flushMME( sync ) ;
      }
      return rc ;
   }

   INT32 _dmsStorageData::_onMarkHeaderInvalid( INT32 collectionID )
   {
      INT32 rc = SDB_OK ;
      BOOLEAN needSync = FALSE ;

      if ( collectionID >= 0 && collectionID < DMS_MME_SLOTS )
      {
         _mbStatInfo[ collectionID ]._lastWriteTick = pmdGetDBTick() ;
         if ( !_mbStatInfo[ collectionID ]._isCrash &&
              _mbStatInfo[ collectionID ]._commitFlag.compareAndSwap( 1, 0 ) )
         {
            needSync = TRUE ;
            _dmsMME->_mbList[ collectionID ]._commitFlag = 0 ;
         }
      }
      else if ( -1 == collectionID )
      {
         for ( UINT16 i = 0 ; i < DMS_MME_SLOTS ; ++i )
         {
            _mbStatInfo[ i ]._lastWriteTick = pmdGetDBTick() ;
            if ( DMS_IS_MB_INUSE ( _dmsMME->_mbList[i]._flag ) &&
                 !_mbStatInfo[ i ]._isCrash &&
                 _mbStatInfo[ i ]._commitFlag.compareAndSwap( 1, 0 ) )
            {
               needSync = TRUE ;
               _dmsMME->_mbList[ i ]._commitFlag = 0 ;
            }
         }
      }

      if ( needSync )
      {
         rc = flushMME( isSyncDeep() ) ;
      }
      return rc ;
   }

   UINT64 _dmsStorageData::_getOldestWriteTick() const
   {
      UINT64 oldestWriteTick = ~0 ;
      UINT64 lastWriteTick = 0 ;

      for ( INT32 i = 0 ; i < DMS_MME_SLOTS ; i++ )
      {
         lastWriteTick = _mbStatInfo[i]._lastWriteTick ;
         /// The collection is commit valid, should ignored
         if ( 0 == _mbStatInfo[i]._commitFlag.peek() &&
              lastWriteTick < oldestWriteTick )
         {
            oldestWriteTick = lastWriteTick ;
         }
      }
      return oldestWriteTick ;
   }

   void _dmsStorageData::_onRestore()
   {
      for ( INT32 i = 0 ; i < DMS_MME_SLOTS ; i++ )
      {
         _mbStatInfo[i]._isCrash = FALSE ;
      }
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSTORAGEDATA__INITMME, "_dmsStorageData::_initializeMME" )
   void _dmsStorageData::_initializeMME ()
   {
      PD_TRACE_ENTRY ( SDB__DMSSTORAGEDATA__INITMME ) ;
      SDB_ASSERT ( _dmsMME, "MME is NULL" ) ;

      for ( INT32 i = 0; i < DMS_MME_SLOTS ; i++ )
      {
         _dmsMME->_mbList[i].reset () ;
      }
      PD_TRACE_EXIT ( SDB__DMSSTORAGEDATA__INITMME ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSTORAGEDATA__LOGDPS, "_dmsStorageData::_logDPS" )
   INT32 _dmsStorageData::_logDPS( SDB_DPSCB * dpsCB,
                                   dpsMergeInfo & info,
                                   pmdEDUCB * cb,
                                   ossSLatch * pLatch,
                                   OSS_LATCH_MODE mode,
                                   BOOLEAN & locked,
                                   UINT32 clLID,
                                   dmsExtentID extLID )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( SDB__DMSSTORAGEDATA__LOGDPS ) ;
      info.setInfoEx( _logicalCSID, clLID, extLID, cb ) ;
      rc = dpsCB->prepare( info ) ;
      if ( rc )
      {
         goto error ;
      }

      // release lock
      if ( pLatch && locked )
      {
         if ( SHARED == mode )
         {
            pLatch->release_shared() ;
         }
         else
         {
            pLatch->release() ;
         }
         locked = FALSE ;
      }
      // write dps
      dpsCB->writeData( info ) ;
   done :
      PD_TRACE_EXITRC ( SDB__DMSSTORAGEDATA__LOGDPS, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSTORAGEDATA__LOGDPS1, "_dmsStorageData::_logDPS" )
   INT32 _dmsStorageData::_logDPS( SDB_DPSCB *dpsCB, dpsMergeInfo &info,
                                   pmdEDUCB *cb, dmsMBContext *context,
                                   dmsExtentID extLID,
                                   BOOLEAN needUnLock,
                                   DMS_FILE_TYPE type,
                                   UINT32 *clLID )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__DMSSTORAGEDATA__LOGDPS1 ) ;
      UINT64 lsn = DPS_INVALID_LSN_OFFSET ;
      info.setInfoEx( logicalID(),
                      NULL == clLID ?
                      context->clLID() : *clLID,
                      extLID, cb ) ;
      rc = dpsCB->prepare( info ) ;
      if ( rc )
      {
         goto error ;
      }
      lsn = info.getMergeBlock().record().head()._lsn ;
      context->mbStat()->updateLastLSN( lsn, type ) ;

      // release lock
      if ( needUnLock )
      {
         context->mbUnlock() ;
      }

      // write dps
      dpsCB->writeData( info ) ;

   done :
      PD_TRACE_EXITRC ( SDB__DMSSTORAGEDATA__LOGDPS1, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSTORAGEDATA__ALLOCATEEXTENT, "_dmsStorageData::_allocateExtent" )
   INT32 _dmsStorageData::_allocateExtent( dmsMBContext *context,
                                           UINT16 numPages,
                                           BOOLEAN map2DelList,
                                           BOOLEAN add2LoadList,
                                           dmsExtentID *allocExtID )
   {
      SDB_ASSERT( context, "dms mb context can't be NULL" ) ;
      INT32 rc                 = SDB_OK ;
      SINT32 firstFreeExtentID = DMS_INVALID_EXTENT ;
      dmsExtRW extRW ;
      dmsExtent *extAddr       = NULL ;
      PD_TRACE_ENTRY ( SDB__DMSSTORAGEDATA__ALLOCATEEXTENT ) ;
      PD_TRACE3 ( SDB__DMSSTORAGEDATA__ALLOCATEEXTENT,
                  PD_PACK_USHORT ( numPages ),
                  PD_PACK_UINT ( map2DelList ),
                  PD_PACK_UINT ( add2LoadList ) ) ;
      if ( numPages > segmentPages() || numPages < 1 )
      {
         PD_LOG ( PDERROR, "Invalid number of pages: %d", numPages ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      rc = context->mbLock( EXCLUSIVE ) ;
      PD_RC_CHECK( rc, PDERROR, "dms mb lock failed, rc: %d", rc ) ;

      rc = _findFreeSpace ( numPages, firstFreeExtentID, context ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Error find free space for %d pages, rc = %d",
                  numPages, rc ) ;
         goto error ;
      }

      extRW = extent2RW( firstFreeExtentID, context->mbID() ) ;
      extRW.setNothrow( TRUE ) ;
      extAddr = extRW.writePtr<dmsExtent>() ;
      if ( !extAddr )
      {
         PD_LOG( PDERROR, "Get extent[%d] address failed",
                 firstFreeExtentID ) ;
         rc = SDB_SYS ;
         goto error ;
      }
      /// Init
      extAddr->init( numPages, context->mbID(),
                     (UINT32)numPages << pageSizeSquareRoot() ) ;

      // and add the new extent into MB's extent chain
      // now let's change the extent pointer into MB's extent list
      // new extent->preextent always assign to _mbList.lastExtentID
      if ( TRUE == add2LoadList )
      {
         extAddr->_prevExtent = context->mb()->_loadLastExtentID ;
         extAddr->_nextExtent = DMS_INVALID_EXTENT ;
         // if this is the load first extent in this MB, we assign
         // firstExtentID to it
         if ( DMS_INVALID_EXTENT == context->mb()->_loadFirstExtentID )
         {
            context->mb()->_loadFirstExtentID = firstFreeExtentID ;
         }

         if ( DMS_INVALID_EXTENT != extAddr->_prevExtent )
         {
            dmsExtRW prevRW = extent2RW( extAddr->_prevExtent,
                                         context->mbID() ) ;
            dmsExtent *prevExt = prevRW.writePtr<dmsExtent>() ;
            prevExt->_nextExtent = firstFreeExtentID ;
         }

         // MB's last extent always assigned to the new extent
         context->mb()->_loadLastExtentID = firstFreeExtentID ;
      }
      else
      {
         rc = addExtent2Meta( firstFreeExtentID, extAddr, context ) ;
         PD_RC_CHECK( rc, PDERROR, "Add extent to meta failed, rc: %d", rc ) ;
      }

      // Once we put the extent into linked list, next we want to break such
      // extent into multiple deleted records, and insert into delete list
      if ( map2DelList )
      {
         _mapExtent2DelList( context->mb(), extAddr, firstFreeExtentID ) ;
      }

      if ( allocExtID )
      {
         *allocExtID = firstFreeExtentID ;
         PD_TRACE1 ( SDB__DMSSTORAGEDATA__ALLOCATEEXTENT,
                     PD_PACK_INT ( firstFreeExtentID ) ) ;
      }

   done :
      PD_TRACE_EXITRC ( SDB__DMSSTORAGEDATA__ALLOCATEEXTENT, rc ) ;
      return rc ;
   error :
      if ( DMS_INVALID_EXTENT != firstFreeExtentID )
      {
         _freeExtent( firstFreeExtentID, context->mbID() ) ;
      }
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSTORAGEDATA__FREEEXTENT, "_dmsStorageData::_freeExtent" )
   INT32 _dmsStorageData::_freeExtent( dmsExtentID extentID,
                                       INT32 collectionID )
   {
      INT32 rc = SDB_OK ;
      dmsExtRW extRW ;
      const dmsExtent *extAddr = NULL ;
      PD_TRACE_ENTRY ( SDB__DMSSTORAGEDATA__FREEEXTENT ) ;
      if ( DMS_INVALID_EXTENT == extentID )
      {
         PD_LOG( PDERROR, "Invalid extent id for free" ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      PD_TRACE1 ( SDB__DMSSTORAGEDATA__FREEEXTENT,
                  PD_PACK_INT ( extentID ) ) ;
      extRW = extent2RW( extentID, collectionID ) ;
      extRW.setNothrow( TRUE ) ;
      extAddr = extRW.readPtr<dmsExtent>() ;

      if ( !extAddr || !extAddr->validate() )
      {
         PD_LOG ( PDERROR, "Invalid eye catcher or flag for extent %d",
                  extentID ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      /*
      * Not to write the extent, so perfermance will improved
      *
      extAddr->_flag = DMS_EXTENT_FLAG_FREED ;
      // change logical id
      extAddr->_logicID = DMS_INVALID_EXTENT ;
      */

      rc = _releaseSpace( extentID, extAddr->_blockSize ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Failed to release page, rc = %d", rc ) ;
         goto error ;
      }

   done :
      PD_TRACE_EXITRC ( SDB__DMSSTORAGEDATA__FREEEXTENT, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSTORAGEDATA__RESERVEFROMDELETELIST, "_dmsStorageData::_reserveFromDeleteList" )
   INT32 _dmsStorageData::_reserveFromDeleteList( dmsMBContext *context,
                                                  UINT32 requiredSize,
                                                  dmsRecordID &resultID,
                                                  pmdEDUCB * cb )
   {
      INT32 rc                      = SDB_OK ;
      UINT32 dmsRecordSizeTemp      = 0 ;
      UINT8  deleteRecordSlot       = 0 ;
      const static INT32 s_maxSearch = 3 ;

      INT32  j                      = 0 ;
      INT32  i                      = 0 ;
      dmsRecordID foundDeletedID  ;
      dmsRecordRW delRecordRW ;
      const dmsDeletedRecord* pRead = NULL ;
      dpsTransCB *pTransCB          = pmdGetKRCB()->getTransCB() ;

      PD_TRACE_ENTRY ( SDB__DMSSTORAGEDATA__RESERVEFROMDELETELIST ) ;
      PD_TRACE1 ( SDB__DMSSTORAGEDATA__RESERVEFROMDELETELIST,
                  PD_PACK_UINT ( requiredSize ) ) ;
      rc = context->mbLock( EXCLUSIVE ) ;
      PD_RC_CHECK( rc, PDERROR, "dms mb context lock failed, rc: %d", rc ) ;

   retry:
      // let's count which delete slots it fits
      // divide by 32 first since our first slot is for <32 bytes
      dmsRecordSizeTemp = ( requiredSize-1 ) >> 5 ;
      deleteRecordSlot  = 0 ;
      while ( dmsRecordSizeTemp != 0 )
      {
         deleteRecordSlot ++ ;
         dmsRecordSizeTemp = dmsRecordSizeTemp >> 1 ;
      }
      SDB_ASSERT( deleteRecordSlot < dmsMB::_max, "Invalid record size" ) ;

      if ( deleteRecordSlot >= dmsMB::_max )
      {
         PD_LOG( PDERROR, "Invalid record size: %u", requiredSize ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      rc = SDB_DMS_NOSPC ;
      try
      {
         for ( j = deleteRecordSlot ; j < dmsMB::_max ; ++j )
         {
            dmsRecordRW preRW ;
            // get the first delete record from delete list
            foundDeletedID = _dmsMME->_mbList[context->mbID()]._deleteList[j] ;
            for ( i = 0 ; i < s_maxSearch ; ++i )
            {
               // if we don't get a valid record id, we break to get next slot
               if ( foundDeletedID.isNull() )
               {
                  break ;
               }
               delRecordRW = record2RW( foundDeletedID, context->mbID() ) ;
               pRead = delRecordRW.readPtr<dmsDeletedRecord>() ;

               // once the extent is valid, let's check the record is deleted
               // and got sufficient size for us
               if( pRead->isDeleted() && pRead->getSize() >= requiredSize )
               {
                  if ( SDB_OK == pTransCB->transLockTestX( cb, _logicalCSID,
                                                           context->mbID(),
                                                           &foundDeletedID ) )
                  {
                     if ( preRW.isEmpty() )
                     {
                        // it's just the first one from delete list, let's get it
                        context->mb()->_deleteList[j] = pRead->getNextRID() ;
                     }
                     else
                     {
                        dmsDeletedRecord *preWrite =
                           preRW.writePtr<dmsDeletedRecord>() ;
                        // we need to link the previous delete record to the next
                        preWrite->setNextRID( pRead->getNextRID() ) ;
                     }

                     // change extent free space
                     dmsExtRW rw = extent2RW( foundDeletedID._extent,
                                              context->mbID() ) ;
                     dmsExtent *pExtent = rw.writePtr<dmsExtent>() ;
                     pExtent->_freeSpace -= pRead->getSize() ;
                     context->mbStat()->_totalDataFreeSpace -= pRead->getSize() ;

                     resultID = foundDeletedID ;
                     rc = SDB_OK ;
                     goto done ;
                  }
                  else
                  {
                     // can't increase i counter
                     --i ;
                  }
               }

               //for some reason this slot can't be reused, let's get to the next
               preRW = delRecordRW ;
               foundDeletedID = pRead->getNextRID() ;
            }
         }
      }
      catch( std::exception &e )
      {
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         rc = pdGetLastError() ? pdGetLastError() : SDB_SYS ;
         goto error ;
      }

      // no space, need to allocate extent
      {
         UINT32 expandSize = requiredSize << DMS_RECORDS_PER_EXTENT_SQUARE ;
         if ( expandSize > DMS_BEST_UP_EXTENT_SZ )
         {
            expandSize = requiredSize < DMS_BEST_UP_EXTENT_SZ ?
                         DMS_BEST_UP_EXTENT_SZ : requiredSize ;
         }
         UINT32 reqPages = ( expandSize + DMS_EXTENT_METADATA_SZ +
                             pageSize() - 1 ) >> pageSizeSquareRoot() ;
         if ( reqPages > segmentPages() )
         {
            reqPages = segmentPages() ;
         }
         if ( reqPages < 1 )
         {
            reqPages = 1 ;
         }

         rc = _allocateExtent( context, reqPages, TRUE, FALSE, NULL ) ;
         PD_RC_CHECK( rc, PDERROR, "Unable to allocate %d pages extent to the "
                      "collection, rc: %d", reqPages, rc ) ;
         goto retry ;
      }

   done:
      PD_TRACE_EXITRC ( SDB__DMSSTORAGEDATA__RESERVEFROMDELETELIST, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSTORAGEDATA__TRUNCATECOLLECTION, "_dmsStorageData::_truncateCollection" )
   INT32 _dmsStorageData::_truncateCollection( dmsMBContext *context,
                                               BOOLEAN needChangeCLID )
   {
      INT32 rc                     = SDB_OK ;
      dmsExtRW lastRW ;
      dmsExtRW metaRW ;
      dmsExtRW dictRW ;
      dmsExtentID currentExt       = DMS_INVALID_EXTENT ;
      dmsExtentID prevExt          = DMS_INVALID_EXTENT ;
      dmsMetaExtent *metaExt       = NULL ;
      dmsDictExtent *dictExt       = NULL ;
      BOOLEAN reachLast            = FALSE ;
      dmsExtentID nextExt          = DMS_INVALID_EXTENT ;
      const dmsExtent *pCurrExt    = NULL ;

      SDB_ASSERT( context, "dms mb context can't be NULL" ) ;

      PD_TRACE_ENTRY ( SDB__DMSSTORAGEDATA__TRUNCATECOLLECTION ) ;
      rc = context->mbLock( EXCLUSIVE ) ;
      PD_RC_CHECK( rc, PDERROR, "dms mb context lock failed, rc: %d", rc ) ;

      currentExt = context->mb()->_lastExtentID ;
      // If there is only one extent, set reachLast as TRUE.
      reachLast = ( currentExt == context->mb()->_firstExtentID ) ?
                  TRUE : FALSE ;

      // reset delete list
      for ( UINT32 i = 0 ; i < dmsMB::_max ; i++ )
      {
         context->mb()->_deleteList[i].reset() ;
      }

      // Free all extent from the end. If the system went down becuase of power
      // cut, the file may be damanged. During the recovery, if we find any
      // damaged extent, try to truncate from the beginning also.
      while ( DMS_INVALID_EXTENT != currentExt )
      {
         try
         {
            lastRW = extent2RW( currentExt, context->mbID() ) ;
            pCurrExt = lastRW.readPtr<dmsExtent>() ;
         }
         catch ( std::exception &e )
         {
            PD_LOG( PDERROR, "Occur exception:%s", e.what() ) ;
            break ;
         }

         // get the previous extent
         prevExt = pCurrExt->_prevExtent ;
         // free the extent
         rc = _freeExtent ( currentExt, context->mbID() ) ;
         if ( rc )
         {
            SDB_ASSERT( SDB_OK == rc, "Free extent can't be failure" ) ;
            PD_LOG ( PDERROR, "Failed to free extent[%u], rc: %d", currentExt,
                     rc ) ;
            rc = SDB_DMS_CORRUPTED_EXTENT ;
            break ;
         }

         // set last to previous
         currentExt = prevExt ;
         // update MB
         context->mb()->_lastExtentID = currentExt ;
         if ( currentExt == context->mb()->_firstExtentID )
         {
            // Now ready to release the first(also the last for now) extent.
            reachLast = TRUE ;
         }
      }

      if ( !reachLast )
      {
         // Extent error found, now try to free from the beginning.
         currentExt = context->mb()->_firstExtentID ;
         while ( DMS_INVALID_EXTENT != currentExt )
         {
            try
            {
               lastRW = extent2RW( currentExt, context->mbID() ) ;
               pCurrExt = lastRW.readPtr<dmsExtent>() ;
            }
            catch ( std::exception &e )
            {
               PD_LOG( PDERROR, "Occur exception:%s", e.what() ) ;
               break ;
            }

            nextExt = pCurrExt->_nextExtent ;
            rc = _freeExtent( currentExt, context->mbID() ) ;
            if ( rc )
            {
               SDB_ASSERT( SDB_OK == rc, "Free extent can't be failure" ) ;
               PD_LOG ( PDERROR, "Failed to free extent[%u], rc: %d", currentExt,
                        rc ) ;
               rc = SDB_DMS_CORRUPTED_EXTENT ;
               break ;
            }

            currentExt = nextExt ;
            context->mb()->_firstExtentID = currentExt ;
            if ( currentExt == context->mb()->_lastExtentID )
            {
               break ;
            }
         }
      }

      context->mb()->_firstExtentID = DMS_INVALID_EXTENT ;
      context->mb()->_lastExtentID = DMS_INVALID_EXTENT ;

      // free all load extent
      currentExt = context->mb()->_loadLastExtentID ;
      while ( DMS_INVALID_EXTENT != currentExt )
      {
         lastRW = extent2RW( currentExt, context->mbID() ) ;
         pCurrExt = lastRW.readPtr<dmsExtent>() ;
         prevExt = pCurrExt->_prevExtent ;
         rc = _freeExtent( currentExt, context->mbID() ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Failed to free load extent[%u], rc: %d",
                    currentExt, rc ) ;
            SDB_ASSERT( SDB_OK == rc, "Free extent can't be failure" ) ;
         }
         currentExt = prevExt ;
         context->mb()->_loadLastExtentID = currentExt ;
      }
      context->mb()->_loadFirstExtentID = DMS_INVALID_EXTENT ;

      if ( DMS_INVALID_EXTENT != context->mb()->_mbExExtentID )
      {
         metaRW = extent2RW( context->mb()->_mbExExtentID, context->mbID() ) ;
         metaExt = metaRW.writePtr<dmsMetaExtent>() ;
         metaExt = metaRW.writePtr<dmsMetaExtent>( 0,
                                                  (UINT32)metaExt->_blockSize <<
                                                   pageSizeSquareRoot() ) ;
         metaExt->reset() ;
      }

      /*
       * Incase of drop/truncate collection, destroy the compressor and release
       * the dictionary both in memory and on disk.
       */
      if ( DMS_INVALID_EXTENT != context->mb()->_dictExtentID
           && needChangeCLID )
      {
         dictRW = extent2RW( context->mb()->_dictExtentID,
                             context->mbID() ) ;
         dictExt = dictRW.writePtr<dmsDictExtent>() ;
         _releaseSpace( context->mb()->_dictExtentID, dictExt->_blockSize ) ;
         dictExt->_flag = DMS_EXTENT_FLAG_FREED ;
         context->mb()->_dictExtentID = DMS_INVALID_EXTENT ;
         context->mb()->_dictVersion = 0 ;
      }

      context->mbStat()->_totalDataFreeSpace = 0 ;
      context->mbStat()->_totalDataPages = 0 ;
      context->mbStat()->_totalRecords = 0 ;
      context->mbStat()->_totalDataLen = 0 ;
      context->mbStat()->_totalOrgDataLen = 0 ;

   done :
      PD_TRACE_EXITRC ( SDB__DMSSTORAGEDATA__TRUNCATECOLLECTION, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSTORAGEDATA__TRUNCATECOLLECITONLOADS, "_dmsStorageData::_truncateCollectionLoads" )
   INT32 _dmsStorageData::_truncateCollectionLoads( dmsMBContext * context )
   {
      INT32 rc                     = SDB_OK ;
      dmsExtRW extRW ;
      dmsExtentID lastExt          = DMS_INVALID_EXTENT ;
      dmsExtentID prevExt          = DMS_INVALID_EXTENT ;

      SDB_ASSERT( context, "dms mb context can't be NULL" ) ;
      PD_TRACE_ENTRY ( SDB__DMSSTORAGEDATA__TRUNCATECOLLECITONLOADS ) ;
      rc = context->mbLock( EXCLUSIVE ) ;
      PD_RC_CHECK( rc, PDERROR, "dms mb context lock failed, rc: %d", rc ) ;

      // free all load extent
      lastExt = context->mb()->_loadLastExtentID ;
      while ( DMS_INVALID_EXTENT != lastExt )
      {
         rc = context->mbLock( EXCLUSIVE ) ;
         PD_RC_CHECK( rc, PDERROR, "dms mb context lock failed, rc: %d", rc ) ;

         extRW = extent2RW( lastExt, context->mbID() ) ;
         const dmsExtent *pLastExt = extRW.readPtr<dmsExtent>() ;
         prevExt = pLastExt->_prevExtent ;
         rc = _freeExtent( lastExt, context->mbID() ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Failed to free load extent[%u], rc: %d",
                    lastExt, rc ) ;
            SDB_ASSERT( SDB_OK == rc, "Free extent can't be failure" ) ;
         }
         lastExt = prevExt ;
         context->mb()->_loadLastExtentID = lastExt ;

         if ( DMS_INVALID_EXTENT != lastExt )
         {
            context->mbUnlock() ;
         }
      }
      context->mb()->_loadFirstExtentID = DMS_INVALID_EXTENT ;

   done :
      PD_TRACE_EXITRC ( SDB__DMSSTORAGEDATA__TRUNCATECOLLECITONLOADS, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSTORAGEDATA__SAVEDELETEDRECORD, "_dmsStorageData::_saveDeletedRecord" )
   INT32 _dmsStorageData::_saveDeletedRecord( dmsMB *mb,
                                              const dmsRecordID &rid,
                                              INT32 recordSize,
                                              dmsExtent *extAddr,
                                              dmsDeletedRecord *pRecord )
   {
      UINT8 deleteRecordSlot = 0 ;

      SDB_ASSERT( extAddr && pRecord, "NULL Pointer" ) ;

      PD_TRACE_ENTRY ( SDB__DMSSTORAGEDATA__SAVEDELETEDRECORD ) ;
      PD_TRACE6 ( SDB__DMSSTORAGEDATA__SAVEDELETEDRECORD,
                  PD_PACK_STRING ( "offset" ),
                  PD_PACK_INT ( rid._offset ),
                  PD_PACK_STRING ( "recordSize" ),
                  PD_PACK_INT ( recordSize ),
                  PD_PACK_STRING ( "extentID" ),
                  PD_PACK_INT ( rid._extent ) ) ;

      // assign flags to the memory
      pRecord->setDeleted() ;
      if ( recordSize > 0 )
      {
         pRecord->setSize( recordSize ) ;
      }
      else
      {
         recordSize = pRecord->getSize() ;
      }
      pRecord->setMyOffset( rid._offset ) ;

      // change free space
      extAddr->_freeSpace += recordSize ;
      _mbStatInfo[mb->_blockID]._totalDataFreeSpace += recordSize ;

      // let's count which delete slots it fits
      // divide by 32 first since our first slot is for <32 bytes
      recordSize = ( recordSize - 1 ) >> 5 ;
      // while loop, divde by 2 everytime, find the closest delete slot
      // for example, for a given size 3000, we should go _4k (which is
      // _deleteList[7], using 3000>>5=93
      // then in a loop, first round we have 46, type=1
      // then 23, type=2
      // then 11, type=3
      // then 5, type=4
      // then 2, type=5
      // then 1, type=6
      // finally 0, type=7

      while ( (recordSize) != 0 )
      {
         deleteRecordSlot ++ ;
         recordSize = ( recordSize >> 1 ) ;
      }

      // make sure we don't mis calculated it
      SDB_ASSERT ( deleteRecordSlot < dmsMB::_max, "Invalid record size" ) ;

      // set the first matching delete slot to the
      // next rid for the deleted record
      pRecord->setNextRID( mb->_deleteList [ deleteRecordSlot ] ) ;
      // Then assign MB delete slot to the extent and offset
      mb->_deleteList[ deleteRecordSlot ] = rid ;
      PD_TRACE_EXIT ( SDB__DMSSTORAGEDATA__SAVEDELETEDRECORD ) ;
      return SDB_OK ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSTORAGEDATA__SAVEDELETEDRECORD1, "_dmsStorageData::_saveDeletedRecord" )
   INT32 _dmsStorageData::_saveDeletedRecord( dmsMB * mb,
                                              const dmsRecordID &recordID,
                                              INT32 recordSize )
   {
      INT32 rc = SDB_OK ;
      dmsExtRW rw ;
      dmsRecordRW recordRW ;

      PD_TRACE_ENTRY ( SDB__DMSSTORAGEDATA__SAVEDELETEDRECORD1 ) ;
      PD_TRACE2 ( SDB__DMSSTORAGEDATA__SAVEDELETEDRECORD1,
                  PD_PACK_INT ( recordID._extent ),
                  PD_PACK_INT ( recordID._offset ) ) ;
      if ( recordID.isNull() )
      {
         rc = SDB_INVALIDARG ;
         goto done ;
      }

      rw = extent2RW( recordID._extent, mb->_blockID ) ;
      recordRW = record2RW( recordID, mb->_blockID ) ;

      try
      {
         dmsExtent *pExtent = rw.writePtr<dmsExtent>() ;
         dmsDeletedRecord* pRecord = recordRW.writePtr<dmsDeletedRecord>() ;
         rc = _saveDeletedRecord ( mb, recordID, recordSize,
                                   pExtent, pRecord ) ;
      }
      catch( std::exception &e )
      {
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         rc = pdGetLastError() ? pdGetLastError() : SDB_SYS ;
         goto error ;
      }

   done :
      PD_TRACE_EXITRC ( SDB__DMSSTORAGEDATA__SAVEDELETEDRECORD1, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSTORAGEDATA__MAPEXTENT2DELLIST, "_dmsStorageData::_mapExtent2DelList" )
   void _dmsStorageData::_mapExtent2DelList( dmsMB *mb, dmsExtent *extAddr,
                                             SINT32 extentID )
   {
      INT32 extentSize         = 0 ;
      INT32 extentUseableSpace = 0 ;
      INT32 deleteRecordSize   = 0 ;
      dmsOffset recordOffset   = 0 ;
      INT32 curUseableSpace    = 0 ;

      PD_TRACE_ENTRY ( SDB__DMSSTORAGEDATA__MAPEXTENT2DELLIST ) ;

      if ( (UINT32)extAddr->_freeSpace < DMS_MIN_RECORD_SZ )
      {
         if ( extAddr->_freeSpace != 0 )
         {
            PD_LOG( PDINFO, "Collection[%s, mbID: %d]'s extent[%d] free "
                    "space[%d] is less than min record size[%d]",
                    mb->_collectionName, mb->_blockID, extentID,
                    extAddr->_freeSpace, DMS_MIN_RECORD_SZ ) ;
         }
         goto done ;
      }

      // calculate the delete record size we need to use
      extentSize          = extAddr->_blockSize << pageSizeSquareRoot() ;
      extentUseableSpace  = extAddr->_freeSpace ;
      extAddr->_freeSpace = 0 ;

      // make sure the delete record is not greater 16MB
      deleteRecordSize    = OSS_MIN ( extentUseableSpace,
                                      DMS_RECORD_MAX_SZ ) ;
      // place first record offset
      recordOffset        = extentSize - extentUseableSpace ;
      curUseableSpace     = extentUseableSpace ;

      /// extentUseableSpace > 16MB
      while ( curUseableSpace - deleteRecordSize >=
              (INT32)DMS_MIN_DELETEDRECORD_SZ )
      {
         dmsRecordID rid( extentID, recordOffset ) ;
         dmsRecordRW rRW = record2RW( rid, mb->_blockID ) ;
         _saveDeletedRecord( mb, rid, deleteRecordSize,
                             extAddr, rRW.writePtr<dmsDeletedRecord>() ) ;
         curUseableSpace -= deleteRecordSize ;
         recordOffset += deleteRecordSize ;
      }
      /// 16MB < curUseableSpace < 16MB + DMS_MIN_DELETEDRECORD_SZ
      if ( curUseableSpace > deleteRecordSize )
      {
         dmsRecordID rid( extentID, recordOffset ) ;
         dmsRecordRW rRW = record2RW( rid, mb->_blockID ) ;
         _saveDeletedRecord( mb, rid, DMS_PAGE_SIZE4K,
                             extAddr, rRW.writePtr<dmsDeletedRecord>() ) ;
         curUseableSpace -= DMS_PAGE_SIZE4K ;
         recordOffset += DMS_PAGE_SIZE4K ;
      }
      /// 0 < curUseableSpace < 16MB
      if ( curUseableSpace > 0 )
      {
         dmsRecordID rid( extentID, recordOffset ) ;
         dmsRecordRW rRW = record2RW( rid, mb->_blockID ) ;
         _saveDeletedRecord( mb, rid, curUseableSpace,
                             extAddr, rRW.writePtr<dmsDeletedRecord>() ) ;
      }

      // correct check
      SDB_ASSERT( extentUseableSpace == extAddr->_freeSpace,
                  "Extent[%d] free space invalid" ) ;
   done :
      PD_TRACE_EXIT ( SDB__DMSSTORAGEDATA__MAPEXTENT2DELLIST ) ;
      return ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSTORAGEDATA_ADDEXTENT2META, "_dmsStorageData::addExtent2Meta" )
   INT32 _dmsStorageData::addExtent2Meta( dmsExtentID extID,
                                          dmsExtent *extent,
                                          dmsMBContext *context )
   {
      INT32 rc = SDB_OK ;
      dmsExtRW mbExRW ;
      dmsMBEx *mbEx = NULL ;
      PD_TRACE_ENTRY ( SDB__DMSSTORAGEDATA_ADDEXTENT2META ) ;
      UINT32 segID = extent2Segment( extID ) - dataStartSegID() ;
      dmsExtentID lastExtID = DMS_INVALID_EXTENT ;
      dmsExtRW prevRW ;
      dmsExtRW nextRW ;
      dmsExtent *prevExt = NULL ;
      dmsExtent *nextExt = NULL ;

      if ( !context->isMBLock( EXCLUSIVE ) )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Caller must hold mb exclusive lock[%s]",
                 context->toString().c_str() ) ;
         goto error ;
      }

      // SYSTEMP SU
      if ( isTempSU() )
      {
         extent->_prevExtent = context->mb()->_lastExtentID ;

         // if this is the first extent in this MB, we assign firstExtentID to
         // it
         if ( DMS_INVALID_EXTENT == context->mb()->_firstExtentID )
         {
            context->mb()->_firstExtentID = extID ;
         }

         // if there's previous record, we reassign the previous
         // extent->nextExtent to this new extent
         if ( DMS_INVALID_EXTENT != extent->_prevExtent )
         {
            prevRW = extent2RW( extent->_prevExtent, context->mbID() ) ;
            dmsExtent *prevExt = prevRW.writePtr<dmsExtent>() ;
            prevExt->_nextExtent = extID ;
            extent->_logicID = prevExt->_logicID + 1 ;
         }
         else
         {
            extent->_logicID = DMS_INVALID_EXTENT + 1 ;
         }

         // MB's last extent always assigned to the new extent
         context->mb()->_lastExtentID = extID ;
      }
      else
      {
         mbExRW = extent2RW( context->mb()->_mbExExtentID,
                             context->mbID() ) ;
         mbExRW.setNothrow( TRUE ) ;
         mbEx = mbExRW.writePtr<dmsMBEx>() ;
         if ( NULL == mbEx )
         {
            rc = SDB_SYS ;
            PD_LOG( PDERROR, "dms mb expand extent is invalid: %d",
                    context->mb()->_mbExExtentID ) ;
            goto error ;
         }

         if ( segID >= mbEx->_header._segNum )
         {
            rc = SDB_SYS ;
            PD_LOG( PDERROR, "Invalid segID: %d, max segNum: %d", segID,
                    mbEx->_header._segNum ) ;
            goto error ;
         }

         /// re-write ptr
         mbEx = mbExRW.writePtr<dmsMBEx>( 0,
                                          (UINT32)mbEx->_header._blockSize <<
                                          pageSizeSquareRoot() ) ;
         mbEx->getLastExtentID( segID, lastExtID ) ;

         if ( DMS_INVALID_EXTENT == lastExtID )
         {
            extent->_logicID = ( segID << _getFactor() ) ;
            mbEx->setFirstExtentID( segID, extID ) ;
            mbEx->setLastExtentID( segID, extID ) ;
            ++(mbEx->_header._usedSegNum) ;

            // find prevExt
            INT32 tmpSegID = segID ;
            dmsExtentID tmpExtID = DMS_INVALID_EXTENT ;
            while ( DMS_INVALID_EXTENT != context->mb()->_firstExtentID &&
                    --tmpSegID >= 0 )
            {
               mbEx->getLastExtentID( tmpSegID, tmpExtID ) ;
               if ( DMS_INVALID_EXTENT != tmpExtID )
               {
                  extent->_prevExtent = tmpExtID ;
                  prevRW = extent2RW( tmpExtID, context->mbID() ) ;
                  prevExt = prevRW.writePtr<dmsExtent>() ;
                  break ;
               }
            }
         }
         else
         {
            mbEx->setLastExtentID( segID, extID ) ;
            extent->_prevExtent = lastExtID ;
            prevRW = extent2RW( lastExtID, context->mbID() ) ;
            prevExt = prevRW.writePtr<dmsExtent>() ;
            extent->_logicID = prevExt->_logicID + 1 ;
         }

         if ( prevExt )
         {
            if ( DMS_INVALID_EXTENT != prevExt->_nextExtent )
            {
               extent->_nextExtent = prevExt->_nextExtent ;
               nextRW = extent2RW( extent->_nextExtent, context->mbID() ) ;
               nextExt = nextRW.writePtr<dmsExtent>() ;
               nextExt->_prevExtent = extID ;
            }
            else
            {
               context->mb()->_lastExtentID = extID ;
            }
            prevExt->_nextExtent = extID ;
         }
         else
         {
            if ( DMS_INVALID_EXTENT != context->mb()->_firstExtentID )
            {
               extent->_nextExtent = context->mb()->_firstExtentID ;
               nextRW = extent2RW( extent->_nextExtent, context->mbID() ) ;
               nextExt = nextRW.writePtr<dmsExtent>() ;
               nextExt->_prevExtent = extID ;
            }
            context->mb()->_firstExtentID = extID ;

            if ( DMS_INVALID_EXTENT == context->mb()->_lastExtentID )
            {
               context->mb()->_lastExtentID = extID ;
            }
         }
      }

      context->mbStat()->_totalDataPages += extent->_blockSize ;

   done:
      PD_TRACE_EXITRC ( SDB__DMSSTORAGEDATA_ADDEXTENT2META, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSTORAGEDATA_ADDCOLLECTION, "_dmsStorageData::addCollection" )
   INT32 _dmsStorageData::addCollection( const CHAR * pName,
                                         UINT16 * collectionID,
                                         UINT32 attributes,
                                         pmdEDUCB * cb,
                                         SDB_DPSCB * dpscb,
                                         UINT16 initPages,
                                         BOOLEAN sysCollection,
                                         UINT8  compressionType,
                                         UINT32 *logicID )
   {
      INT32 rc                = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__DMSSTORAGEDATA_ADDCOLLECTION ) ;
      dpsMergeInfo info ;
      dpsLogRecord &record    = info.getMergeBlock().record() ;
      UINT32 logRecSize       = 0;
      dpsTransCB *pTransCB    = pmdGetKRCB()->getTransCB() ;
      CHAR fullName[DMS_COLLECTION_FULL_NAME_SZ + 1] = {0} ;
      UINT16 newCollectionID  = DMS_INVALID_MBID ;
      UINT32 logicalID        = DMS_INVALID_CLID ;
      BOOLEAN metalocked      = FALSE ;
      dmsMB *mb               = NULL ;
      SDB_DPSCB *dropDps      = NULL ;
      dmsMBContext *context   = NULL ;

      UINT32 segNum           = DMS_MAX_PG >> segmentPagesSquareRoot() ;
      UINT32 mbExSize         = (( segNum << 3 ) >> pageSizeSquareRoot()) + 1 ;
      dmsExtentID mbExExtent  = DMS_INVALID_EXTENT ;
      dmsMetaExtent *mbExtent = NULL ;

      SDB_ASSERT( pName, "Collection name cat't be NULL" ) ;

      rc = dmsCheckCLName ( pName, sysCollection ) ;
      PD_RC_CHECK( rc, PDERROR, "Invalid collection name %s, rc: %d",
                   pName, rc ) ;

      // calc the reserve dps size
      if ( dpscb )
      {
         rc = dpsCLCrt2Record( _clFullName(pName, fullName, sizeof(fullName)),
                               attributes, compressionType, record ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to build record, rc: %d", rc ) ;

         rc = dpscb->checkSyncControl( record.alignedLen(), cb ) ;
         PD_RC_CHECK( rc, PDERROR, "Check sync control failed, rc: %d", rc ) ;

         logRecSize = record.alignedLen() ;
         rc = pTransCB->reservedLogSpace( logRecSize, cb ) ;
         if( rc )
         {
            PD_LOG( PDERROR, "Failed to reserved log space(length=%u)",
                    logRecSize ) ;
            logRecSize = 0 ;
            goto error ;
         }
      }

      if ( !isTempSU () )
      {
         // allocate mbEx extent
         rc = _findFreeSpace( mbExSize, mbExExtent, NULL ) ;
         PD_RC_CHECK( rc, PDERROR, "Allocate metablock expand extent failed, "
                      "pageNum: %d, rc: %d", mbExSize, rc ) ;
      }

      // first exclusive latch metadata, this shouldn't be replaced by SHARED to
      // prevent racing with dropCollection
      ossLatch( &_metadataLatch, EXCLUSIVE ) ;
      metalocked = TRUE ;

      // then let's make sure the collection name does not exist
      if ( DMS_INVALID_MBID != _collectionNameLookup ( pName ) )
      {
         rc = SDB_DMS_EXIST ;
         goto error ;
      }

      if ( DMS_MME_SLOTS <= _dmsHeader->_numMB )
      {
         PD_LOG ( PDERROR, "There is no free slot for extra collection" ) ;
         rc = SDB_DMS_NOSPC ;
         goto error ;
      }

      // find a slot
      for ( UINT32 i = 0 ; i < DMS_MME_SLOTS ; ++i )
      {
         if ( DMS_IS_MB_FREE ( _dmsMME->_mbList[i]._flag ) )
         {
            newCollectionID = i ;
            break ;
         }
      }
      // make sure we find free collection id
      if ( DMS_INVALID_MBID == newCollectionID )
      {
         PD_LOG ( PDERROR, "Unable to find free collection id" ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      // set mb meta data and header data
      logicalID = _dmsHeader->_MBHWM++ ;
      mb = &_dmsMME->_mbList[newCollectionID] ;
      mb->reset( pName, newCollectionID, logicalID,
                 attributes, compressionType ) ;
      _mbStatInfo[ newCollectionID ].reset() ;
      _compressorEntry[ newCollectionID ].reset() ;

      _dmsHeader->_numMB++ ;
      _collectionNameInsert( pName, newCollectionID ) ;

      if ( !isTempSU () )
      {
         dmsExtRW rw = extent2RW( mbExExtent, newCollectionID ) ;
         rw.setNothrow( TRUE ) ;
         mbExtent = rw.writePtr<dmsMetaExtent>( 0,
                                                mbExSize <<
                                                pageSizeSquareRoot() ) ;
         PD_CHECK( mbExtent, SDB_SYS, error, PDERROR,
                   "Invalid meta extent[%d]", mbExExtent ) ;
         mbExtent->init( mbExSize, newCollectionID, segNum ) ;
         mb->_mbExExtentID = mbExExtent ;
         mbExExtent = DMS_INVALID_EXTENT ;
      }

      // write dps log
      if ( dpscb )
      {
         rc = _logDPS( dpscb, info, cb, &_metadataLatch, EXCLUSIVE,
                       metalocked, logicalID, DMS_INVALID_EXTENT ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to insert CLcrt record to log, "
                      "rc = %d", rc ) ;
      }

      // release meta lock
      if ( metalocked )
      {
         ossUnlatch( &_metadataLatch, EXCLUSIVE ) ;
         metalocked = FALSE ;
      }

      if ( collectionID )
      {
         *collectionID = newCollectionID ;
      }
      dropDps = dpscb ;

      // create dms cb context
      rc = getMBContext( &context, newCollectionID, logicalID, EXCLUSIVE ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Failed to get mb[%u] context, rc: %d",
                 newCollectionID, rc ) ;
         if ( SDB_DMS_NOTEXIST == rc )
         {
            newCollectionID = DMS_INVALID_MBID ;
         }
         goto error ;
      }

      /// set compressor when snappy
      _setCompressor( context ) ;

      // allocate new extent
      if ( 0 != initPages )
      {
         rc = _allocateExtent( context, initPages, TRUE, FALSE, NULL ) ;
         PD_RC_CHECK( rc, PDERROR, "Allocate new %u pages of collection[%s] "
                      "failed, rc: %d", initPages, pName, rc ) ;
      }

      // create $id index[s_idKeyObj]
      if ( !OSS_BIT_TEST( attributes, DMS_MB_ATTR_NOIDINDEX ) )
      {
         rc = _pIdxSU->createIndex( context, s_idKeyObj, cb, NULL, TRUE ) ;
         PD_RC_CHECK( rc, PDERROR, "Create $id index failed in collection[%s], "
                      "rc: %d", pName, rc ) ;
      }

      if ( logicID )
      {
         *logicID = logicalID ;
      }

      if ( cb && cb->getLsnCount() > 0 )
      {
         context->mbStat()->updateLastLSNWithComp( cb->getEndLsn(),
                                                   DMS_FILE_DATA,
                                                   cb->isDoRollback() ) ;
      }

   done:
      if ( context )
      {
         releaseMBContext( context ) ;
      }
      if ( 0 != logRecSize )
      {
         pTransCB->releaseLogSpace( logRecSize, cb );
      }
      if ( SDB_OK == rc )
      {
         flushMeta( isSyncDeep() ) ;
      }
      PD_TRACE_EXITRC ( SDB__DMSSTORAGEDATA_ADDCOLLECTION, rc ) ;
      return rc ;
   error:
      if ( metalocked )
      {
         ossUnlatch( &_metadataLatch, EXCLUSIVE ) ;
      }
      if ( DMS_INVALID_EXTENT != mbExExtent )
      {
         _releaseSpace( mbExExtent, mbExSize ) ;
      }
      if ( DMS_INVALID_MBID != newCollectionID )
      {
         // drop collection
         INT32 rc1 = dropCollection( pName, cb, dropDps, sysCollection,
                                     context ) ;
         if ( rc1 )
         {
            PD_LOG( PDSEVERE, "Failed to clean up bad collection creation[%s], "
                    "rc: %d", pName, rc ) ;
         }
      }
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSTORAGEDATA_DROPCOLLECTION, "_dmsStorageData::dropCollection" )
   INT32 _dmsStorageData::dropCollection( const CHAR * pName, pmdEDUCB * cb,
                                          SDB_DPSCB * dpscb,
                                          BOOLEAN sysCollection,
                                          dmsMBContext * context )
   {
      INT32 rc                = 0 ;
      CHAR fullName[DMS_COLLECTION_FULL_NAME_SZ + 1] = {0} ;

      PD_TRACE_ENTRY ( SDB__DMSSTORAGEDATA_DROPCOLLECTION ) ;
      dpsMergeInfo info ;
      dpsLogRecord &record    = info.getMergeBlock().record() ;
      UINT32 logRecSize       = 0;
      dpsTransCB *pTransCB    = pmdGetKRCB()->getTransCB() ;
      BOOLEAN isTransLocked   = FALSE ;
      BOOLEAN getContext      = FALSE ;
      BOOLEAN metalocked      = FALSE ;

      SDB_ASSERT( pName, "Collection name cat't be NULL" ) ;

      rc = dmsCheckCLName ( pName, sysCollection ) ;
      PD_RC_CHECK( rc, PDERROR, "Invalid collection name %s, rc: %d",
                   pName, rc ) ;

      // calc the reserve dps size
      if ( dpscb )
      {
         rc = dpsCLDel2Record( _clFullName(pName, fullName, sizeof(fullName)),
                               record ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to build record, rc: %d", rc ) ;

         rc = dpscb->checkSyncControl( record.alignedLen(), cb ) ;
         PD_RC_CHECK( rc, PDERROR, "Check sync control failed, rc: %d", rc ) ;

         logRecSize = record.alignedLen() ;
         rc = pTransCB->reservedLogSpace( logRecSize, cb ) ;
         if( rc )
         {
            PD_LOG( PDERROR, "Failed to reserved log space(length=%u)",
                    logRecSize ) ;
            logRecSize = 0 ;
            goto error ;
         }
      }

      // lock collection mb exclusive lock
      if ( NULL == context )
      {
         rc = getMBContext( &context, pName, EXCLUSIVE ) ;
         PD_RC_CHECK( rc, PDWARNING, "Failed to get mb[%s] context, rc: %d",
                      pName, rc ) ;
         getContext = TRUE ;
      }
      else
      {
         rc = context->mbLock( EXCLUSIVE ) ;
         PD_RC_CHECK( rc, PDWARNING, "Collection[%s] mblock failed, rc: %d",
                      pName, rc ) ;
      }

      if ( !dmsAccessAndFlagCompatiblity ( context->mb()->_flag,
                                           DMS_ACCESS_TYPE_TRUNCATE ) )
      {
         PD_LOG ( PDERROR, "Incompatible collection mode: %d",
                  context->mb()->_flag ) ;
         rc = SDB_DMS_INCOMPATIBLE_MODE ;
         goto error ;
      }

      // it is not need to lock that drop temp collection while startup
      // which cb is NULL
      if ( cb && dpscb )
      {
         rc = pTransCB->transLockTryX( cb, _logicalCSID, context->mbID() ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to lock the collection, rc: %d",
                      rc ) ;
         isTransLocked = TRUE ;
      }

      // drop all index
      rc = _pIdxSU->dropAllIndexes( context, cb, NULL ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to drop index for collection[%s], "
                   "rc: %d", pName, rc ) ;

      // truncate the collection
      _rmCompressor( context ) ;

      rc = _truncateCollection( context ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to truncate the collection[%s], rc: %d",
                   pName, rc ) ;

      // truncate lob
      if ( _pLobSU->isOpened() )
      {
         rc = _pLobSU->truncate( context, cb, NULL ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to truncate the collection[%s] lob,"
                      "rc: %d", pName, rc ) ;
      }

      // change mb meta data
      DMS_SET_MB_DROPPED( context->mb()->_flag ) ;
      context->mb()->_logicalID-- ;

      if ( DMS_INVALID_EXTENT != context->mb()->_mbExExtentID )
      {
         dmsExtRW rw = extent2RW( context->mb()->_mbExExtentID,
                                  context->mbID() ) ;
         rw.setNothrow( TRUE ) ;
         const dmsMetaExtent *metaExt = rw.readPtr<dmsMetaExtent>() ;
         if ( metaExt )
         {
            _releaseSpace( context->mb()->_mbExExtentID, metaExt->_blockSize ) ;
         }
         context->mb()->_mbExExtentID = DMS_INVALID_EXTENT ;
      }

      // release mb lock
      context->mbUnlock() ;

      // change metadata
      ossLatch( &_metadataLatch, EXCLUSIVE ) ;
      metalocked = TRUE ;
      _collectionNameRemove( pName ) ;
      DMS_SET_MB_FREE( context->mb()->_flag ) ;
      _dmsHeader->_numMB-- ;

      // write dps log
      if ( dpscb )
      {
         rc = _logDPS( dpscb, info, cb, &_metadataLatch, EXCLUSIVE, metalocked,
                       context->clLID(), DMS_INVALID_EXTENT ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to insert CLDel record to log, rc: "
                      "%d", rc ) ;
      }
      PD_LOG( PDEVENT, "Drop collection[%s] succeed", fullName ) ;

   done:
      if ( metalocked )
      {
         ossUnlatch( &_metadataLatch, EXCLUSIVE ) ;
         metalocked = FALSE ;
      }
      if ( isTransLocked )
      {
         pTransCB->transLockRelease( cb, _logicalCSID, context->mbID() );
      }
      if ( context && getContext )
      {
         releaseMBContext( context ) ;
      }
      if ( 0 != logRecSize )
      {
         pTransCB->releaseLogSpace( logRecSize, cb );
      }
      if ( SDB_OK == rc )
      {
         flushMeta( isSyncDeep() ) ;
      }
      PD_TRACE_EXITRC ( SDB__DMSSTORAGEDATA_DROPCOLLECTION, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSTORAGEDATA_TRUNCATECOLLECTION, "_dmsStorageData::truncateCollection" )
   INT32 _dmsStorageData::truncateCollection( const CHAR *pName,
                                              pmdEDUCB *cb,
                                              SDB_DPSCB *dpscb,
                                              BOOLEAN sysCollection,
                                              dmsMBContext *context,
                                              BOOLEAN needChangeCLID,
                                              BOOLEAN truncateLob )
   {
      INT32 rc           = SDB_OK ;
      BOOLEAN getContext = FALSE ;
      UINT32 newCLID     = DMS_INVALID_CLID ;
      UINT32 oldCLID     = DMS_INVALID_CLID ;
      UINT64 oldRecords  = 0 ;
      UINT64 oldLobs     = 0 ;

      PD_TRACE_ENTRY ( SDB__DMSSTORAGEDATA_TRUNCATECOLLECTION ) ;
      CHAR fullName[DMS_COLLECTION_FULL_NAME_SZ + 1] = {0} ;
      dpsMergeInfo info ;
      dpsLogRecord &record    = info.getMergeBlock().record() ;
      UINT32 logRecSize       = 0;
      dpsTransCB *pTransCB    = pmdGetKRCB()->getTransCB() ;
      BOOLEAN isTransLocked   = FALSE ;

      SDB_ASSERT( pName, "Collection name cat't be NULL" ) ;

      rc = dmsCheckCLName ( pName, sysCollection ) ;
      PD_RC_CHECK( rc, PDERROR, "Invalid collection name %s, rc: %d",
                   pName, rc ) ;

      // calc the reserve dps size
      if ( dpscb )
      {
         rc = dpsCLTrunc2Record( _clFullName(pName, fullName, sizeof(fullName)),
                                 record ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to build record, rc: %d", rc ) ;

         rc = dpscb->checkSyncControl( record.alignedLen(), cb ) ;
         PD_RC_CHECK( rc, PDERROR, "Check sync control failed, rc: %d", rc ) ;

         logRecSize = record.alignedLen() ;
         rc = pTransCB->reservedLogSpace( logRecSize, cb ) ;
         if( rc )
         {
            PD_LOG( PDERROR, "Failed to reserved log space(length=%u)",
                    logRecSize ) ;
            logRecSize = 0 ;
            goto error ;
         }
      }

      // lock collection mb exclusive lock
      if ( NULL == context )
      {
         rc = getMBContext( &context, pName, EXCLUSIVE ) ;
         PD_RC_CHECK( rc, PDWARNING, "Failed to get mb[%s] context, rc: %d",
                      pName, rc ) ;
         getContext = TRUE ;
      }
      else
      {
         rc = context->mbLock( EXCLUSIVE ) ;
         PD_RC_CHECK( rc, PDWARNING, "Collection[%s] mblock failed, rc: %d",
                      pName, rc ) ;
      }

      if ( !dmsAccessAndFlagCompatiblity ( context->mb()->_flag,
                                           DMS_ACCESS_TYPE_TRUNCATE ) )
      {
         PD_LOG ( PDERROR, "Incompatible collection mode: %d",
                  context->mb()->_flag ) ;
         rc = SDB_DMS_INCOMPATIBLE_MODE ;
         goto error ;
      }

      // trans lock
      if ( cb && dpscb )
      {
         rc = pTransCB->transLockTryX( cb, _logicalCSID, context->mbID() ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to lock the collection, rc: %d",
                      rc ) ;
         isTransLocked = TRUE ;
      }

      // pause mb lock and change metadata
      context->pause() ;
      ossLatch( &_metadataLatch, EXCLUSIVE ) ;
      if ( needChangeCLID )
      {
         newCLID = _dmsHeader->_MBHWM++ ;
      }
      ossUnlatch( &_metadataLatch, EXCLUSIVE ) ;

      // resume context lock
      rc = context->resume() ;
      PD_RC_CHECK( rc, PDERROR, "dms mb context resume falied, rc: %d", rc ) ;

      oldRecords = context->mbStat()->_totalRecords ;
      oldLobs = context->mbStat()->_totalLobs ;

      rc = _pIdxSU->truncateIndexes( context ) ;
      PD_RC_CHECK( rc, PDERROR, "Truncate collection[%s] indexes failed, "
                   "rc: %d", pName, rc ) ;

      /*
       * For LZW, the compressor and dictionary should be removed during
       * truncate. In case of snappy, the compressor should be reserved.
       */
      if ( UTIL_COMPRESSOR_LZW ==
           (UTIL_COMPRESSOR_TYPE)(context->mb()->_compressorType) &&
           needChangeCLID )
      {
         _rmCompressor( context ) ;
      }
      rc = _truncateCollection( context, needChangeCLID ) ;
      PD_RC_CHECK( rc, PDERROR, "Truncate collection[%s] data failed, rc: %d",
                   pName, rc ) ;

      if ( truncateLob && _pLobSU->isOpened() )
      {
         rc = _pLobSU->truncate( context, cb, NULL ) ;
         PD_RC_CHECK( rc, PDERROR, "Truncate collection[%s] lob failed, rc: %d",
                      pName, rc ) ;
      }

      // change mb metadata
      if ( needChangeCLID )
      {
         oldCLID = context->_clLID ;
         context->mb()->_logicalID = newCLID ;
         context->_clLID           = newCLID ;
      }

      // write dps log
      if ( dpscb )
      {
         PD_AUDIT_OP_WITHNAME( AUDIT_DML, "TRUNCATE", AUDIT_OBJ_CL,
                               fullName, rc, "RecordNum:%llu, LobNum:%llu",
                               oldRecords, oldLobs ) ;
         rc = _logDPS( dpscb, info, cb, context, DMS_INVALID_EXTENT,
                       TRUE, DMS_FILE_DATA, &oldCLID ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to insert CLTrunc record to log, "
                      "rc: %d", rc ) ;
      }
      else if ( cb->getLsnCount() > 0 )
      {
         context->mbStat()->updateLastLSN( cb->getEndLsn(), DMS_FILE_DATA ) ;
      }

   done:
      if ( isTransLocked )
      {
         pTransCB->transLockRelease( cb, _logicalCSID, context->mbID() ) ;
      }
      if ( context && getContext )
      {
         releaseMBContext( context ) ;
      }
      if ( 0 != logRecSize )
      {
         pTransCB->releaseLogSpace( logRecSize, cb ) ;
      }
      if ( SDB_OK == rc )
      {
         /// flush meta
         flushMeta( isSyncDeep() ) ;
      }
      PD_TRACE_EXITRC ( SDB__DMSSTORAGEDATA_TRUNCATECOLLECTION, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSTORAGEDATA_TRUNCATECOLLECTIONLOADS, "_dmsStorageData::truncateCollectionLoads" )
   INT32 _dmsStorageData::truncateCollectionLoads( const CHAR *pName,
                                                   dmsMBContext * context )
   {
      INT32 rc           = SDB_OK ;
      BOOLEAN getContext = FALSE ;

      PD_TRACE_ENTRY ( SDB__DMSSTORAGEDATA_TRUNCATECOLLECTIONLOADS ) ;
      if ( NULL == context )
      {
         SDB_ASSERT( pName, "Collection name cat't be NULL" ) ;

         rc = getMBContext( &context, pName, -1 ) ;
         PD_RC_CHECK( rc, PDWARNING, "Failed to get mb[%s] context, rc: %d",
                      pName, rc ) ;
         getContext = TRUE ;
      }

      rc = _truncateCollectionLoads( context ) ;
      PD_RC_CHECK( rc, PDERROR, "Truncate collection[%s] loads failed, rc: %d",
                   pName, rc ) ;

   done:
      if ( context && getContext )
      {
         releaseMBContext( context ) ;
      }
      if ( SDB_OK == rc )
      {
         /// flush mme
         flushMME( isSyncDeep() ) ;
      }
      PD_TRACE_EXITRC ( SDB__DMSSTORAGEDATA_TRUNCATECOLLECTIONLOADS, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSTORAGEDATA_RENAMECOLLECTION, "_dmsStorageData::renameCollecion" )
   INT32 _dmsStorageData::renameCollection( const CHAR * oldName,
                                            const CHAR * newName,
                                            pmdEDUCB * cb,
                                            SDB_DPSCB * dpscb,
                                            BOOLEAN sysCollection )
   {
      INT32 rc                = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__DMSSTORAGEDATA_RENAMECOLLECTION ) ;
      dpsTransCB *pTransCB    = pmdGetKRCB()->getTransCB() ;
      UINT32 logRecSize       = 0 ;
      dpsMergeInfo info ;
      dpsLogRecord &record    = info.getMergeBlock().record() ;
      BOOLEAN metalocked      = FALSE ;
      UINT16  mbID            = DMS_INVALID_MBID ;
      UINT32  clLID           = DMS_INVALID_CLID ;

      PD_TRACE2 ( SDB__DMSSTORAGEDATA_RENAMECOLLECTION,
                  PD_PACK_STRING ( oldName ),
                  PD_PACK_STRING ( newName ) ) ;
      rc = dmsCheckCLName ( oldName, sysCollection ) ;
      PD_RC_CHECK( rc, PDERROR, "Invalid old collection name %s, rc: %d",
                   oldName, rc ) ;
      rc = dmsCheckCLName ( newName, sysCollection ) ;
      PD_RC_CHECK( rc, PDERROR, "Invalid new collection name %s, rc: %d",
                   newName, rc ) ;

      // reserved log-size
      if ( dpscb )
      {
         rc = dpsCLRename2Record( getSuName(), oldName, newName, record ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to build log record, rc: %d", rc ) ;

         rc = dpscb->checkSyncControl( record.alignedLen(), cb ) ;
         PD_RC_CHECK( rc, PDERROR, "Check sync control failed, rc: %d", rc ) ;

         logRecSize = record.alignedLen() ;
         rc = pTransCB->reservedLogSpace( logRecSize, cb );
         if( rc )
         {
            PD_LOG( PDERROR, "Failed to reserved log space(length=%u)",
                    logRecSize ) ;
            logRecSize = 0 ;
            goto error ;
         }
      }

      // lock metadata
      ossLatch ( &_metadataLatch, EXCLUSIVE ) ;
      metalocked = TRUE ;

      mbID = _collectionNameLookup( oldName ) ;
      if ( DMS_INVALID_MBID == mbID )
      {
         rc = SDB_DMS_NOTEXIST ;
         goto error ;
      }
      if ( DMS_INVALID_MBID != _collectionNameLookup ( newName ) )
      {
         rc = SDB_DMS_EXIST ;
         goto error ;
      }

      _collectionNameRemove ( oldName ) ;
      _collectionNameInsert ( newName, mbID ) ;
      ossMemset ( _dmsMME->_mbList[mbID]._collectionName, 0,
                  DMS_COLLECTION_NAME_SZ ) ;
      ossStrncpy ( _dmsMME->_mbList[mbID]._collectionName, newName,
                   DMS_COLLECTION_NAME_SZ ) ;
      clLID = _dmsMME->_mbList[mbID]._logicalID ;

      if ( dpscb )
      {
         rc = _logDPS( dpscb, info, cb, &_metadataLatch, EXCLUSIVE, metalocked,
                       clLID, DMS_INVALID_EXTENT ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to insert clrename to log, rc = %d",
                      rc ) ;
      }

   done :
      if ( metalocked )
      {
         ossUnlatch ( &_metadataLatch, EXCLUSIVE ) ;
         metalocked = FALSE ;
      }
      if ( 0 != logRecSize )
      {
         pTransCB->releaseLogSpace( logRecSize, cb );
      }
      if ( SDB_OK == rc )
      {
         flushMME( isSyncDeep() ) ;
      }
      PD_TRACE_EXITRC ( SDB__DMSSTORAGEDATA_RENAMECOLLECTION, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSTORAGEDATA_FINDCOLLECTION, "_dmsStorageData::findCollection" )
   INT32 _dmsStorageData::findCollection( const CHAR * pName,
                                          UINT16 & collectionID )
   {
      INT32 rc            = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__DMSSTORAGEDATA_FINDCOLLECTION ) ;
      PD_TRACE1 ( SDB__DMSSTORAGEDATA_FINDCOLLECTION,
                  PD_PACK_STRING ( pName ) ) ;
      ossLatch ( &_metadataLatch, SHARED ) ;
      collectionID = _collectionNameLookup ( pName ) ;
      PD_TRACE1 ( SDB__DMSSTORAGEDATA_FINDCOLLECTION,
                  PD_PACK_USHORT ( collectionID ) ) ;
      if ( DMS_INVALID_MBID == collectionID )
      {
         rc = SDB_DMS_NOTEXIST ;
         goto error ;
      }

   done :
      ossUnlatch ( &_metadataLatch, SHARED ) ;
      PD_TRACE_EXITRC ( SDB__DMSSTORAGEDATA_FINDCOLLECTION, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSTORAGEDATA_INSERTRECORD, "_dmsStorageData::insertRecord" )
   INT32 _dmsStorageData::insertRecord( dmsMBContext *context,
                                        const BSONObj &record,
                                        pmdEDUCB *cb,
                                        SDB_DPSCB *dpscb,
                                        BOOLEAN mustOID,
                                        BOOLEAN canUnLock )
   {
      INT32 rc                      = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__DMSSTORAGEDATA_INSERTRECORD ) ;
      IDToInsert oid ;
      idToInsertEle oidEle((CHAR*)(&oid)) ;
      UINT32 dmsRecordSize          = 0 ;
      CHAR fullName[DMS_COLLECTION_FULL_NAME_SZ + 1] = {0} ;
      CHAR *pMergedData             = NULL ;
      BSONObj insertObj             = record ;
      BOOLEAN hasInsert             = FALSE ;
      dpsTransCB *pTransCB          = pmdGetKRCB()->getTransCB() ;
      UINT32 logRecSize             = 0 ;
      monAppCB * pMonAppCB          = cb ? cb->getMonAppCB() : NULL ;
      dpsMergeInfo info ;
      dpsLogRecord &logRecord       = info.getMergeBlock().record() ;
      SDB_DPSCB *dropDps            = NULL ;
      // trans related
      DPS_TRANS_ID transID          = cb->getTransID() ;
      DPS_LSN_OFFSET preTransLsn    = cb->getCurTransLsn() ;
      DPS_LSN_OFFSET relatedLsn     = cb->getRelatedTransLSN() ;
      BOOLEAN  isTransLocked        = FALSE ;
      // delete record related
      dmsRecordID foundDeletedID ;
      dmsRecordData recordData ;
      dmsExtRW extRW ;
      dmsRecordRW recordRW ;
      const dmsExtent *pExtent      = NULL ;
      _dmsCompressorEntry *compressorEntry = &_compressorEntry[context->mbID()] ;

      /* For concurrency protection with drop CL and set compresor. */
      dmsCompressorGuard compGuard( compressorEntry, SHARED ) ;

      try
      {
         recordData.setData( record.objdata(), record.objsize(),
                             FALSE, TRUE ) ;
         // verify whether the record got "_id" inside
         BSONElement ele = record.getField( DMS_ID_KEY_NAME ) ;
         const CHAR *pCheckErr = "" ;
         if ( !dmsIsRecordIDValid( ele, TRUE, &pCheckErr ) )
         {
            PD_LOG( PDERROR, "Record[%s] _id is error: %s",
                    record.toString().c_str(), pCheckErr ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }
         // judge must oid
         if ( mustOID && ele.eoo() )
         {
            oid._oid.init() ;
            rc = cb->allocBuff( oidEle.size() + record.objsize(),
                                &pMergedData ) ;
            if ( rc )
            {
               PD_LOG( PDERROR, "Alloc memory[size:%u] failed, rc: %d",
                       oidEle.size() + record.objsize(), rc ) ;
               goto error ;
            }
            /// copy to new data
            *(UINT32*)pMergedData = oidEle.size() + record.objsize() ;
            ossMemcpy( pMergedData + sizeof(UINT32), oidEle.rawdata(),
                       oidEle.size() ) ;
            ossMemcpy( pMergedData + sizeof(UINT32) + oidEle.size(),
                       record.objdata() + sizeof(UINT32),
                       record.objsize() - sizeof(UINT32) ) ;
            recordData.setData( pMergedData,
                                oidEle.size() + record.objsize(),
                                FALSE, TRUE ) ;
            insertObj = BSONObj( pMergedData ) ;
         }
         dmsRecordSize = recordData.len() ;

         // check
         if ( recordData.len() + DMS_RECORD_METADATA_SZ >
              DMS_RECORD_USER_MAX_SZ )
         {
            rc = SDB_DMS_RECORD_TOO_BIG ;
            goto error ;
         }

         if ( compressorEntry->ready() )
         {
            const CHAR *compressedData    = NULL ;
            INT32 compressedDataSize      = 0 ;
            UINT8 compressRatio           = 0 ;
            rc = dmsCompress( cb, compressorEntry,
                              recordData.data(), recordData.len(),
                              &compressedData, &compressedDataSize,
                              compressRatio ) ;
            // Compression is valid and ratio is less the threshold
            if ( SDB_OK == rc &&
                 compressedDataSize + sizeof(UINT32) < recordData.orgLen() &&
                 compressRatio < DMS_COMPRESS_RATIO_THRESHOLD )
            {
               // 4 bytes len + compressed record
               dmsRecordSize = compressedDataSize + sizeof(UINT32) ;
               PD_TRACE2 ( SDB__DMSSTORAGEDATA_INSERTRECORD,
                           PD_PACK_STRING ( "size after compress" ),
                           PD_PACK_UINT ( dmsRecordSize ) ) ;

               // set the compression data
               recordData.setData( compressedData, compressedDataSize,
                                   TRUE, FALSE ) ;
            }
            else if ( rc )
            {
               // In any case of error, leave it, and use the original data.
               if ( SDB_UTIL_COMPRESS_ABORT == rc )
               {
                  PD_LOG( PDINFO, "Record compression aborted. "
                          "Insert the original data. rc: %d", rc ) ;
               }
               else
               {
                  PD_LOG( PDWARNING, "Record compression failed. "
                          "Insert the original data. rc: %d", rc ) ;
               }
               rc = SDB_OK ;
            }
         }

         /*
          * Release the guard to avoid deadlock with truncate/drop collection.
          */
         compGuard.release() ;

         // get the recordsize that we have to allocate
         _overflowSize( dmsRecordSize ) ;
         // add record metadata and oid
         dmsRecordSize += DMS_RECORD_METADATA_SZ ;
         // record is ALWAYS 4 bytes aligned
         dmsRecordSize = OSS_MIN( DMS_RECORD_MAX_SZ,
                                  ossAlignX ( dmsRecordSize, 4 ) ) ;
         PD_TRACE2 ( SDB__DMSSTORAGEDATA_INSERTRECORD,
                     PD_PACK_STRING ( "size after align" ),
                     PD_PACK_UINT ( dmsRecordSize ) ) ;

         // calc log reserve
         if ( dpscb )
         {
            _clFullName( context->mb()->_collectionName, fullName,
                         sizeof(fullName) ) ;
            rc = dpsInsert2Record( fullName, insertObj, transID,
                                   preTransLsn, relatedLsn, logRecord ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to build record, rc: %d", rc ) ;

            logRecSize = ossAlign4( logRecord.alignedLen() ) ;

            rc = dpscb->checkSyncControl( logRecSize, cb ) ;
            if ( SDB_OK != rc )
            {
               logRecSize = 0 ;
               PD_LOG( PDERROR, "Check sync control failed, rc: %d", rc ) ;
               goto error ;
            }

            rc = pTransCB->reservedLogSpace( logRecSize, cb ) ;
            if ( rc )
            {
               PD_LOG( PDERROR, "Failed to reserved log space(length=%u)",
                       logRecSize ) ;
               logRecSize = 0 ;
               goto error ;
            }
         }

         // lock mb
         rc = context->mbLock( EXCLUSIVE ) ;
         PD_RC_CHECK( rc, PDERROR, "dms mb context lock failed, rc: %d", rc ) ;

         // then make sure the collection compatiblity
         if ( !dmsAccessAndFlagCompatiblity ( context->mb()->_flag,
                                              DMS_ACCESS_TYPE_INSERT ) )
         {
            PD_LOG ( PDERROR, "Incompatible collection mode: %d",
                     context->mb()->_flag ) ;
            rc = SDB_DMS_INCOMPATIBLE_MODE ;
            goto error ;
         }

         // find delete record
         rc = _reserveFromDeleteList( context, dmsRecordSize,
                                      foundDeletedID, cb ) ;
         PD_RC_CHECK( rc, PDERROR, "Reserve delete record failed, "
                      "rc: %d", rc ) ;

         /// validate the extent page infomation
         extRW = extent2RW( foundDeletedID._extent, context->mbID() ) ;
         pExtent = extRW.readPtr<dmsExtent>() ;
         if ( !pExtent->validate( context->mbID() ) )
         {
            rc = SDB_SYS ;
            goto error ;
         }
         recordRW = record2RW( foundDeletedID, context->mbID() ) ;

         if ( dpscb )
         {
            rc = pTransCB->transLockTryX( cb, _logicalCSID, context->mbID(),
                                          &foundDeletedID ) ;
            SDB_ASSERT( SDB_OK == rc, "Failed to get record-X-LOCK" );
            PD_RC_CHECK( rc, PDERROR, "Failed to insert the record, get "
                        "transaction-X-lock of record failed, rc: %d", rc );
            isTransLocked = TRUE ;
         }
         // insert to extent
         rc = _extentInsertRecord( context, extRW, recordRW, recordData,
                                   dmsRecordSize, cb, TRUE ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to append record, rc: %d", rc ) ;

         hasInsert = TRUE ;
         // update totalInsert monitor counter
         DMS_MON_OP_COUNT_INC( pMonAppCB, MON_INSERT, 1 ) ;
         _incWriteRecord() ;

         /// insert object's indexes
         rc = _pIdxSU->indexesInsert( context, pExtent->_logicID,
                                      insertObj, foundDeletedID, cb ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to insert to index, rc: %d", rc ) ;
      }
      catch( std::exception &e )
      {
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         rc = pdGetLastError() ? pdGetLastError() : SDB_SYS ;
         goto error ;
      }

      if ( dpscb )
      {
         PD_AUDIT_OP_WITHNAME( AUDIT_INSERT, "INSERT", AUDIT_OBJ_CL,
                               fullName, rc, "%s",
                               insertObj.toString().c_str() ) ;

         /// enable trans
         info.enableTrans() ;
         rc = _logDPS( dpscb, info, cb, context,
                       pExtent->_logicID, canUnLock,
                       DMS_FILE_DATA ) ;
         PD_RC_CHECK ( rc, PDERROR, "Failed to insert record into log, "
                       "rc: %d", rc ) ;
         dropDps = dpscb ;
      }
      else if ( cb->getLsnCount() > 0 )
      {
         context->mbStat()->updateLastLSNWithComp( cb->getEndLsn(),
                                                   DMS_FILE_DATA,
                                                   cb->isDoRollback() ) ;
      }

   done:
      // release the lock immediately if it is not transaction-operation,
      // the transaction-operation's lock will release in rollback or commit
      if ( isTransLocked && ( transID == DPS_INVALID_TRANS_ID || rc ) )
      {
         pTransCB->transLockRelease( cb, _logicalCSID, context->mbID(),
                                     &foundDeletedID ) ;
      }
      if ( 0 != logRecSize )
      {
         pTransCB->releaseLogSpace( logRecSize, cb ) ;
      }
      if ( pMergedData )
      {
         cb->releaseBuff( pMergedData ) ;
      }
      PD_TRACE_EXITRC ( SDB__DMSSTORAGEDATA_INSERTRECORD, rc ) ;
      return rc ;
   error:
      if ( hasInsert )
      {
         INT32 rc1 = deleteRecord( context, foundDeletedID,
                                   (ossValuePtr)insertObj.objdata(),
                                   cb, dropDps ) ;
         if ( rc1 )
         {
            PD_LOG( PDERROR, "Failed to rollback, rc: %d", rc1 ) ;
         }
      }
      else if ( foundDeletedID.isValid() )
      {
         _saveDeletedRecord( context->mb(), foundDeletedID, 0 ) ;
      }
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSTORAGEDATA__EXTENTINSERTRECORD, "_dmsStorageData::_extentInsertRecord" )
   INT32 _dmsStorageData::_extentInsertRecord( dmsMBContext *context,
                                               dmsExtRW &extRW,
                                               dmsRecordRW &recordRW,
                                               const dmsRecordData &recordData,
                                               UINT32 needRecordSize,
                                               _pmdEDUCB *cb,
                                               BOOLEAN addIntoList )
   {
      INT32 rc                         = SDB_OK ;
      monAppCB * pMonAppCB             = cb ? cb->getMonAppCB() : NULL ;
      dmsRecord* pRecord               = NULL ;
      dmsOffset  myOffset              = DMS_INVALID_OFFSET ;

      PD_TRACE_ENTRY ( SDB__DMSSTORAGEDATA__EXTENTINSERTRECORD ) ;
      rc = context->mbLock( EXCLUSIVE ) ;
      PD_RC_CHECK( rc, PDERROR, "dms mb context lock failed, rc: %d", rc ) ;

      pRecord = recordRW.writePtr( needRecordSize ) ;
      myOffset = pRecord->getMyOffset() ;
      // first we need to check if the delete record is large enough
      if ( pRecord->getSize() < needRecordSize )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      else if ( !recordData.isCompressed() && recordData.len() < 5 )
      {
         PD_LOG( PDERROR, "Bson obj size[%d] is invalid",
                 recordData.len() ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      // set to normal status
      pRecord->setNormal() ;
      pRecord->resetAttr() ;

      // and then need to check if we need to split deleted record
      if ( pRecord->getSize() - needRecordSize > DMS_MIN_RECORD_SZ )
      {
         // original offset+new size = new delete offset
         dmsOffset newOffset = myOffset + needRecordSize ;
         // original size - new size = new delete size
         INT32 newSize = pRecord->getSize() - needRecordSize ;
         dmsRecordID newRid = recordRW.getRecordID() ;
         newRid._offset = newOffset ;
         rc = _saveDeletedRecord( context->mb(), newRid, newSize ) ;
         if ( rc )
         {
            PD_LOG ( PDERROR, "Failed to save deleted record, rc: %d", rc ) ;
            goto error ;
         }
         // set the original place with new dmsrecordSize
         pRecord->setSize( needRecordSize ) ;
      }
      // if the leftover space is not good enough for a min_record, then we
      // don't change the record size

      // then for the original location we set new record header and copy data
      pRecord->setData( recordData ) ;

      pRecord->setNextOffset( DMS_INVALID_OFFSET ) ;
      pRecord->setPrevOffset( DMS_INVALID_OFFSET ) ;

      // increase write counter
      DMS_MON_OP_COUNT_INC( pMonAppCB, MON_DATA_WRITE, 1 ) ;

      // no need to change offset
      if ( addIntoList )
      {
         dmsExtent *extent       = extRW.writePtr<dmsExtent>() ;
         dmsOffset   offset      = extent->_lastRecordOffset ;
         // finally add the record into list
         extent->_recCount++ ;
         ++( _mbStatInfo[ context->mbID() ]._totalRecords ) ;
         // if there is last record in the extent
         if ( DMS_INVALID_OFFSET != offset )
         {
            // if there is already record in the extent
            dmsRecordRW preRW = record2RW( dmsRecordID( extRW.getExtentID(),
                                                        offset ),
                                           context->mbID() ) ;
            dmsRecord *preRecord = preRW.writePtr() ;
            // set the next of previous point to the new record
            preRecord->setNextOffset( myOffset ) ;
            // set the previous of current points to the original last
            pRecord->setPrevOffset( offset ) ;
         }
         extent->_lastRecordOffset = myOffset ;
         // then check for first record in extent
         if ( DMS_INVALID_OFFSET == extent->_firstRecordOffset )
         {
            // we only change it when it points to nothing
            extent->_firstRecordOffset = myOffset ;
         }
      }

      _mbStatInfo[context->mbID()]._lastCompressRatio =
         (UINT8)( recordData.getCompressRatio() * 100 ) ;
      _mbStatInfo[context->mbID()]._totalOrgDataLen += recordData.orgLen() ;
      _mbStatInfo[context->mbID()]._totalDataLen += recordData.len() ;

   done :
      PD_TRACE_EXITRC ( SDB__DMSSTORAGEDATA__EXTENTINSERTRECORD, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSTORAGEDATA_DELETERECORD, "_dmsStorageData::deleteRecord" )
   INT32 _dmsStorageData::deleteRecord( dmsMBContext *context,
                                        const dmsRecordID &recordID,
                                        ossValuePtr deletedDataPtr,
                                        pmdEDUCB *cb,
                                        SDB_DPSCB * dpscb )
   {
      INT32 rc                      = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__DMSSTORAGEDATA_DELETERECORD ) ;
      dpsTransCB *pTransCB          = pmdGetKRCB()->getTransCB() ;
      monAppCB * pMonAppCB          = cb ? cb->getMonAppCB() : NULL ;
      BOOLEAN isDeleting            = FALSE ;
      BOOLEAN isNeedToSetDeleting   = FALSE ;

      dmsRecordID ovfRID ;
      BSONObj delObject ;
      UINT32 logRecSize             = 0 ;
      dpsMergeInfo info ;
      dpsLogRecord &record          = info.getMergeBlock().record() ;
      CHAR fullName[DMS_COLLECTION_FULL_NAME_SZ + 1] = {0} ;
      DPS_TRANS_ID transID          = cb->getTransID() ;
      DPS_LSN_OFFSET preLsn         = cb->getCurTransLsn() ;
      DPS_LSN_OFFSET relatedLSN     = cb->getRelatedTransLSN() ;
      dmsExtRW extRW ;
      dmsRecordRW recordRW ;
      dmsExtent *pExtent            = NULL ;
      dmsRecord *pRecord            = NULL ;
      dmsRecordData recordData ;

      if ( !context->isMBLock( EXCLUSIVE ) )
      {
         PD_LOG( PDERROR, "Caller must hold mb exclusive lock[%s]",
                 context->toString().c_str() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

#ifdef _DEBUG
      if ( !dmsAccessAndFlagCompatiblity ( context->mb()->_flag,
                                           DMS_ACCESS_TYPE_DELETE ) )
      {
         PD_LOG ( PDERROR, "Incompatible collection mode: %d",
                  context->mb()->_flag ) ;
         rc = SDB_DMS_INCOMPATIBLE_MODE ;
         goto error ;
      }
#endif //_DEBUG

      try
      {
         extRW = extent2RW( recordID._extent, context->mbID() ) ;
         pExtent = extRW.writePtr<dmsExtent>() ;
         recordRW = record2RW( recordID, context->mbID() ) ;
         pRecord = recordRW.writePtr() ;

         if ( pRecord->isOvf() )
         {
            ovfRID = pRecord->getOvfRID() ;
         }
         else if ( pRecord->isOvt() )
         {
            _extentRemoveRecord( context, extRW, recordRW, cb ) ;
            goto done ;
         }
         else if ( pRecord->isDeleted() )
         {
            rc = SDB_DMS_RECORD_NOTEXIST ;
            goto error ;
         }
         else if ( !pRecord->isNormal() )
         {
            rc = SDB_SYS ;
            goto error ;
         }

         // don't delete the record, someone are waitting for the record-X-Lock,
         // mark the record's attr to DMS_RECORD_FLAG_DELETING and write the log
         // the last one who get the record-X-Lock will delete the record.
         if ( pTransCB->hasWait( _logicalCSID, context->mbID(), &recordID ) )
         {
            if ( pRecord->isDeleting() )
            {
               rc = SDB_OK ;
               goto done ;
            }
            isNeedToSetDeleting = TRUE ;
         }
         else if ( pRecord->isDeleting() )
         {
            isDeleting = TRUE ;
         }

         if ( FALSE == isDeleting ) // first delete the record
         {
            if ( deletedDataPtr )
            {
               recordData.setData( (const CHAR*)deletedDataPtr,
                                   *(UINT32*)deletedDataPtr,
                                   FALSE, TRUE ) ;
               if ( pRecord->isCompressed() )
               {
                  recordData.setOrgData( NULL, _getRecordDataLen( pRecord ) ) ;
               }
            }
            else
            {
               rc = extractData( context, recordRW, cb, recordData ) ;
               PD_RC_CHECK( rc, PDERROR, "Extract data failed, rc: %d", rc ) ;
            }

            // delete index keys
            try
            {
               delObject = BSONObj( recordData.data() ) ;
               // first to reserve dps
               if ( NULL != dpscb )
               {
                  _clFullName( context->mb()->_collectionName, fullName,
                               sizeof(fullName) ) ;
                  // reserved log-size
                  rc = dpsDelete2Record( fullName, delObject, transID,
                                         preLsn, relatedLSN, record ) ;
                  if ( SDB_OK != rc )
                  {
                     PD_LOG( PDERROR, "Failed to build record: %d",rc ) ;
                     goto error ;
                  }

                  rc = dpscb->checkSyncControl( record.alignedLen(), cb ) ;
                  PD_RC_CHECK( rc, PDERROR, "Check sync control failed, rc: %d",
                               rc ) ;

                  logRecSize = record.alignedLen() ;
                  rc = pTransCB->reservedLogSpace( logRecSize, cb ) ;
                  if ( rc )
                  {
                     PD_LOG( PDERROR, "Failed to reserved log space(length=%u)",
                             logRecSize ) ;
                     logRecSize = 0 ;
                     goto error ;
                  }
               }
               // then delete indexes
               rc = _pIdxSU->indexesDelete( context, pExtent->_logicID,
                                            delObject, recordID, cb ) ;
               if ( rc )
               {
                  // if index delete fail, let's continue remove the record
                  PD_LOG ( PDERROR, "Failed to delete indexes, rc: %d",rc ) ;
               }
               context->mbStat()->_totalDataLen -= recordData.orgLen() ;
               context->mbStat()->_totalOrgDataLen -= recordData.len() ;
            }
            catch ( std::exception &e )
            {
               PD_LOG ( PDERROR, "Corrupted record: %d:%d: %s",
                        recordID._extent, recordID._offset, e.what() ) ;
               rc = SDB_CORRUPTED_RECORD ;
               goto error ;
            }
         }

         // no wait for X lock, delete really
         if ( FALSE == isNeedToSetDeleting )
         {
            rc = _extentRemoveRecord( context, extRW, recordRW, cb,
                                      !isDeleting ) ;
            PD_RC_CHECK( rc, PDERROR, "Extent remove record failed, "
                         "rc: %d", rc ) ;
            if ( ovfRID.isValid() )
            {
               dmsRecordRW ovfRW = record2RW( ovfRID, context->mbID() ) ;
               _extentRemoveRecord( context, extRW, ovfRW, cb, FALSE ) ;
            }
         }
         // set deleting attr
         else
         {
            pRecord->setDeleting() ;
            // need to dec count
            --( pExtent->_recCount ) ;
            --( _mbStatInfo[ context->mbID() ]._totalRecords ) ;
         }

         if ( FALSE == isDeleting )
         {
            // increase conter
            DMS_MON_OP_COUNT_INC( pMonAppCB, MON_DELETE, 1 ) ;
            _incWriteRecord() ;
         }
      }
      catch( std::exception &e )
      {
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         rc = pdGetLastError() ? pdGetLastError() : SDB_SYS ;
         goto error ;
      }

      // if we are asked to log
      if ( dpscb && FALSE == isDeleting )
      {
         try
         {
            PD_AUDIT_OP_WITHNAME( AUDIT_DELETE, "DELETE", AUDIT_OBJ_CL,
                                  fullName, rc, "%s",
                                  BSONObj( recordData.data() ).toString().c_str() ) ;
         }
         catch( std::exception &e )
         {
            PD_LOG( PDERROR, "Failed to audit delete record: %s", e.what() ) ;
            /// ignore the error
         }

         info.enableTrans() ;
         rc = _logDPS( dpscb, info, cb, context, pExtent->_logicID, FALSE,
                       DMS_FILE_DATA ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to insert record into log, rc: %d",
                      rc ) ;
      }
      else if ( FALSE == isDeleting && cb->getLsnCount() > 0 )
      {
         context->mbStat()->updateLastLSNWithComp( cb->getEndLsn(),
                                                   DMS_FILE_DATA,
                                                   cb->isDoRollback() ) ;
      }

   done :
      if ( 0 != logRecSize )
      {
         pTransCB->releaseLogSpace( logRecSize, cb ) ;
      }
      PD_TRACE_EXITRC ( SDB__DMSSTORAGEDATA_DELETERECORD, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSTORAGEDATA__EXTENTREMOVERECORD, "_dmsStorageData::_extentRemoveRecord" )
   INT32 _dmsStorageData::_extentRemoveRecord( dmsMBContext *context,
                                               dmsExtRW &extRW,
                                               dmsRecordRW &recordRW,
                                               _pmdEDUCB *cb,
                                               BOOLEAN decCount )
   {
      INT32 rc              = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__DMSSTORAGEDATA__EXTENTREMOVERECORD ) ;
      monAppCB * pMonAppCB  = cb ? cb->getMonAppCB() : NULL ;

      dmsExtent *pExtent = NULL ;
      const dmsRecord *pRecord = NULL ;
      dmsRecordID rid = recordRW.getRecordID() ;

      if ( !context->isMBLock( EXCLUSIVE ) )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Caller must hold exclusive lock[%s]",
                 context->toString().c_str() ) ;
         goto error ;
      }

      pExtent = extRW.writePtr<dmsExtent>() ;
      pRecord = recordRW.readPtr() ;

      // not over-flow to record
      if ( !pRecord->isOvt() )
      {
         dmsOffset prevRecordOffset = pRecord->getPrevOffset() ;
         dmsOffset nextRecordOffset = pRecord->getNextOffset() ;

         if ( DMS_INVALID_OFFSET != prevRecordOffset )
         {
            dmsRecordID prevRID = rid ;
            prevRID._offset = prevRecordOffset ;
            dmsRecordRW prevRW = record2RW( prevRID, context->mbID() ) ;
            dmsRecord *prevRecord = prevRW.writePtr() ;
            prevRecord->setNextOffset( nextRecordOffset ) ;
         }
         if ( DMS_INVALID_OFFSET != nextRecordOffset )
         {
            dmsRecordID nextRID = rid ;
            nextRID._offset = nextRecordOffset ;
            dmsRecordRW nextRW = record2RW( nextRID, context->mbID() ) ;
            dmsRecord *nextRecord = nextRW.writePtr() ;
            nextRecord->setPrevOffset( prevRecordOffset ) ;
         }
         if ( pExtent->_firstRecordOffset == rid._offset )
         {
            pExtent->_firstRecordOffset = nextRecordOffset ;
         }
         if ( pExtent->_lastRecordOffset == rid._offset )
         {
            pExtent->_lastRecordOffset = prevRecordOffset ;
         }

         if ( decCount )
         {
            --(pExtent->_recCount) ;
            --( _mbStatInfo[ context->mbID() ]._totalRecords ) ;
         }
      }
      //increase data write counter
      DMS_MON_OP_COUNT_INC( pMonAppCB, MON_DATA_WRITE, 1 ) ;

      rc = _saveDeletedRecord( context->mb(), rid, 0 ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Failed to save deleted record, rc = %d", rc ) ;
         goto error ;
      }
   done :
      PD_TRACE_EXITRC ( SDB__DMSSTORAGEDATA__EXTENTREMOVERECORD, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSTORAGEDATA_UPDATERECORD, "_dmsStorageData::updateRecord" )
   INT32 _dmsStorageData::updateRecord( dmsMBContext *context,
                                        const dmsRecordID &recordID,
                                        ossValuePtr updatedDataPtr,
                                        pmdEDUCB *cb, SDB_DPSCB *dpscb,
                                        mthModifier &modifier,
                                        BSONObj* newRecord )
   {
      INT32 rc                      = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__DMSSTORAGEDATA_UPDATERECORD ) ;
      monAppCB * pMonAppCB          = cb ? cb->getMonAppCB() : NULL ;
      BSONObj oldMatch, oldChg ;
      BSONObj newMatch, newChg ;
      UINT32 logRecSize             = 0 ;
      dpsMergeInfo info ;
      dpsLogRecord &record = info.getMergeBlock().record() ;
      dpsTransCB *pTransCB = pmdGetKRCB()->getTransCB() ;
      CHAR fullName[DMS_COLLECTION_FULL_NAME_SZ + 1] = {0} ;
      DPS_TRANS_ID transID = cb->getTransID() ;
      DPS_LSN_OFFSET preTransLsn = cb->getCurTransLsn() ;
      DPS_LSN_OFFSET relatedLSN = cb->getRelatedTransLSN() ;

      dmsExtRW extRW ;
      dmsRecordRW recordRW ;
      const dmsExtent *pExtent  = NULL ;
      const dmsRecord *pRecord = NULL ;
      dmsRecordData recordData ;

      if ( !context->isMBLock( EXCLUSIVE ) )
      {
         PD_LOG( PDERROR, "Caller must hold mb exclusive lock[%s]",
                 context->toString().c_str() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      try
      {
         extRW = extent2RW( recordID._extent, context->mbID() ) ;
         pExtent = extRW.readPtr<dmsExtent>() ;
         recordRW = record2RW( recordID, context->mbID() ) ;
         pRecord = recordRW.readPtr() ;

#ifdef _DEBUG
         if ( !dmsAccessAndFlagCompatiblity ( context->mb()->_flag,
                                              DMS_ACCESS_TYPE_UPDATE ) )
         {
            PD_LOG ( PDERROR, "Incompatible collection mode: %d",
                     context->mb()->_flag ) ;
            rc = SDB_DMS_INCOMPATIBLE_MODE ;
            goto error ;
         }
         // validate the extent is valid
         if ( !pExtent->validate( context->mbID() ) )
         {
            rc = SDB_SYS ;
            goto error ;
         }
#endif //_DEBUG

         // get data
         if ( updatedDataPtr )
         {
            recordData.setData( (const CHAR*)updatedDataPtr,
                                *(UINT32*)updatedDataPtr,
                                FALSE, TRUE ) ;
            if ( pRecord->isCompressed() )
            {
               recordData.setOrgData( NULL, _getRecordDataLen( pRecord ) ) ;
            }
         }
         else
         {
            rc = extractData(  context, recordRW, cb, recordData ) ;
            PD_RC_CHECK( rc, PDERROR, "Extract record data failed, rc: %d",
                         rc ) ;
         }

         try
         {
            BSONObj obj ( recordData.data() ) ;
            // create a new object for updated record
            BSONObj newobj ;

            if ( dpscb )
            {
               rc = modifier.modify ( obj, newobj, &oldMatch, &oldChg,
                                      &newMatch, &newChg ) ;
               if ( SDB_OK == rc && newChg.isEmpty() )
               {
                  SDB_ASSERT( oldChg.isEmpty(),
                              "Old change must be empty" ) ;
                  goto done ;
               }
            }
            else
            {
               rc = modifier.modify ( obj, newobj ) ;
            }

            if ( rc )
            {
               PD_LOG ( PDERROR, "Failed to create modified record, rc: %d",
                        rc ) ;
               goto error ;
            }

            if ( NULL != dpscb )
            {
               _clFullName( context->mb()->_collectionName, fullName,
                            sizeof(fullName) ) ;
               // reserved log-size
               rc = dpsUpdate2Record( fullName, oldMatch, oldChg, newMatch,
                                      newChg, transID, preTransLsn,
                                      relatedLSN, record ) ;
               if ( SDB_OK != rc )
               {
                  PD_LOG( PDERROR, "failed to build record:%d", rc ) ;
                  goto error ;
               }

               rc = dpscb->checkSyncControl( record.alignedLen(), cb ) ;
               PD_RC_CHECK( rc, PDERROR, "Check sync control failed, rc: %d",
                            rc ) ;

               logRecSize = record.alignedLen() ;
               rc = pTransCB->reservedLogSpace( logRecSize, cb );
               if ( rc )
               {
                  PD_LOG( PDERROR, "Failed to reserved log space(len:%u), "
                          "rc: %d", logRecSize, rc ) ;
                  logRecSize = 0 ;
                  goto error ;
               }
            }

            rc = _extentUpdatedRecord( context, extRW, recordRW,
                                       recordData, newobj, cb ) ;
            if ( rc )
            {
               PD_LOG ( PDERROR, "Failed to update record, rc: %d", rc ) ;
               goto error ;
            }

            if ( NULL != newRecord )
            {
               *newRecord = newobj.getOwned() ;
            }
         }
         catch ( std::exception &e )
         {
            PD_LOG ( PDERROR, "Failed to create BSON object: %s", e.what() ) ;
            rc = SDB_CORRUPTED_RECORD ;
            goto error ;
         }

         // increase update counter
         DMS_MON_OP_COUNT_INC( pMonAppCB, MON_UPDATE, 1 ) ;
         _incWriteRecord() ;
      }
      catch( std::exception &e )
      {
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         rc = pdGetLastError() ? pdGetLastError() : SDB_SYS ;
         goto error ;
      }

      // log update information
      if ( dpscb )
      {
         PD_LOG ( PDDEBUG, "oldChange: %s,%s\nnewChange: %s,%s",
                  oldMatch.toString().c_str(),
                  oldChg.toString().c_str(),
                  newMatch.toString().c_str(),
                  newChg.toString().c_str() ) ;

         PD_AUDIT_OP_WITHNAME( AUDIT_UPDATE, "UPDATE", AUDIT_OBJ_CL,
                               fullName, rc, "OldMatch:%s, OldChange:%s, "
                               "NewMatch:%s, NewChange:%s",
                               oldMatch.toString().c_str(),
                               oldChg.toString().c_str(),
                               newMatch.toString().c_str(),
                               newChg.toString().c_str() ) ;

         info.enableTrans() ;
         rc = _logDPS( dpscb, info, cb, context, pExtent->_logicID, FALSE,
                       DMS_FILE_DATA ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to insert update record into log, "
                      "rc: %d", rc ) ;
      }
      else if ( cb->getLsnCount() > 0 )
      {
         context->mbStat()->updateLastLSNWithComp( cb->getEndLsn(),
                                                   DMS_FILE_DATA,
                                                   cb->isDoRollback() ) ;
      }

   done :
      if ( 0 != logRecSize )
      {
         pTransCB->releaseLogSpace( logRecSize, cb );
      }
      PD_TRACE_EXITRC ( SDB__DMSSTORAGEDATA_UPDATERECORD, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSTORAGEDATA__EXTENTUPDATERECORD, "_dmsStorageData::_extentUpdatedRecord" )
   INT32 _dmsStorageData::_extentUpdatedRecord( dmsMBContext *context,
                                                dmsExtRW &extRW,
                                                dmsRecordRW &recordRW,
                                                const dmsRecordData &recordData,
                                                const BSONObj &newObj,
                                                _pmdEDUCB *cb )
   {
      INT32 rc                     = SDB_OK ;
      UINT32 dmsRecordSize         = 0 ;
      PD_TRACE_ENTRY ( SDB__DMSSTORAGEDATA__EXTENTUPDATERECORD ) ;
      monAppCB * pMonAppCB         = cb ? cb->getMonAppCB() : NULL ;
      dmsCompressorEntry *compressorEntry = &_compressorEntry[context->mbID()] ;
      const dmsExtent *pExtent     = NULL ;
      dmsRecord *pRecord           = NULL ;

      dmsRecordID ovfRID ;
      dmsExtRW ovfExtRW ;
      dmsRecordRW ovfRW ;
      dmsRecord *pOvfRecord        = NULL ;
      dmsRecordData newRecordData ;

      BOOLEAN rollbackIndex        = FALSE ;

      SDB_ASSERT ( !recordData.isEmpty(), "recordData can't be empty" ) ;

      // Check the new object size
      if ( newObj.objsize() + DMS_RECORD_METADATA_SZ > DMS_RECORD_USER_MAX_SZ )
      {
         PD_LOG ( PDERROR, "record is too big: %d", newObj.objsize() ) ;
         rc = SDB_DMS_RECORD_TOO_BIG ;
         goto error ;
      }

      if ( !context->isMBLock( EXCLUSIVE ) )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Caller must hold exclusive lock[%s]",
                 context->toString().c_str() ) ;
         goto error ;
      }

      try
      {
         pExtent = extRW.readPtr<dmsExtent>() ;
         pRecord = recordRW.writePtr( 0 ) ;

         newRecordData.setData( newObj.objdata(), newObj.objsize(),
                                FALSE, TRUE ) ;
         dmsRecordSize = newRecordData.len() ;

         // compress data
         if ( compressorEntry->ready() )
         {
            INT32 compressedDataSize     = 0 ;
            const CHAR *compressedData   = NULL ;
            UINT8 compressRatio          = 0 ;
            rc = dmsCompress( cb, compressorEntry,
                              newObj, NULL, 0,
                              &compressedData, &compressedDataSize,
                              compressRatio ) ;
            // Compression is valid and ratio is less the threshold
            if ( SDB_OK == rc &&
                 compressedDataSize + sizeof(UINT32) < newRecordData.orgLen() &&
                 compressRatio < DMS_COMPRESS_RATIO_THRESHOLD )
            {
               // 4 bytes len + compressed record
               dmsRecordSize = compressedDataSize + sizeof(UINT32) ;
               PD_TRACE2 ( SDB__DMSSTORAGEDATA__EXTENTUPDATERECORD,
                           PD_PACK_STRING ( "size after compress" ),
                           PD_PACK_UINT ( dmsRecordSize ) ) ;

               // set the compression data
               newRecordData.setData( compressedData, compressedDataSize,
                                      TRUE, FALSE ) ;
            }
         }

         // add metadata to size
         dmsRecordSize += DMS_RECORD_METADATA_SZ ;
         {
            // before moving on, let's first make sure the new object doesn't
            // violate any index unique rule
            BSONObj oriObj( recordData.data() ) ;
            BSONObj newObj( newRecordData.orgData() ) ;
            rc = _pIdxSU->indexesUpdate( context, pExtent->_logicID,
                                         oriObj, newObj,
                                         recordRW.getRecordID(),
                                         cb, FALSE ) ;
            rollbackIndex = TRUE ;
            if ( rc )
            {
               PD_LOG ( PDWARNING, "Failed to update index, rc: %d", rc ) ;
               goto error ;
            }
         }

         if ( pRecord->isOvf() )
         {
            ovfRID = pRecord->getOvfRID() ;
            DMS_MON_OP_COUNT_INC( pMonAppCB, MON_DATA_READ, 1 ) ;
            ovfExtRW = extent2RW( ovfRID._extent, context->mbID() ) ;
            ovfRW = record2RW( ovfRID, context->mbID() ) ;
            pOvfRecord = ovfRW.writePtr( 0 ) ;
         }

         // if the current space is big enough for the whole record,
         // let's put it here and return rightaway
         if ( dmsRecordSize <= pRecord->getSize() )
         {
            pRecord->setData( newRecordData ) ;
            DMS_MON_OP_COUNT_INC( pMonAppCB, MON_DATA_WRITE, 1 ) ;

            if ( ovfRID.isValid() )
            {
               _extentRemoveRecord( context, ovfExtRW, ovfRW, cb, FALSE ) ;
               pRecord->setNormal() ;
            }
            /// sub the remove data info
            context->mbStat()->_totalDataLen -= recordData.orgLen() ;
            context->mbStat()->_totalOrgDataLen -= recordData.len() ;
            context->mbStat()->_totalDataLen += newRecordData.len() ;
            context->mbStat()->_totalOrgDataLen += newRecordData.orgLen() ;
            goto done ;
         }
         else if ( pOvfRecord && dmsRecordSize <= pOvfRecord->getSize() )
         {
            pOvfRecord->setData( newRecordData ) ;
            DMS_MON_OP_COUNT_INC( pMonAppCB, MON_DATA_WRITE, 1 ) ;
            /// sub the remove data info
            context->mbStat()->_totalDataLen -= recordData.orgLen() ;
            context->mbStat()->_totalOrgDataLen -= recordData.len() ;
            context->mbStat()->_totalDataLen += newRecordData.len() ;
            context->mbStat()->_totalOrgDataLen += newRecordData.orgLen() ;
            goto done ;
         }
         // over-flow recrod
         else
         {
            dmsRecordID foundDeletedID ;
            dmsExtRW newExtRW ;
            dmsRecordRW newRecordRW ;
            const dmsExtent *pNewExtent = NULL ;
            dmsRecord *pNewRecord = NULL ;

            dmsRecordSize -= DMS_RECORD_METADATA_SZ ;
            // get the recordsize that we have to allocate
            _overflowSize( dmsRecordSize ) ;
            dmsRecordSize += DMS_RECORD_METADATA_SZ ;
            // record is ALWAYS 4 bytes aligned
            dmsRecordSize = OSS_MIN( DMS_RECORD_MAX_SZ,
                                     ossAlignX ( dmsRecordSize, 4 ) ) ;

            // find a free spot from delete list
            rc = _reserveFromDeleteList ( context, dmsRecordSize,
                                          foundDeletedID, cb ) ;
            if ( rc )
            {
               PD_LOG( PDERROR, "Failed to reserve delete record, rc: %d", rc ) ;
               goto error ;
            }
            newExtRW = extent2RW( foundDeletedID._extent, context->mbID() ) ;
            newRecordRW = record2RW( foundDeletedID, context->mbID() ) ;
            pNewExtent = newExtRW.readPtr<dmsExtent>() ;
            pNewRecord = newRecordRW.writePtr() ;

            if ( !pNewExtent->validate( context->mbID() ) )
            {
               PD_LOG ( PDERROR, "Invalid extent[%d] is detected",
                        foundDeletedID._extent ) ;
               rc = SDB_SYS ;
               goto error ;
            }
            // pass FALSE to addIntoList so that we don't add the record into
            // target extent's list
            rc = _extentInsertRecord ( context, newExtRW, newRecordRW,
                                       newRecordData, dmsRecordSize,
                                       cb, FALSE ) ;
            if ( rc )
            {
               PD_LOG ( PDERROR, "Failed to append record due to %d", rc ) ;
               goto error ;
            }
            // set remote record as overflowed to
            pNewRecord->setOvt() ;
            pRecord->setOvf() ;
            pRecord->setOvfRID( foundDeletedID ) ;
            if ( ovfRID.isValid() )
            {
               // overflowed record removal is done here, and it will mark the
               // segment dirty in the function
               _extentRemoveRecord( context, ovfExtRW, ovfRW, cb, FALSE ) ;
            }

            /// sub the remove data info
            context->mbStat()->_totalDataLen -= recordData.orgLen() ;
            context->mbStat()->_totalOrgDataLen -= recordData.len() ;
         }
      }
      catch( std::exception &e )
      {
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         rc = pdGetLastError() ? pdGetLastError() : SDB_SYS ;
         goto error ;
      }

   done :
      PD_TRACE_EXITRC ( SDB__DMSSTORAGEDATA__EXTENTUPDATERECORD, rc ) ;
      return rc ;
   error :
      if( rollbackIndex )
      {
         BSONObj oriObj( recordData.data() ) ;
         BSONObj newObj( newRecordData.orgData() ) ;
         // rollback the change on index by switching obj and oriObj
         INT32 rc1 = _pIdxSU->indexesUpdate( context, pExtent->_logicID,
                                             newObj, oriObj,
                                             recordRW.getRecordID(),
                                             cb, TRUE ) ;
         if ( rc1 )
         {
            PD_LOG ( PDERROR, "Failed to rollback update due to rc %d", rc1 ) ;
         }
      }
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSTORAGEDATA_FETCH, "_dmsStorageData::fetch" )
   INT32 _dmsStorageData::fetch( dmsMBContext *context,
                                 const dmsRecordID &recordID,
                                 BSONObj &dataRecord,
                                 pmdEDUCB * cb,
                                 BOOLEAN dataOwned )
   {
      INT32 rc                     = SDB_OK ;
      dmsRecordData recordData ;
      dmsExtRW extRW ;
      dmsRecordRW recordRW ;
      const dmsExtent *pExtent     = NULL ;
      const dmsRecord *pRecord     = NULL ;

      PD_TRACE_ENTRY ( SDB__DMSSTORAGEDATA_FETCH ) ;

      if ( !context->isMBLock() )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Caller must hold mb lock[%s]",
                 context->toString().c_str() ) ;
         goto error ;
      }

      try
      {
         if ( !dmsAccessAndFlagCompatiblity ( context->mb()->_flag,
                                              DMS_ACCESS_TYPE_FETCH ) )
         {
            PD_LOG ( PDERROR, "Incompatible collection mode: %d",
                     context->mb()->_flag ) ;
            rc = SDB_DMS_INCOMPATIBLE_MODE ;
            goto error ;
         }

         extRW = extent2RW( recordID._extent, context->mbID() ) ;
         recordRW = record2RW( recordID, context->mbID() ) ;
         pExtent = extRW.readPtr<dmsExtent>() ;

         // validate extent
         if ( !pExtent->validate( context->mbID()) )
         {
            PD_LOG ( PDERROR, "Invalid extent[%d]", recordID._extent ) ;
            rc = SDB_SYS ;
            goto error ;
         }

         pRecord = recordRW.readPtr() ;

         // make sure the record is not deleted
         // since we get the RID outside the latch scope, so there could be
         // inconsistency happen, we need to make sure the flag is expected
         if ( pRecord->isDeleted() )
         {
            rc = SDB_DMS_NOTEXIST ;
            goto error ;
         }
         else if ( pRecord->isDeleting() )
         {
            rc = SDB_DMS_DELETING ;
            goto error ;
         }

#ifdef _DEBUG
         // but the status shouldn't be in other flag
         if ( !pRecord->isNormal() && !pRecord->isOvf() )
         {
            PD_LOG ( PDERROR, "Record[%d:%d] flag[%d] error", recordID._extent,
                     recordID._offset, pRecord->getState() ) ;
            rc = SDB_SYS ;
            goto error ;
         }
#endif //_DEBUG

         // if this record is overflow from
         rc = extractData( context, recordRW, cb, recordData ) ;
         PD_RC_CHECK( rc, PDERROR, "Extract record data failed, rc: %d", rc ) ;

         if ( dataOwned )
         {
            dataRecord = BSONObj( recordData.data() ).getOwned() ;
         }
         else
         {
            dataRecord = BSONObj( recordData.data() ) ;
         }
      }
      catch( std::exception &e )
      {
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         rc = pdGetLastError() ? pdGetLastError() : SDB_SYS ;
         goto error ;
      }

   done :
      PD_TRACE_EXITRC ( SDB__DMSSTORAGEDATA_FETCH, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSTORAGEDATA_SETCOMPRESSOR, "_dmsStorageData::_setCompressor" )
   void _dmsStorageData::_setCompressor( dmsMBContext *context )
   {
      PD_TRACE_ENTRY( SDB__DMSSTORAGEDATA_SETCOMPRESSOR ) ;

      if ( OSS_BIT_TEST( context->mb()->_attributes,
                         DMS_MB_ATTR_COMPRESSED ) )
      {
         UINT16 mbID = context->mbID() ;
         UTIL_COMPRESSOR_TYPE type =
            (UTIL_COMPRESSOR_TYPE)context->mb()->_compressorType ;

         /// only set when snappy
         if ( UTIL_COMPRESSOR_SNAPPY == type )
         {
            dmsCompressorGuard guard( &_compressorEntry[ mbID ], EXCLUSIVE ) ;
            _compressorEntry[mbID].setCompressor( getCompressorByType( type ) ) ;
         }
      }

      PD_TRACE_EXIT( SDB__DMSSTORAGEDATA_SETCOMPRESSOR ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSTORAGEDATA_RMCOMPRESSOR, "_dmsStorageData::_rmCompressor" )
   void _dmsStorageData::_rmCompressor( _dmsMBContext *context )
   {
      PD_TRACE_ENTRY( SDB__DMSSTORAGEDATA_RMCOMPRESSOR ) ;
      dmsCompressorGuard compGuard( &_compressorEntry[context->mbID()],
                                    EXCLUSIVE ) ;
      utilDictHandle dictAddr =
         _compressorEntry[ context->mbID() ].getDictionary() ;

      if ( UTIL_INVALID_DICT != dictAddr )
      {
         endFixedAddr( (const ossValuePtr)dictAddr -
                       DMS_DICTEXTENT_HEADER_SZ ) ;
      }
      _compressorEntry[context->mbID()].reset() ;
      PD_TRACE_EXIT( SDB__DMSSTORAGEDATA_RMCOMPRESSOR ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSTORAGEDATA_DICTPERSIST, "_dmsStorageData::dictPersist" )
   INT32 _dmsStorageData::dictPersist( UINT16 mbID, UINT32 clLID,
                                       const CHAR *dict, UINT32 dictLen )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__DMSSTORAGEDATA_DICTPERSIST ) ;
      dmsExtentID dictExtID = DMS_INVALID_EXTENT ;
      dmsDictExtent *dictExtent = NULL ;
      dmsMBContext *context = NULL ;
      dmsCompressorEntry *compressorEntry = &_compressorEntry[ mbID ] ;
      dmsExtRW extRW ;

      /* Number of pages to store the dictionary, including the extent header.*/
      UINT32 pageNum = ( sizeof( dmsDictExtent ) + dictLen +
                         ( pageSize() - 1 ) ) / pageSize() ;

      rc = getMBContext( &context, mbID, clLID, EXCLUSIVE ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get dms mb context, rc: %d", rc ) ;

      if ( !dmsAccessAndFlagCompatiblity( context->mb()->_flag,
                                          DMS_ACCESS_TYPE_CRT_DICT ) )
      {
         PD_LOG( PDERROR, "Incompatible collection mode: %d",
                 context->mb()->_flag ) ;
         rc = SDB_DMS_INCOMPATIBLE_MODE ;
         goto error ;
      }

      if ( !OSS_BIT_TEST( context->mb()->_attributes,
                          DMS_MB_ATTR_COMPRESSED ) ||
           UTIL_COMPRESSOR_LZW != context->mb()->_compressorType ||
           DMS_INVALID_EXTENT != context->mb()->_dictExtentID )
      {
         PD_LOG( PDERROR, "Some system error occurs[MBID:%u, Attribute:%u"
                 "CompressorType:%u, DictExtentID:%d]", mbID,
                 context->mb()->_attributes, context->mb()->_compressorType,
                 context->mb()->_dictExtentID ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      rc = _findFreeSpace( pageNum, dictExtID, context ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to allocate space for dictionary "
                   "extent" ) ;

      extRW = extent2RW( dictExtID, context->mbID() ) ;
      extRW.setNothrow( TRUE ) ;
      dictExtent = extRW.writePtr<dmsDictExtent>( 0,
                                                  pageNum <<
                                                  pageSizeSquareRoot() ) ;
      if( !dictExtent )
      {
         PD_LOG( PDERROR, "Get the dict extent[%d] address failed",
                 dictExtent ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      /// Init
      dictExtent->init( pageNum, context->mbID() ) ;
      dictExtent->setDict( dict, dictLen ) ;
      for ( INT32 i = 0; i < 3; i++ )
      {
         rc = flushPages( dictExtID, pageNum, TRUE ) ;
         if ( SDB_OK == rc )
         {
            break ;
         }
      }
      PD_RC_CHECK( rc, PDERROR, "Failed to flush dictionary. It will be "
                   "created and flushed again next time" ) ;

      /*
       * Set the dictionary extent id in mb only after the dictionary has been
       * successfully flushed to disk.
       */
      context->mb()->_dictExtentID = dictExtID ;
      context->mb()->_dictVersion = UTIL_LZW_DICT_VERSION ;

      /// Make sure the dict persist
      flushMME( isSyncDeep() ) ;

      {
         UTIL_COMPRESSOR_TYPE type  = (UTIL_COMPRESSOR_TYPE)
                                   ( context->mb()->_compressorType ) ;
         dmsCompressorGuard guard( compressorEntry, EXCLUSIVE ) ;
         compressorEntry->setCompressor( getCompressorByType( type ) ) ;
         compressorEntry->setDictionary(
            (const utilDictHandle)( beginFixedAddr( dictExtID, pageNum ) +
                                    DMS_DICTEXTENT_HEADER_SZ ) ) ;
      }

   done:
      if ( context )
      {
         releaseMBContext( context ) ;
      }

      PD_TRACE_EXITRC( SDB__DMSSTORAGEDATA_DICTPERSIST, rc ) ;
      return rc ;
   error:
      if ( DMS_INVALID_EXTENT != dictExtID )
      {
         _freeExtent( dictExtID, mbID ) ;
      }
      goto done ;
   }

   INT32 _dmsStorageData::extractData( dmsMBContext *mbContext,
                                       dmsRecordRW &recordRW,
                                       _pmdEDUCB *cb,
                                       dmsRecordData &recordData )
   {
      INT32 rc                = SDB_OK ;
      monAppCB * pMonAppCB    = cb ? cb->getMonAppCB() : NULL ;
      const dmsRecord *pRecord= recordRW.readPtr( 0 ) ;

      recordData.reset() ;

      if ( !mbContext->isMBLock() )
      {
         PD_LOG( PDERROR, "MB Context must be locked" ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      /// if ovf, need to get the ovt's data
      if ( pRecord->isOvf() )
      {
         dmsRecordID ovfRID = pRecord->getOvfRID() ;
         dmsRecordRW ovfRW = record2RW( ovfRID, mbContext->mbID() ) ;
         pRecord = ovfRW.readPtr( 0 ) ;
         SDB_ASSERT( pRecord->isOvt(), "Record must be ovt" ) ;
         DMS_MON_OP_COUNT_INC( pMonAppCB, MON_DATA_READ, 1 ) ;
      }

      recordData.setData( pRecord->getData(), pRecord->getDataLength(),
                          FALSE, TRUE ) ;

      if ( pRecord->isCompressed() )
      {
         const CHAR *pUncompressData = NULL ;
         INT32 unCompressDataLen = 0 ;
         rc = dmsUncompress( cb, &_compressorEntry[ mbContext->mbID() ],
                             pRecord->getData(), pRecord->getDataLength(),
                             &pUncompressData, &unCompressDataLen ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Failed to uncompress data, rc: %d", rc ) ;
            goto error ;
         }
         /// check the length
         if ( unCompressDataLen != *(INT32*)pUncompressData )
         {
            PD_LOG( PDERROR, "Uncompress data length[%d] is not unmatch "
                    "real length[%d]", unCompressDataLen,
                    *(INT32*)pUncompressData ) ;
            rc = SDB_CORRUPTED_RECORD ;
            goto error ;
         }
         recordData.setData( pUncompressData, unCompressDataLen,
                             FALSE, FALSE ) ;
      }
      DMS_MON_OP_COUNT_INC( pMonAppCB, MON_DATA_READ, 1 ) ;
      DMS_MON_OP_COUNT_INC( pMonAppCB, MON_READ, 1 ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   UINT32 _dmsStorageData::_getRecordDataLen( const dmsRecord *pRecord )
   {
      /// if ovf, need to get the ovt's data
      if ( pRecord->isOvf() )
      {
         dmsRecordID ovfRID = pRecord->getOvfRID() ;
         dmsRecordRW ovfRW = record2RW( ovfRID, -1 ) ;
         pRecord = ovfRW.readPtr() ;
      }
      return pRecord->getDataLength() ;
   }

   /*
      Tool Fuctions
   */
   BOOLEAN dmsIsKeyNameValid( const BSONObj &obj,
                              const CHAR **pErrStr )
   {
      const CHAR *pTmpStr = NULL ;
      BOOLEAN valid = TRUE ;

      BSONObjIterator itr( obj ) ;
      while ( itr.more() )
      {
         BSONElement e = itr.next() ;

         if ( '$' == e.fieldName()[ 0 ] )
         {
            pTmpStr = "field name can't start with \'$\'" ;
            valid = FALSE ;
            break ;
         }
         else if ( ossStrchr( e.fieldName(), '.' ) )
         {
            pTmpStr = "field name can't include \'.\'" ;
            valid = FALSE ;
            break ;
         }
         else if ( e.isABSONObj() &&
                   !dmsIsKeyNameValid( e.embeddedObject(), pErrStr ) )
         {
            valid = FALSE ;
            break ;
         }
      }

      if ( !valid && pErrStr && pTmpStr )
      {
         *pErrStr = pTmpStr ;
      }

      return valid ;
   }

   BOOLEAN dmsIsRecordIDValid( const BSONElement &oidEle,
                               BOOLEAN allowEOO,
                               const CHAR **pErrStr )
   {
      const CHAR *pTmpStr = NULL ;
      BOOLEAN valid = TRUE ;

      switch ( oidEle.type() )
      {
         case EOO :
            if ( !allowEOO )
            {
               pTmpStr = "is not exist" ;
               valid = FALSE ;
            }
            break ;
         case Array :
            pTmpStr = "can't be Array" ;
            valid = FALSE ;
            break ;
         case Object :
            valid = dmsIsKeyNameValid( oidEle.embeddedObject(), pErrStr ) ;
            break ;
         default :
            break ;
      }

      if ( !valid && pErrStr && pTmpStr )
      {
         *pErrStr = pTmpStr ;
      }

      return valid ;
   }

}

