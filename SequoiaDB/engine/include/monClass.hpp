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

   Source File Name = monClass.hpp

   Descriptive Name = monitor Service Header

   When/how to use: this program may be used on binary and text-formatted
   versions of OSS component. This file contains functions for OSS operations.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          05/24/2019  CW  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef MONCLASS_HPP_
#define MONCLASS_HPP_

#include "monLatch.hpp"
#include "dpsTransLockDef.hpp"
#include "utilPooledObject.hpp"
#include "ossTypes.h"
#include "monLatch.hpp"
#include "ossAtomic.hpp"
#include "ossUtil.hpp"
#include "utilList.hpp"
#include "msg.h"
#include "pd.hpp"
#include "../bson/bson.hpp"
#include <boost/intrusive/list.hpp>
#include <iterator>
#include "monLatch.hpp"
#include "sdbInterface.hpp"
#include "pmdDef.hpp"

using namespace bson ;

namespace engine
{

class _monAppCB ;

#define MONQUERY_SET_NAME(edu, n)\
  if (edu->getMonQueryCB()) \
  { \
     try \
     { \
        edu->getMonQueryCB()->name.assign(n); \
     } \
     catch ( std::exception &e ) \
     { \
        PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ; \
     } \
  }\

#define MONQUERY_SET_QUERY_TEXT(edu, n)\
  if (edu->getMonQueryCB() && \
      edu->getMonQueryCB()->dataLvl == MON_DATA_LVL_DETAIL && \
      edu->getMonQueryCB()->queryText.empty() )\
  {\
     try \
     { \
        edu->getMonQueryCB()->queryText.assign(n); \
     } \
     catch( std::exception &e ) \
     { \
        PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ; \
     } \
  }\

#define MONQUERY_REPLACE_QUERY_TEXT(edu, n)\
    if (edu->getMonQueryCB() && \
        edu->getMonQueryCB()->dataLvl == MON_DATA_LVL_DETAIL )\
    {\
       try \
       { \
          edu->getMonQueryCB()->queryText.assign(n); \
       } \
       catch( std::exception &e ) \
       { \
          PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ; \
       } \
    }\


typedef enum
{
   MON_CLASS_QUERY = 0,  // _monClassQuery
   MON_CLASS_LATCH,      // _monClassLatch
   MON_CLASS_LOCK,       // _monClassLock
   MON_CLASS_MAX
} MON_CLASS_TYPE ;

// Status for _monClass
#define MON_CLASS_STATUS_NORMAL   0x00
#define MON_CLASS_STATUS_PEND_DEL 0x01
#define MON_CLASS_STATUS_PEND_ARC 0x02

// Mon group mask
#define MON_GROUP_MASK_DEFAULT 0
#define MON_GROUP_QUERY_BASIC  0x00000001
#define MON_GROUP_QUERY_DETAIL 0x00000002
#define MON_GROUP_LATCH_BASIC  0x00000004
#define MON_GROUP_LATCH_DETAIL 0x00000008
#define MON_GROUP_LOCK_BASIC   0x00000010
#define MON_GROUP_LOCK_DETAIL  0x00000020

// monitor data capture level
typedef enum
{
   MON_DATA_LVL_NONE = 0,
   MON_DATA_LVL_BASIC,
   MON_DATA_LVL_DETAIL
} MON_DATA_LEVEL ;

MON_DATA_LEVEL monGroupMaskToLevle( UINT32 groupMask, MON_CLASS_TYPE classType ) ;

UINT32 monGetGroupMask() ;
void   monUpdateGroupMask( UINT32 groupMask ) ;

UINT32 monGetCurGroupMask() ;
void   monUpdateCurGroupMask( UINT32 groupMask ) ;

UINT32 monGetOptiLevel() ;
UINT32 monGetSlowLatchThreshold() ;
UINT32 monGetSlowLockThreshold() ;
UINT32 monGetSlowQueryThreshold() ;
UINT32 monGetHistExpiredTime() ;

void   monUpdateConf( UINT32 queryThreshold,
                      UINT32 latchThreshold,
                      UINT32 lockThreshold,
                      UINT32 optiLevel,
                      UINT32 histExpiredTime ) ;

struct _monClassBaseData
{
} ;

struct _monClassQueryTimeInfo : public _monClassBaseData
{
   ossTick     _msgRecvTime ;
   ossTick     _beginTime ;

   _monClassQueryTimeInfo()
   {
   }

   _monClassQueryTimeInfo( UINT64 msgRecvTime, UINT64 beginTime )
   {
      ossTickConversionFactor cFactor ;

      if ( 0 == msgRecvTime || msgRecvTime > beginTime )
      {
         msgRecvTime = beginTime ;
      }
      _msgRecvTime.initFromTimeValue( cFactor, msgRecvTime ) ;
      _beginTime.initFromTimeValue( cFactor, beginTime ) ;
   }
} ;

typedef _monClassQueryTimeInfo monClassQueryTimeInfo ;

// Data structure used when constructing a monClassLatch
struct _monClassLatchData : public _monClassBaseData
{
   ossTick startTick ;
   MON_LATCH_IDENTIFIER latchID ;
   UINT32 waiterTID ;
   UINT32 ownerTID ;
   INT16  waiterType ;
   INT16  ownerType ;
   INT16  ownerMode ;
   INT16  latchMode ;
   UINT32 numOwner ;
   void*  latchAddr ;
   MsgQueryID queryID ;

   _monClassLatchData( MON_LATCH_IDENTIFIER id = MON_LATCH_ID_MAX,
                       void *pAddr = NULL )
      : latchID( id ),
        waiterTID( 0 ), ownerTID( 0 ),
        waiterType( -1 ), ownerType( -1 ),
        ownerMode( -1 ), latchMode( -1 ), numOwner( 0 ),
        latchAddr( pAddr )
   {}

   void init( IExecutor *pExe, INT16 latchMode, const ossTick *pStartTick = NULL )
   {
      if ( pExe )
      {
         waiterTID = pExe->getTID() ;
         waiterType = pExe->getType() ;

         ISession *pSession = pExe->getSession() ;
         if ( pSession && pSession->getOperator() )
         {
            queryID = pSession->getOperator()->getGlobalID().getQueryID() ;
         }
      }

      this->latchMode = latchMode ;

      if ( pStartTick && (BOOLEAN)(*pStartTick) )
      {
         startTick = *pStartTick ;
      }
      else
      {
         startTick.sample() ;
      }
   }

