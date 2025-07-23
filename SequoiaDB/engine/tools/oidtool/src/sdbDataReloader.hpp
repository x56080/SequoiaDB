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

   Source File Name = sdbDataReloader.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who      Description
   ====== =========== ======== ==============================================
          2022/11/29  TZB      Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef SDB_RELOAD_DATA_HPP_
#define SDB_RELOAD_DATA_HPP_
#include "core.hpp"
#include "ossLatch.hpp"
#include "ossAtomic.hpp"
#include "ossEvent.hpp"
#include "utilCircularQueue.hpp"
#include "sdbOidToolUtil.hpp"
#include "../../../client/client.hpp"
#include <vector>
#include <map>
#include <string>
#include <boost/thread.hpp>

using namespace std ;
using namespace sdbclient ;
using namespace bson ;
 
struct RecordSet ;

/**
 * IController
 */
 class IController
 {
public:
   IController() {}
   virtual ~IController() {}
   virtual void stop() = 0 ;
   virtual BOOLEAN isStop() = 0 ;
 } ;

/**
 * IDataProvider
 */
class IDataProvider
{
public:
   IDataProvider() {}
   virtual ~IDataProvider() {}
   virtual INT32 getRecordSet( RecordSet **ppRecordSet, UINT32 *pWaitTimeMicro = NULL ) = 0 ;
   virtual INT32 releaseRecordSet( RecordSet *pRecordSet ) = 0 ;
} ;

/**
 * ITask
 */
class ITask
{
public:
   ITask() { _errNo = SDB_OK ; }
   virtual ~ITask() {}
   virtual INT32 run() = 0 ;
   virtual const CHAR* name() = 0 ;

public:
   void setErrNo( INT32 errNo ) { _errNo = errNo ; } ;
   INT32 getErrNo() const { return _errNo ; } ;

private:
   INT32   _errNo ;
} ;

/**
 * ReloadContext
 */
class ReloadContext: public IController
{
public:
   ReloadContext()
   : _isStop( 0 )
   {
   }
   ~ReloadContext() {}

   virtual void stop()
   {
      _isStop.inc() ;
   }

   virtual BOOLEAN isStop()
   {
      return _isStop.peek() > 0 ? TRUE : FALSE ;
   }

private:
   ossAtomic32   _isStop ;
} ;

/**
 * RecordSet
 */
struct RecordSet
{
   vector<BSONObj> vecObjs ;
} ;

/**
 * RecordPool
 */
class RecordPool: public ITask, public IDataProvider
{
public:
   typedef _utilCircularBuffer< RecordSet* > RECORD_QUEUE ;

public:
   RecordPool( IController *pController ) ;
   ~RecordPool() ;

public:
   INT32 init() ;
   void fini() ;

public:
   virtual INT32 run() ;
   virtual const CHAR* name() { return "Query Task" ; }

public:
   virtual INT32 getRecordSet( RecordSet **ppRecordSet, UINT32 *pWaitTimeMicro = NULL ) ;
   virtual INT32 releaseRecordSet( RecordSet *pRecordSet ) ;

private:
   BOOLEAN _isQueueEmpty( RECORD_QUEUE &queue ) ;
   BOOLEAN _checkSrcCLStatus() ;
   INT32 _getLsnSum( UINT64 &lsnSum ) ;

private:
   SINT64               _totalRecordNum ; // total records of the cl
   SINT64               _currReadNum ;    // the records we has read from cl
   UINT64               _beginLsnSum ;    // the lsn sum of the src cl before reading records 
   BOOLEAN              _hasReadAll ;     // we has finished reading all the records or not

   InfoOptions*         _pOptions ;
   IController*         _pController ;

   sdb*                 _pConnect ;
   sdbCollection        _cl ;

   ossAutoEvent         _queryEvent ;  // notify pool to query record
   ossAutoEvent         _insertEvent ;  // notify importer to insert record
   RECORD_QUEUE         _queriedQueue ; // for importer to get RecordSet
   RECORD_QUEUE         _insertedQueue ; // for importer to releasse RecordSet
   ossSpinSLatch        _queueMutex ;    // lock both _queriedQueue and _insertedQueue
} ;

/**
 * RecordImporter
 */
class RecordImporter : public ITask
{
public:
   RecordImporter( IController *pController, 
                   IDataProvider *pDataProvider ) ;
   ~RecordImporter() ;

public:
   virtual INT32 run() ;
   virtual const CHAR* name() { return "Import Task" ; }

private:
   INT32 _init() ;

private:
   InfoOptions*      _pOptions ;
   IController*      _pController ;
   IDataProvider*    _pDataProvider ;
   sdb*              _pConnect ;
   sdbCollection     _cl ;
} ;

class SdbDataReloader
{
public:
   SdbDataReloader() ;
   ~SdbDataReloader() ;

public:
   INT32 run( BSONObj &result ) ;

private:
   INT32 _init() ;
   INT32 _run() ;
   void _fini() ;

private:
   InfoOptions*                _pOptions ;
   ReloadContext               _reloadCtx ;
   RecordPool                  _recordPool ;
   vector< RecordImporter* >   _verImprters ;
   vector< boost::thread* >    _vecImprtThds ;
   boost::thread*              _pQueryThd ;
} ;

#endif