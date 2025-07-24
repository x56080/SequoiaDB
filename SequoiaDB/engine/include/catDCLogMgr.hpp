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

   Source File Name = catDCLogMgr.hpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of runtime component. This file contains code logic for
   common functions for coordinator node.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================

   Last Changed =     XJH Opt

*******************************************************************************/
#ifndef CAT_DCLOGMGR_HPP__
#define CAT_DCLOGMGR_HPP__

#include "pmd.hpp"
#include "netDef.hpp"
#include "interface/IDataProtectionService.h"
#include "dpsReplicaLogMgr.hpp"
#include "pmdEDU.hpp"
#include <vector>
#include <string>

using namespace bson ;
using namespace std ;

namespace engine
{
   class sdbCatalogueCB ;
   class _SDB_RTNCB ;
   class _SDB_DMSCB ;

   /*
      _catDCLogItem define
   */
   class _catDCLogItem : public SDBObject
   {
      public:
         _catDCLogItem( UINT32 pos, const string &clname ) ;
         ~_catDCLogItem() ;

         string         toString() const ;

         const CHAR*    getCLName() const { return _clName.c_str() ; }
         UINT32         getPos() const { return _pos ; }
         UINT64         getCount() const ;
         DPS_LSN        getFirstLSN() const ;
         DPS_LSN        getLastLSN() const ;
         DPS_LSN        getComingLSN() const ;
         UINT32         getLID() const ;

         BOOLEAN        isFull() const ;
         BOOLEAN        isEmpty() const ;

         INT32          truncate( _pmdEDUCB *cb ) ;
         INT32          restore( _pmdEDUCB *cb ) ;
         INT32          writeData( BSONObj &obj, const DPS_LSN &lsn,
                                   _pmdEDUCB *cb ) ;
         INT32          readData( const BSONObj &match,
                                  _dpsMessageBlock *mb,
                                  _pmdEDUCB *cb,
                                  const BSONObj &orderby = BSONObj(),
                                  INT64 limit = 1,
                                  INT32 maxTime = -1,
                                  INT32 maxSize = 5242880 ) ;
         INT32          removeDataToLow( DPS_LSN_OFFSET lowOffset,
                                         _pmdEDUCB *cb ) ;

      protected:
         void           _reset() ;
         INT32          _parseLsn( const BSONObj &orderby,
                                   _pmdEDUCB *cb,
                                   DPS_LSN &lsn ) ;
         INT32          _parseMeta( _pmdEDUCB *cb ) ;

      private:
         _SDB_DMSCB                 *_pDmsCB;
         _dpsLogWrapper             *_pDpsCB;
         _SDB_RTNCB                 *_pRtnCB;
         sdbCatalogueCB             *_pCatCB;

         string                     _clName ;
         UINT32                     _pos ;
         UINT32                     _clLID ;

         UINT64                     _count ;
         DPS_LSN                    _first ;
         DPS_LSN                    _last ;
         DPS_LSN                    _coming ;

   } ;
   typedef _catDCLogItem catDCLogItem ;

   /*
      _catDCLogMgr define
   */
   class _catDCLogMgr : public IDataJournal
   {
      public:
         _catDCLogMgr() ;
         ~_catDCLogMgr() ;

         void  attachCB( _pmdEDUCB *cb ) ;
         void  detachCB( _pmdEDUCB *cb ) ;

         INT32 init() ;
         INT32 restore() ;

         INT32 saveSysLog( dpsLogRecordHeader *pHeader,
                           DPS_LSN *pRetLSN = NULL ) ;

      public:
         virtual DPS_LSN getMinFileLSN() override ;
         virtual DPS_LSN getMinBufLSN() override ;
         virtual DPS_LSN getCurrentLSN() override ;
         virtual DPS_LSN getExpectedLSN() override ;
         virtual DPS_LSN getCommittedLSN() override ;

         virtual void getLsnWindow( DPS_LSN &minFileLSN,
                                    DPS_LSN &minBufLSN,
                                    DPS_LSN &currentLSN,
                                    DPS_LSN *expectedLSN,
                                    DPS_LSN *committedLSN ) override ;

         virtual INT32 write( IExecutor *executor,
                              const dpsWriteRequest &request,
                              const dpsWriteOptions &o,
                              dpsLogRecordHeader *result ) override ;

         virtual INT32 search( const DPS_LSN &lsn,
                              const dpsSearchOptions &o,
                              dpsMessageBlock &block )  override ;

         virtual INT32 replicate( const CHAR *rawdata, UINT32 size ) override
         {
            return recordRow(rawdata, size);
         }

         virtual INT32 flush( DPS_LSN_OFFSET offset, BOOLEAN async ) override ;

         virtual INT32 move( const DPS_LSN_OFFSET &lsn,
                             const DPS_LSN_VER &version ) override ;

      public:
         INT32     search( const DPS_LSN &minLsn,
                           _dpsMessageBlock *mb,
                           UINT8 type = DPS_SEARCH_ALL,
                           INT32 maxNum = 1,
                           INT32 maxTime = -1,
                           INT32 maxSize = 5242880 ) ;

         INT32     searchHeader( const DPS_LSN &lsn,
                                 _dpsMessageBlock *mb,
                                 UINT8 type = DPS_SEARCH_ALL ) ;

         DPS_LSN getStartLsn ( BOOLEAN logBufOnly = FALSE );

         DPS_LSN expectLsn();
         DPS_LSN commitLsn();
         DPS_LSN getCurrentLsn();

         void getLsnWindow( DPS_LSN &beginLsn,
                           DPS_LSN &endLsn,
                           DPS_LSN *pExpectLsn,
                           DPS_LSN *committed );  

         INT32 recordRow( const CHAR *row, UINT32 len );

      protected:
         UINT32 _incFileID( UINT32 fileID ) ;
         UINT32 _decFileID( UINT32 fileID ) ;
         void   _setVersion( DPS_LSN_VER version ) ;

         // caller must hold the _latch
         INT32  _writeData( BSONObj &obj, const DPS_LSN &lsn ) ;
         INT32  _readData( const BSONObj &match,
                           catDCLogItem *pLog,
                           _dpsMessageBlock *mb,
                           const BSONObj &orderby = BSONObj(),
                           INT64 limit = 1,
                           INT32 maxTime = -1,
                           INT32 maxSize = 5242880 ) ;

         DPS_LSN   _getStartLsn() ;

         INT32     _filterLog( dpsLogRecordHeader *pHeader,
                               BOOLEAN &valid ) ;

      private:
         _pmdEDUCB                  *_pEduCB;
         vector< catDCLogItem* >    _vecLogCL ;

         UINT32                     _begin ;
         UINT32                     _work ;

         DPS_LSN                    _curLsn ;
         DPS_LSN                    _expectLsn ;
         ossSpinSLatch              _latch ;

   } ;
   typedef _catDCLogMgr catDCLogMgr ;


}

#endif // CAT_DCLOGMGR_HPP__