   void setOwner( UINT32 tid, INT16 type, UINT32 num, INT16 mode )
   {
      ownerTID = tid ;
      ownerType = type ;
      numOwner = num ;
      ownerMode = mode ;
   }

} ;

typedef _monClassLatchData monClassLatchData ;

typedef boost::intrusive::list_base_hook< > BaseHook ;

/**
 * _monClass - Parent class for all monitor class
 *
 * It is up to the user to define the synchronization strategy for each
 * _monClass. For example, if a metric can be concurrently updated
 * by more than one thread, then the user might want to
 * define that metric as atomic
 */
class _monClass : public BaseHook, public utilPooledObject
{
   friend class _monClassContainer ;

protected:
   ossTimestamp    _endTS ;      /**! end timestamp for this object */
   ossTick  _createTSTick ;      /**! create tick for this object */
   UINT16         _status ;      /**! object status */

   MON_CLASS_TYPE _type ;        /**! object type */

public:
   //TODO retrieve this directly from container
   MON_DATA_LEVEL    dataLvl ;      /**! monitor data collection level */

   _monClass( const ossTick *pStartTick = NULL ) ;
   virtual ~_monClass() ;

   /**
    * getType - return the type of the object
    */
   MON_CLASS_TYPE getType () const { return _type; }

   /**
    * setPendingDelete - mark an object to be pending delete
    */
   void setPendingDelete() { _status |= MON_CLASS_STATUS_PEND_DEL ; }
   BOOLEAN isPendingDelete() const
   {
      return ( _status & MON_CLASS_STATUS_PEND_DEL ) ? TRUE : FALSE ;
   }

   /**
    * setPendingArchive - mark an object to be pending archive
    */
   void setPendingArchive() { _status |= MON_CLASS_STATUS_PEND_ARC ; }
   BOOLEAN isPendingArchive() const
   {
      return ( _status & MON_CLASS_STATUS_PEND_ARC ) ? TRUE : FALSE ;
   }

   void discard()
   {
      if ( MON_CLASS_STATUS_NORMAL == _status && _pPendingDelete )
      {
         setPendingDelete() ;
         _pPendingDelete->inc() ;
         _pPendingDelete = NULL ;
      }
   }

   /**
    * getStatus() - get the current status
    */
   UINT16 getStatus() const { return _status ; }

   /**
    * Get the create timestamp
    */
   ossTimestamp getCreateTS() const
   {
      ossTimestamp ts ;
      _createTSTick.convertToTimestamp( ts ) ;
      return ts ;
   }

   /**
    * Get the end timestamp
    */
   ossTimestamp &getEndTS() { return _endTS ; }

   /**
    * Get the end timestamp
    */
   ossTimestamp getEndTSConst() const { return _endTS ; }

   /**
    * Get the end time in ms
    */
   UINT64 getEndTime() const { return (UINT64)_endTS.time * 1000L + _endTS.microtm / 1000 ; }

   /**
    * Get the create timestamp tick
    */
   ossTick getCreateTSTick() const { return _createTSTick ; }

   /**
    * Format object into a BSON
    */
   virtual void dump( BSONObj &obj ) = 0 ;

   /**
    * Reset metrics
    */
   virtual void reset() = 0 ;

   /**
    * Done for tmp
    */
   virtual void done() = 0 ;

private:
   ossAtomic32       *_pPendingDelete ;
} ;

typedef _monClass monClass ;

/**
 * Template to ensure all subclasses implement the getType function
 */
template<class T>
class monClassTemplate : public _monClass
{
protected:
   monClassTemplate( const ossTick *pStartTick = NULL )
   :_monClass( pStartTick )
   {}

public:
   ~monClassTemplate()
   {
      T::getType();
   }
} ;

/**
 * archive query information based on response time
 */
BOOLEAN monArchiveQuery ( _monClass *obj ) ;

/**
 * archive latch information based on wait time
 */
BOOLEAN monArchiveLatch ( _monClass *obj ) ;

BOOLEAN monArchiveLock ( _monClass *obj ) ;

/**
 * Default function pointer to indicate no archive
 */
BOOLEAN monNoArchive ( _monClass *obj ) ;

typedef BOOLEAN (*archiveFunc)(_monClass *) ;

typedef enum
{
   MON_CLASS_ACTIVE_LIST,
   MON_CLASS_ARCHIVED_LIST
} MON_CLASS_LIST_TYPE ;

/*
 * Structure to help compute the delta from monAppCB
 */
struct _monClassQueryTmpData
{
   UINT64           dataRead ;
   UINT64           indexRead ;
   UINT64           dataWrite ;
   UINT64           indexWrite ;
   UINT64           lobRead ;
   UINT64           lobWrite ;
   UINT64           lobTruncate ;
   UINT64           lobAddressing ;

   _monClassQueryTmpData()
      : dataRead(0),
        indexRead(0),
        dataWrite(0),
        indexWrite(0),
        lobRead(0),
        lobWrite(0),
        lobTruncate(0),
        lobAddressing(0)
   {}

   _monClassQueryTmpData& operator=(const _monAppCB& cb) ;

   void diff(_monAppCB &cb) ;
} ;
typedef _monClassQueryTmpData monClassQueryTmpData ;

enum MON_QUERY_TICK_TYPE
{
   MON_TICK_NONE  = 0,
   MON_TICK_LATCH,
   MON_TICK_LOCK,
   MON_TICK_FILE,
   MON_TICK_LOG,
   MON_TICK_CATA,
   MON_TICK_BLOCK,
   MON_TICK_SYNCWAIT,
   MON_TICK_SORT,

   MON_TICK_MAX,
   MON_TICK_REF
} ;

#define MON_QUERY_TICK_SZ              ( 14 )

/**
 * Capture metrics about a query
 */
class _monClassQuery : public monClassTemplate<_monClassQuery>
{
public:
   UINT32                 tid ;  /**! TID of the EDU */
   BSONObj         clientInfo ;  /**! Information of Client which connects to engine */
   UINT32           clientTID ;  /**! OS TID of Client which connects to engine */
   ossPoolString   clientHost ;  /**! Client IP */
   ossPoolString         name ;  /**! The target object name of this query */
   SINT64        accessPlanID ;  /**! Access plan ID used by the query */
   UINT32            hashCode ;  /**! Access plan hash code */
   UINT32              opCode ;  /**! Message opCode */
   UINT32           sessionID ;  /**! EDU Session ID */
   ossTickDelta   processTime ;  /**! Process time, not include the network cost*/
   ossTickDelta  responseTime ;  /**! Response time of the query */
   ossTickDelta  dispatchTime ;  /**! Dispatch time */
   ossTickDelta latchWaitTime ;  /**! Time spent on latch wait */
   ossTickDelta  lockWaitTime ;  /**! Time spent on lock wait */
   ossTickDelta queryCataTime ;  /**! Time spent on query with catalog */
   ossTickDelta     blockTime ;  /**! Time spent on some blocking */
   ossTickDelta      fileTime ;  /**! Time spent on file operation */
   ossTickDelta       logTime ;  /**! Time spent on log operation */
   ossTickDelta      sortTime ;  /**! Time spent on sort operation */
   UINT32            dataRead ;  /**! Total data read (record)*/
   UINT32           indexRead ;  /**! Total index read (record)*/
   UINT32           dataWrite ;  /**! Total data write (record) */
   UINT32          indexWrite ;  /**! Total index write (record) */
   UINT32             lobRead ;  /**! Total LOB read (number of times) */
   UINT32            lobWrite ;  /**! Total LOB write (number of times) */
   UINT32         lobTruncate ;  /**! Total LOB truncate (number of times) */
   UINT32       lobAddressing ;  /**! Total LOB addressing (number of times) */
   UINT32        rowsReturned ;  /**! Total number of rows returned */
   UINT32          numMsgSent ;  /**! Total # of msgs sent to remote nodes */
   UINT32         numMsgReply ;  /**! Total # of msgs reply to source node */
   UINT32        numQueryCata ;  /**! Total # of query catalog */
   UINT32        numLatchWait ;  /**! Total # of latch wait */
   UINT32         numLockWait ;  /**! Total # of lock wait */
   ossPoolSet<UINT32>   nodes ;  /**! Node ID where messages were sent to */
   ossPoolSet<UINT32>  blocks ;  /**! Block types */
   MsgRouteID      relatedNID ;  /**! coordinator node node ID */
   UINT32          relatedTID ;  /**! coordinator node edu TID */
   BOOLEAN    anchorToContext ;  /**! Whether this obj anchored to a context */
   ossPoolString    queryText ;  /**! Full query text */
   ossTickDelta remoteNodesResponseTime ; /*! Time spent waiting remote nodes */
   ossTickDelta   msgSentTime ;  /**! Time spent sending msgs to remote nodes */
   ossTickDelta  syncWaitTime ;  /**! Time spent waiting repl-node sync*/
   MsgQueryID         queryID ;  /**! The id of a query statement */

   _monClassQuery ()
   {
      tid = 0 ;
      reset() ;
      _type = MON_CLASS_QUERY ;
   }

   ~_monClassQuery()
   {
#ifdef _DEBUG
      SDB_ASSERT( 0 == _queryRef, "Query reference must be zero" ) ;
#endif // _DEBUG
   }

   _monClassQuery( _monClassBaseData *data )
   :monClassTemplate<_monClassQuery>( &( ((monClassQueryTimeInfo*)data)->_msgRecvTime ) )
   {
      monClassQueryTimeInfo *pTimeInfo = ( monClassQueryTimeInfo* )data ;

      tid = 0 ;
      reset() ;
      _type = MON_CLASS_QUERY ;
      dispatchTime = pTimeInfo->_beginTime - pTimeInfo->_msgRecvTime ;
   }

   _monClassQuery ( const _monClassQuery &monClassQuery)
     : tid( monClassQuery.tid ),
       clientTID( monClassQuery.clientTID ),
       accessPlanID( monClassQuery.accessPlanID),
       hashCode( monClassQuery.hashCode ),
       opCode( monClassQuery.opCode ),
       sessionID( monClassQuery.sessionID ),
       processTime( monClassQuery.processTime ),
       responseTime( monClassQuery.responseTime ),
       dispatchTime( monClassQuery.dispatchTime ),
       latchWaitTime( monClassQuery.latchWaitTime ),
       lockWaitTime( monClassQuery.lockWaitTime ),
       queryCataTime( monClassQuery.queryCataTime ),
       blockTime( monClassQuery.blockTime ),
       fileTime( monClassQuery.fileTime ),
       logTime( monClassQuery.logTime ),
       sortTime( monClassQuery.sortTime ),
       dataRead( monClassQuery.dataRead ),
       indexRead( monClassQuery.indexRead ),
       dataWrite( monClassQuery.dataWrite ),
       indexWrite( monClassQuery.indexWrite ),
       lobRead( monClassQuery.lobRead ),
       lobWrite( monClassQuery.lobWrite ),
       lobTruncate( monClassQuery.lobTruncate ),
       lobAddressing( monClassQuery.lobAddressing ),
       rowsReturned( monClassQuery.rowsReturned ),
       numMsgSent( monClassQuery.numMsgSent ),
       numMsgReply( monClassQuery.numMsgReply ),
       numQueryCata( monClassQuery.numQueryCata ),
       numLatchWait( monClassQuery.numLatchWait ),
       numLockWait( monClassQuery.numLockWait ),
       relatedNID( monClassQuery.relatedNID ),
       relatedTID( monClassQuery.relatedTID ),
       anchorToContext( monClassQuery.anchorToContext ),
       remoteNodesResponseTime( monClassQuery.remoteNodesResponseTime ),
       msgSentTime( monClassQuery.msgSentTime ),
       syncWaitTime( monClassQuery.syncWaitTime )
   {
      _endTS =  monClassQuery.getEndTSConst() ;
      _createTSTick = monClassQuery.getCreateTSTick() ;
      _status =  monClassQuery.getStatus() ;
      clientInfo = monClassQuery.clientInfo.getOwned() ;
      clientHost.assign( monClassQuery.clientHost ) ;
      name.assign( monClassQuery.name ) ;
      nodes = monClassQuery.nodes ;
      blocks = monClassQuery.blocks ;
      queryText.assign( monClassQuery.queryText ) ;
      _type = MON_CLASS_QUERY ;
      queryID = monClassQuery.queryID ;

      for ( UINT32 i = 0 ; i < MON_QUERY_TICK_SZ ; ++i )
      {
         _queryTickType[i] = MON_TICK_NONE ;
      }
      _queryTickPos = MON_QUERY_TICK_SZ - 1 ;
      _queryTickLen = 0 ;

#ifdef _DEBUG
      _queryRef = 0 ;
#endif // _DEBUG
   }

   static MON_CLASS_TYPE getType () { return MON_CLASS_QUERY ; }

   //TODO: to be implemented
   virtual void dump( BSONObj &obj ) {}

   virtual void reset()
   {
      clientTID = 0 ;
      accessPlanID = -1 ;
      hashCode = 0 ;
      opCode = 0 ;
      sessionID = 0 ;
      dataRead = 0 ;
      indexRead = 0 ;
      dataWrite = 0 ;
      indexWrite = 0 ;
      lobRead = 0 ;
      lobWrite = 0 ;
      lobTruncate = 0 ;
      lobAddressing = 0 ;
      rowsReturned = 0 ;
      numMsgSent = 0 ;
      numMsgReply = 0 ;
      numQueryCata = 0 ;
      numLatchWait = 0 ;
      numLockWait = 0 ;
      relatedTID = 0 ;
      anchorToContext = FALSE ;
      relatedNID.value = 0 ;

      for ( UINT32 i = 0 ; i < MON_QUERY_TICK_SZ ; ++i )
      {
         _queryTickType[i] = MON_TICK_NONE ;
      }
      _queryTickPos = MON_QUERY_TICK_SZ - 1 ;
      _queryTickLen = 0 ;

#ifdef _DEBUG
      _queryRef = 0 ;
#endif // _DEBUG
   }

   virtual void done()
   {
      if ( MON_CLASS_STATUS_NORMAL == _status && 0 == responseTime.toUINT64() )
      {
         ossTick endTick ;
         endTick.sample() ;
         responseTime = endTick - _createTSTick ;
         if ( 0 == processTime.toUINT64() )
         {
            processTime = responseTime ;
         }
      }
   }

   void insertNode( UINT32 nodeID )
   {
      try
      {
         nodes.insert( nodeID ) ;
      }
      catch( std::exception &e )
      {
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
      }
   }

   void startBlockTimer( UINT32 blockType )
   {
      startQueryTick( _blockType2TickType( blockType ) ) ;

      if ( EDU_BLOCK_WAITREPLY != blockType &&
           EDU_BLOCK_SYNCWAIT != blockType )
      {
         try
         {
            blocks.insert( blockType ) ;
         }
         catch( std::exception &e )
         {
            PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         }
      }
   }

   void stopBlockTimer()
   {
      stopQueryTick() ;
   }

   void startCataQueryTimer()
   {
      startQueryTick( MON_TICK_CATA ) ;
   }

   void stopCataQueryTimer( BOOLEAN incTime )
   {
      stopQueryTick() ;

      if ( incTime )
      {
         numQueryCata++ ;
      }
   }

   void startQueryTick( INT8 tickType,
                        BOOLEAN submitLast = TRUE,
                        const ossTick *pStartTick = NULL )
   {
      ossTick tick ;

#ifdef _DEBUG
      ++_queryRef ;
#endif //_DEBUG

      if ( pStartTick )
      {
         tick = *pStartTick ;
      }
      else if ( _hasTickType() || _isMonTickValid( tickType ) )
      {
         tick.sample() ;
      }

      if ( _hasTickType() )
      {
         INT8 curTickType = _getCurTickType() ;
         /// submit
         if ( submitLast )
         {
            ossTickDelta delta = tick - _queryTick ;
            _submitQueryTick( curTickType, delta ) ;
         }
      }

      /// push tick type
      if ( _pushTickType( tickType ) )
      {
         _queryTick = tick ;
      }
      else
      {
         SDB_ASSERT( FALSE, "Tick type que is full" ) ;
      }
   }

   void stopQueryTick( const ossTick *pStartTick = NULL, BOOLEAN submit = TRUE )
   {
      INT8 lastTick = MON_TICK_NONE ;

      if ( _popTickType( lastTick ) )
      {
         ossTick tick ;

         if ( pStartTick )
         {
            tick = *pStartTick ;
         }
         else if ( _hasTickType() || _isMonTickValid( lastTick ) )
         {
            tick.sample() ;
         }

         if ( submit )
         {
            _submitQueryTick( lastTick, tick - _queryTick ) ;
         }

         /// set query tick
         _queryTick = tick ;
      }

#ifdef _DEBUG
      --_queryRef ;
#endif //_DEBUG
   }

   void directSubmitQueryTick( INT8 tickType, const ossTickDelta &delta )
   {
      _submitQueryTick( tickType, delta ) ;
      if ( _hasTickType() )
      {
         _queryTick = _queryTick + delta ;
      }
   }

   void incMetrics( const monClassQueryTmpData &tmpData )
   {
      dataRead += tmpData.dataRead ;
      dataWrite += tmpData.dataWrite ;
      indexRead += tmpData.indexRead ;
      indexWrite += tmpData.indexWrite ;
      lobRead += tmpData.lobRead ;
      lobWrite += tmpData.lobWrite ;
      lobTruncate += tmpData.lobTruncate ;
      lobAddressing += tmpData.lobAddressing ;
   }

   void init( INT32 opCode, IExecutor *pExe, MsgHeader *pMsg )
   {
      this->opCode = opCode ;

      if ( pExe )
      {
         sessionID = pExe->getID() ;
         tid = pExe->getTID() ;

         ISession *pSession = pExe->getSession() ;
         if ( pSession )
         {
            relatedTID = pSession->identifyTID() ;
            relatedNID = pSession->identifyNID() ;

            IClient *pClient = pSession->getClient() ;
            if ( pClient )
            {
               try
               {
                  clientHost.assign( pClient->getFromIPAddr() ) ;
               }
               catch( std::exception &e )
               {
                  PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
               }
            }
         }
      }

      if ( pMsg )
      {
         clientTID = pMsg->TID ;
         queryID = pMsg->globalID.getQueryID() ;
      }
   }

   BOOLEAN isCommand() const
   {
      /// except get count
      if ( MSG_BS_QUERY_REQ == opCode && !name.empty() && '$' == name.at( 0 ) &&
           0 != ossStrcmp( CMD_NAME_GET_COUNT, name.c_str() + 1 ) )
      {
         return TRUE ;
      }
      return FALSE ;
   }

private:

   BOOLEAN _submitQueryTick( INT8 tickType, const ossTickDelta &delta )
   {
      switch( tickType )
      {
         case MON_TICK_CATA :
            queryCataTime += delta ;
            break ;
         case MON_TICK_LATCH :
            latchWaitTime += delta ;
            numLatchWait++ ;
            break ;
         case MON_TICK_LOCK :
            lockWaitTime += delta ;
            numLockWait++ ;
            break ;
         case MON_TICK_FILE :
            fileTime += delta ;
            break ;
         case MON_TICK_LOG :
            logTime += delta ;
            break ;
         case MON_TICK_SORT :
            sortTime += delta ;
            break ;
         case MON_TICK_BLOCK :
            blockTime += delta ;
            break ;
         case MON_TICK_SYNCWAIT :
            syncWaitTime += delta ;
            break ;
         default :
            return FALSE ;
      }
      return TRUE ;
   }

   BOOLEAN _pushTickType( INT8 tickType )
   {
      BOOLEAN hasPush = FALSE ;
      INT8 curTickType = _getCurTickType() ;

      if ( tickType > MON_TICK_MAX )
      {
         /// can not allow user push tick type with MON_TICK_REF
         tickType = MON_TICK_MAX ; 
      }

      /// when same tickType, use ref
      if ( _isMonTickValid( curTickType ) && curTickType == tickType )
      {
         INT16 *pRef = _getOrMakeTickRef( TRUE ) ;
         if ( pRef )
         {
            ++(*pRef) ;
            hasPush = TRUE ;
            SDB_ASSERT( *pRef > 0, "Ref is invalid" ) ;
         }
      }

      if ( !hasPush && _queryTickLen < MON_QUERY_TICK_SZ )
      {
         _queryTickPos = ( _queryTickPos + 1 ) % MON_QUERY_TICK_SZ ;
         _queryTickType[ _queryTickPos ] = tickType ;
         ++_queryTickLen ;
         hasPush = TRUE ;
      }

      return hasPush ;
   }

   BOOLEAN _popTickType( INT8 &tickType )
   {
      BOOLEAN hasPop = FALSE ;
      INT16 *pRef = _getOrMakeTickRef( FALSE ) ;

      if ( pRef )
      {
         SDB_ASSERT( *pRef > 0, "Ref is invalid" ) ;

         if ( *pRef > 0 )
         {
            --(*pRef) ;
            tickType = _getCurTickType() ;
            hasPop = TRUE ;
         }

         if ( *pRef <= 0 )
         {
            _popRef() ;
         }
      }

      if ( !hasPop && _queryTickLen > 0 )
      {
         tickType = _queryTickType[ _queryTickPos ] ;
         if ( 0 == _queryTickPos )
         {
            _queryTickPos = MON_QUERY_TICK_SZ - 1 ;
         }
         else
         {
            --_queryTickPos ;
         }
         --_queryTickLen ;
         hasPop = TRUE ;
      }

      return hasPop ;
   }

   INT16* _getOrMakeTickRef( BOOLEAN allowMake )
   {
      INT16 *pRef = NULL ;

      if ( _queryTickLen > 0 )
      {
         /// get
         if ( MON_TICK_REF == _queryTickType[ _queryTickPos ] &&
              _queryTickLen > (INT8)( sizeof(INT16) + 1 ) )
         {
            pRef = (INT16*)(&_queryTickType[ _queryTickPos-sizeof(INT16) ]) ;
         }
         /// create
         else if ( allowMake && _queryTickLen + sizeof(INT16) < MON_QUERY_TICK_SZ )
         {
            _queryTickPos = ( _queryTickPos + sizeof(INT16) + 1 ) % MON_QUERY_TICK_SZ ;
            _queryTickType[ _queryTickPos ] = MON_TICK_REF ;
            _queryTickLen += ( sizeof(INT16) + 1 ) ;

            pRef = (INT16*)(&_queryTickType[ _queryTickPos-sizeof(INT16) ]) ;
            *pRef = 0 ;
         }
      }

      return pRef ;
   }

   BOOLEAN _popRef()
   {
      if ( _queryTickLen > (INT8)( sizeof(INT16) + 1 ) &&
           MON_TICK_REF == _queryTickType[ _queryTickPos ] )
      {
         _queryTickPos -= ( sizeof( INT16 ) + 1 ) ;
         _queryTickLen -= ( sizeof( INT16 ) + 1 ) ;
         return TRUE ;
      }
      return FALSE ;
   }

   INT8 _getCurTickType() const
   {
      if ( _queryTickLen > 0 )
      {
         if ( MON_TICK_REF == _queryTickType[ _queryTickPos ] &&
              _queryTickLen > (INT8)( sizeof(INT16) + 1 ) )
         {
            return _queryTickType[ _queryTickPos - sizeof(INT16) - 1 ] ;
         }
         return _queryTickType[ _queryTickPos ] ;
      }
      return MON_TICK_NONE ;
   }

   BOOLEAN _hasTickType() const { return _queryTickLen > 0 ? TRUE : FALSE ; }

   BOOLEAN _isMonTickValid( INT8 tickType ) const
   {
      return ( tickType > MON_TICK_NONE && tickType < MON_TICK_MAX ) ? TRUE : FALSE ;
   }

   INT8    _blockType2TickType( INT32 blockType ) const
   {
      if ( EDU_BLOCK_SYNCWAIT == blockType )
      {
         return MON_TICK_SYNCWAIT ;
      }
      else if ( EDU_BLOCK_WAITREPLY != blockType )
      {
         return MON_TICK_BLOCK ;
      }
      return MON_TICK_NONE ;
   }

private:
   ossTick _queryTick ;
   INT8    _queryTickType[ MON_QUERY_TICK_SZ ] ;
   INT8    _queryTickPos ;
   INT8    _queryTickLen ;

#ifdef _DEBUG
   INT32   _queryRef ;
#endif // _DEBUG

} ;

typedef _monClassQuery monClassQuery ;

class _monClassLatch : public monClassTemplate<_monClassLatch>
{
public:
   _monClassLatch ()
      : latchID( MON_LATCH_ID_MAX ),
        ownerTID( 0 ), waiterTID( 0 ),
        ownerType( -1 ), waiterType( -1 ),
        ownerMode( -1 ), latchMode( -1 ),
        numOwner( 0 ),
        latchAddr( NULL )
   {
      _type = MON_CLASS_LATCH ;
   }

   _monClassLatch (_monClassBaseData *data)
   :monClassTemplate<_monClassLatch>( &( ((_monClassLatchData*)data)->startTick ) )
   {
      _monClassLatchData *latchData = (_monClassLatchData *) data ;
      latchID = latchData->latchID ;
      ownerTID = latchData->ownerTID ;
      waiterTID = latchData->waiterTID ;
      ownerType = latchData->ownerType ;
      waiterType = latchData->waiterType ;
      ownerMode = latchData->ownerMode ;
      latchMode = latchData->latchMode ;
      numOwner = latchData->numOwner ;
      latchAddr = latchData->latchAddr ;
      queryID = latchData->queryID ;

      _type = MON_CLASS_LATCH ;
   }

   _monClassLatch( const _monClassLatch & monClassLatch )
      : waitTime( monClassLatch.waitTime ),
        latchID ( monClassLatch.latchID ),
        ownerTID ( monClassLatch.ownerTID ),
        waiterTID ( monClassLatch.waiterTID ),
        ownerType ( monClassLatch.ownerType ),
        waiterType ( monClassLatch.waiterType ),
        ownerMode ( monClassLatch.ownerMode ),
        latchMode ( monClassLatch.latchMode ),
        numOwner ( monClassLatch.numOwner ),
        latchAddr ( monClassLatch.latchAddr ),
        queryID( monClassLatch.queryID )
   {
      _endTS = monClassLatch.getEndTSConst() ;
      _createTSTick = monClassLatch.getCreateTSTick() ;
      _status = monClassLatch.getStatus() ;
      _type = MON_CLASS_LATCH ;
   }

   static MON_CLASS_TYPE getType () { return MON_CLASS_LATCH ; }

   virtual void dump( BSONObj &obj ) {}

   virtual void reset() {}

   virtual void done()
   {
      if ( MON_CLASS_STATUS_NORMAL == _status && 0 == waitTime.toUINT64() )
      {
         ossTick endTick ;
         endTick.sample() ;
         waitTime = endTick - _createTSTick ;
      }
   }

   ossTickDelta waitTime ;
   MON_LATCH_IDENTIFIER latchID ;
   UINT32 ownerTID ;
   UINT32 waiterTID ;
   INT16  ownerType ;
   INT16  waiterType ;
   INT16  ownerMode ;
   INT16  latchMode ;
   UINT32 numOwner ;
   void  *latchAddr ;
   MsgQueryID queryID ;
} ;

typedef _monClassLatch monClassLatch ;

class _monClassLock : public monClassTemplate<_monClassLock>
{
public:
   _monClassLock ()
   {
      _type = MON_CLASS_LOCK ;
      reset() ;
   }

   _monClassLock ( const _monClassLock & monClassLock)
      : waitTime( monClassLock.waitTime ),
        lockID( monClassLock.lockID ),
        ownerTID( monClassLock.ownerTID ),
        waiterTID( monClassLock.waiterTID ),
        ownerType( monClassLock.ownerType ),
        waiterType( monClassLock.waiterType ),
        ownerMode( monClassLock.ownerMode ),
        lockMode( monClassLock.lockMode ),
        numOwner( monClassLock.numOwner ),
        queryID( monClassLock.queryID )
   {
      _endTS = monClassLock.getEndTSConst() ;
      _createTSTick = monClassLock.getCreateTSTick() ;
      _status = monClassLock.getStatus() ;
      _type = MON_CLASS_LOCK ;
   }

   void init( IExecutor *pExe, const ossTick *pStartTick = NULL )
   {
      reset() ;

      if ( pExe )
      {
         waiterTID = pExe->getTID() ;
         waiterType = pExe->getType() ;

         ISession *pSession = pExe->getSession() ;
         if ( pSession && pSession->getOperator() )
         {
            queryID = pSession->getOperator()->getGlobalID().getQueryID() ;
         }
      }

      if ( pStartTick && (BOOLEAN)(*pStartTick) )
      {
         _createTSTick = *pStartTick ;
      }
      else
      {
         _createTSTick.sample() ;
      }
   }

   static MON_CLASS_TYPE getType () { return MON_CLASS_LOCK ; }

   virtual void dump( BSONObj &obj ) {}

   virtual void reset()
   {
      waitTime.clear() ;
      lockID.reset() ;
      ownerTID = 0 ;
      waiterTID = 0 ;
      ownerType = -1 ;
      waiterType = -1 ;
      ownerMode = DPS_TRANSLOCK_MAX ;
      lockMode = DPS_TRANSLOCK_MAX ;
      numOwner = 0 ;
      queryID = MsgQueryID() ;
   }

   virtual void done()
   {
      if ( MON_CLASS_STATUS_NORMAL == _status && 0 == waitTime.toUINT64() )
      {
         ossTick endTick ;
         endTick.sample() ;
         waitTime = endTick - _createTSTick ;
      }
   }

   ossTickDelta         waitTime ;
   dpsTransLockId       lockID ;
   UINT32               ownerTID ;
   UINT32               waiterTID ;
   INT16                ownerType ;
   INT16                waiterType ;
   DPS_TRANSLOCK_TYPE   ownerMode ;
   DPS_TRANSLOCK_TYPE   lockMode ;
   UINT32               numOwner ;
   MsgQueryID           queryID ;
} ;

typedef _monClassLock monClassLock ;

UINT32 monGetTID() ;

/**
 * _monClassContainer defines
 *
 * It is a container for a _monClass type. Contains an active list and
 * archived list.
 * See below for latch protocol for these two lists.
 */
class _monClassContainer : public SDBObject
{
   friend class _monMonitorManager ;

   typedef boost::intrusive::list< _monClass, boost::intrusive::base_hook<BaseHook> > MONCLASS_LIST ;
   typedef _utilPartitionList< monClass, MONCLASS_LIST, monGetTID > MON_PARTITION_LIST ;

public:
   class iterator
   {
      friend class _monClassContainer ;
      _monClassContainer* _container ; // Pointer to monClassContainer
      MON_CLASS_LIST_TYPE _listType ; // which list this itr plan to scan
      MON_CLASS_LIST_TYPE _curListType ;  // current type of the list it is scanning
      MON_PARTITION_LIST::iterator _itr1 ; // active list iterator
      MONCLASS_LIST::iterator _itr2 ; // archive list iterator

      iterator( MON_PARTITION_LIST::iterator it )
      {
         _curListType = MON_CLASS_ACTIVE_LIST ;
         _itr1 = it ;
      }

      iterator( MONCLASS_LIST::iterator it )
      {
         _curListType = MON_CLASS_ARCHIVED_LIST ;
         _itr2 = it ;
      }

      // copy constructor
      iterator( const _monClassContainer::iterator &obj )
      {
         (*this) = obj ;
      }

   public:
      iterator() {}
      ~iterator()
      {
      }
      // By calling the constructor, the list type and the target container
      // pointer has to be passed in to initialize the member inside the object
      // Also the constructor decides which list to traverse and which
      // iterator to be used based on the list type.
      // If the list type is active list. _itr1 is used and _curlist is set to
      // active list to indicate which list we are traversing
      // If the list type is archived list. First check whether there is
      // pending archive, if so _curlist is set to active list first
      // and _itr1 is uesed to traverse first, after we done the active list
      // we will switch to archived list in ++ operation
      iterator( MON_CLASS_LIST_TYPE type, _monClassContainer* classContainer )
         : _container( classContainer ), _listType( type )
      {
         if ( _listType == MON_CLASS_ARCHIVED_LIST )
         {
            if ( _container->getNumPendingArchive() > 0 )
            {
               _itr1 = _container->_activeList.begin() ;
               // Since scan is paralle to delete, it is possible that
               // when we get the itr, it already hit the end of active list
               // if that happens, we need to switch to archived list
               if ( _itr1 != _container->_activeList.end() )
               {
                  _curListType = MON_CLASS_ACTIVE_LIST ;
               }
               else
               {
                  _curListType = MON_CLASS_ARCHIVED_LIST ;
                  _itr2 = _container-> _archivedList.begin() ;
               }
            }
            else
            {
               _curListType = MON_CLASS_ARCHIVED_LIST ;
               _itr2 = _container->_archivedList.begin() ;
               _itr1 = _container->_activeList.end() ;
            }
         }
         else
         {
            _curListType = MON_CLASS_ACTIVE_LIST ;
            _itr1 = _container->_activeList.begin() ;
            _itr2 = _container->_archivedList.end() ;
         }
      }

      // increment the iterator of current list it is traversing
      // if we hit the end of active list and is scan archived list
      // switch to the begin of _archivedList
      iterator& operator++ () // Pre-increment
      {
         if ( _curListType == MON_CLASS_ACTIVE_LIST )
         {
            SDB_ASSERT( _itr1 != _container->_activeList.end(),
                        "end of active list has already been reached" ) ;

            // Move to scan archive list if the end of active list is reached
            if ( ++_itr1 == _container->_activeList.end() &&
                _listType == MON_CLASS_ARCHIVED_LIST )
            {
               _curListType = MON_CLASS_ARCHIVED_LIST ;
               _itr2 = _container-> _archivedList.begin() ;
            }
         }
         else
         {
            SDB_ASSERT( _itr2 != _container->_archivedList.end(),
                        "end of archived list has already been reached" ) ;
            ++_itr2 ;
         }
         return *this;
      }

      iterator operator++ (int) // Post-increment
      {
         iterator tmp( *this ) ;
         ++( *this ) ;
         return tmp ;
      }

      BOOLEAN operator == (const iterator& rhs) const
      {
         if ( _curListType == rhs._curListType )
         {
            if ( _curListType == MON_CLASS_ACTIVE_LIST )
            {
               return _itr1 == rhs._itr1 ;
            }
            else
            {
               return _itr2 == rhs._itr2 ;
            }
         }
         return FALSE ;
      }

      BOOLEAN operator != (const iterator& rhs) const
      {
         return operator==( rhs ) ? FALSE : TRUE ;
      }

      MONCLASS_LIST::reference operator* () const
      {
         return ( _curListType == MON_CLASS_ACTIVE_LIST ) ? *_itr1 : *_itr2 ;
      }

      MONCLASS_LIST::pointer operator-> () const
      {
         return ( _curListType == MON_CLASS_ACTIVE_LIST ) ? &(*_itr1) : &(*_itr2) ;
      }
   } ;

   INT32 cleanup()
   {
      setMaxArchivedListLen( 0 ) ;
      setMonitorLvl( MON_DATA_LVL_NONE ) ;

      SDB_ASSERT( _activeList.size() == _numPendingArchive.fetch() + _numPendingDelete.fetch(),
                  "Has memory leak" ) ;

      MON_PARTITION_LIST::iterator it = _activeList.begin() ;
      while ( it != _activeList.end() )
      {
         monClass &obj = *it ;
         it = _activeList.erase(it) ;

         if ( obj.isPendingDelete() )
         {
            _numPendingDelete.dec() ;
         }
         else if ( obj.isPendingArchive() )
         {
            _numPendingArchive.dec() ;
         }
         /// release memory
         SDB_OSS_DEL &obj ;
      }

      MONCLASS_LIST::iterator it2 = _archivedList.begin() ;
      while ( it2 != _archivedList.end() )
      {
         monClass &obj = *it2 ;
         it2 = _archivedList.erase(it2) ;
         SDB_OSS_DEL &obj ;
      }
      return SDB_OK ;
   }

   ~_monClassContainer ()
   {
      cleanup() ;
   }

private:
   _monClassContainer (MON_CLASS_TYPE type) ;

   // disable assignment/copy
   _monClassContainer& operator= (const _monClassContainer& other) ;
   _monClassContainer( const _monClassContainer& other ) ;

   // a list of all the active objects
   MON_PARTITION_LIST _activeList ;

   // a list of all the archived objects
   MONCLASS_LIST _archivedList ;

   // Latch protecting the archived list. If active list and archived list must
   // be accessed at the same time, get the archive latch before begin scanning
   // the active list.
   ossSpinSLatch _archiveListLatch ;

   /**< number of elements in the archived list, protected by archivedListLatch */
   UINT32 _archivedListMaxLen ;
   /**< maximum number of elements allowed in the archived list */

   /**< Number of pending archive objects in the active list */
   ossAtomic32 _numPendingArchive ;
   /**< Number of pending delete objects in the active list */
   ossAtomic32 _numPendingDelete ;

   /**< The current data collection level for this class */
   MON_DATA_LEVEL _curCollectionLvl ;
   /**< The minimum collection level when monClass objects will get created */
   MON_DATA_LEVEL _minOperationalLvl ;

   MON_CLASS_TYPE _classType ;

   archiveFunc _doArchive ;

   UINT64      _lastNonOperationTick ;
   UINT64      _earliestTime ;

   /*
    * Process pending archive/delete objects from the active list
    */
   void _processPendingObj() ;

   /*
    * Remove archived objects when the number of obj exceeds the capacity
    */
   void _removeArchivedObj( UINT64 curTime ) ;

   void _resetLastNonOperationTick() { _lastNonOperationTick = 0 ; }

   BOOLEAN _hasExpired( UINT64 curTime ) ;

public:

   UINT32 getArchivedListLen() const { return _archivedList.size() ; }

   UINT32 getActiveListLen() { return _activeList.size() ; }

   void setMaxArchivedListLen( UINT32 size ) { _archivedListMaxLen = size ; }
   UINT32 getMaxArchivedListLen() const { return _archivedListMaxLen ; }

   UINT32 getNumPendingArchive() { return _numPendingArchive.fetch() ; }

   UINT32 getNumPendingDelete() { return _numPendingDelete.fetch() ; }

   void getArchiveLatch ( OSS_LATCH_MODE latchMode )
   {
      latchMode == EXCLUSIVE ? _archiveListLatch.get()
                             : _archiveListLatch.get_shared() ;
   }

   void releaseArchiveLatch ( OSS_LATCH_MODE latchMode )
   {
      latchMode == EXCLUSIVE ? _archiveListLatch.release()
                             : _archiveListLatch.release_shared() ;
   }

   void setMonitorLvl( MON_DATA_LEVEL mode )
   {
      _curCollectionLvl = mode ;
   }
   MON_DATA_LEVEL getCollectionLvl() { return _curCollectionLvl ; }

   BOOLEAN isOperational() const
   {
      return ( _curCollectionLvl >= _minOperationalLvl ) ? TRUE: FALSE ;
   }

   BOOLEAN isCurOperational() const
   {
      return ( monGroupMaskToLevle( monGetCurGroupMask(), _classType ) >= _minOperationalLvl ) ?
             TRUE : FALSE ;
   }

   /**
    * Add an object to the active list
    * @return The object added to the container
    */
   template<class T>
   T* add ()
   {
      T* obj = NULL ;
      if ( isOperational() && isCurOperational() )
      {
         try
         {
            obj = SDB_OSS_NEW T() ;
            if ( obj )
            {
               obj->dataLvl = getCollectionLvl() ;
               obj->_pPendingDelete = &_numPendingDelete ;
               _activeList.add( obj ) ;
            }
            else
            {
               PD_LOG( PDERROR, "Failed to allocate new monitor cb" ) ;
            }
         }
         catch( std::exception &e )
         {
            PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
            if ( obj )
            {
               SDB_OSS_DEL obj ;
               obj = NULL ;
            }
         }
      }
      return obj ;
   }

   /**
    * Add an object to the active list
    * @return The object added to the container
    */
   template<class T>
   T* add (_monClassBaseData *data)
   {
      T* obj = NULL ;
      if ( isOperational() && isCurOperational() )
      {
         try
         {
            obj = SDB_OSS_NEW T(data) ;
            if ( obj )
            {
               obj->dataLvl = getCollectionLvl() ;
               obj->_pPendingDelete = &_numPendingDelete ;
               _activeList.add( obj ) ;
            }
            else
            {
               PD_LOG( PDERROR, "Failed to allocate new monitor cb" ) ;
            }
         }
         catch( std::exception &e )
         {
            PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
            if ( obj )
            {
               SDB_OSS_DEL obj ;
               obj = NULL ;
            }
         }
      }
      return obj ;
   }

   /**
    * Add an object to the active list
    * @return The object added to the container
    */
   template<class T>
   T* add ( const T &data )
   {
      T* obj = NULL ;
      if ( isOperational() && isCurOperational() )
      {
         try
         {
            obj = SDB_OSS_NEW T(data) ;
            if ( obj )
            {
               obj->dataLvl = getCollectionLvl() ;
               obj->_pPendingDelete = &_numPendingDelete ;
               _activeList.add( obj ) ;
            }
            else
            {
               PD_LOG( PDERROR, "Failed to allocate new monitor cb" ) ;
            }
         }
         catch( std::exception &e )
         {
            PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
            if ( obj )
            {
               SDB_OSS_DEL obj ;
               obj = NULL ;
            }
         }
      }
      return obj ;
   }

   /**
    * Remove an object from the active list. The object will get archived based
    * on the archive rule.
    * @param obj the object to be added
    */
   void remove ( _monClass *obj ) ;

   /**
    * Return iterator to the first node of the list

    * Dependency: proper latches will been take care by iteraor
    * @param listType the type of list to read
    */
   iterator begin( MON_CLASS_LIST_TYPE listType )
   {
      SDB_ASSERT ( listType == MON_CLASS_ARCHIVED_LIST ||
                   listType == MON_CLASS_ACTIVE_LIST,
                   "Invalid monitor class list type" ) ;
      switch ( listType )
      {
         case ( MON_CLASS_ARCHIVED_LIST ):
            return iterator( MON_CLASS_ARCHIVED_LIST, this ) ;
         case ( MON_CLASS_ACTIVE_LIST ):
         default:
            return iterator( MON_CLASS_ACTIVE_LIST, this ) ;
      }
   }

   iterator end( MON_CLASS_LIST_TYPE listType )
   {
      SDB_ASSERT ( listType == MON_CLASS_ARCHIVED_LIST ||
                   listType == MON_CLASS_ACTIVE_LIST,
                   "Invalid monitor class list type" ) ;

      if ( listType == MON_CLASS_ARCHIVED_LIST )
      {
         return iterator( _archivedList.end() ) ;
      }
      else
      {
         return iterator( _activeList.end() ) ;
      }
   }

   #define MON_DUMP_CHECK_STEP               ( 2000 )

   /*
    * dump correspond monClass object this container has depends on the input listType.
    * @param cachedMonClassList target list we are going to populate
    * @param listType the type of list to read
    */
   template<class T>
   void dumpList( ossPoolVector<T> &cachedMonClassList, MON_CLASS_LIST_TYPE listType, IExecutor *cb)
   {
      BOOLEAN _hasArchiveLatch = FALSE ;
      iterator itr ;

      if ( ! this->isEmpty( listType ) )
      {
         try
         {
            UINT32 stepCount = 0 ;

            if ( listType == MON_CLASS_ARCHIVED_LIST )
            {
               this->getArchiveLatch( SHARED ) ;
               _hasArchiveLatch = TRUE ;
            }

            itr = begin(listType) ;
            while ( itr != end(listType) )
            {
               if ( ++stepCount > MON_DUMP_CHECK_STEP )
               {
                  stepCount = 0 ;
                  if ( cb && cb->isInterrupted( TRUE ) )
                  {
                     break ;
                  }
               }
               // 1. We skip any pending deletes
               // 2. Skip if this is a pending archive and we are only interested in$
               //    the active list$
               // 3. Skip if this is active and we are only interested in the$
               //    archived list$
               if (! ( itr->isPendingDelete() ||
                       (itr->isPendingArchive() && listType == MON_CLASS_ACTIVE_LIST ) ||
                       (!itr->isPendingArchive() && listType == MON_CLASS_ARCHIVED_LIST ) ))
               {
                  monClass * monClassElement = &(*itr) ;
                  T* monClassT = (T *) monClassElement ;
                  cachedMonClassList.push_back(*monClassT) ;
                  cachedMonClassList.back().done() ;
               }
               ++itr ;
            }
         }
         catch ( std::exception &e )
         {
            PD_LOG( PDERROR, "Failed to add monClass into vector:%s", e.what() ) ;
         }

         if ( _hasArchiveLatch )
         {
            this->releaseArchiveLatch( SHARED ) ;
         }
      }
   }

   /*
    * return whether this container is empty for  the correspond list depends
    * on the listType.
    * @param listType the type of list to read
    */
   BOOLEAN isEmpty(MON_CLASS_LIST_TYPE listType) ;
} ;

typedef _monClassContainer monClassContainer ;

} // namespace engine

#endif //MONCLASS_HPP_
