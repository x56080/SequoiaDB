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

   Source File Name = monCB.cpp

   Descriptive Name = Monitor Control Block

   When/how to use: this program may be used on binary and text-formatted
   versions of monitoring component. This file contains structure for
   database, application and context snapshot.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/14/2012  TW  Initial Draft

   Last Changed =

*******************************************************************************/
#include "monCB.hpp"
#include "pmd.hpp"
#include "rtnCommand.hpp"

namespace engine
{

   /*
      Global functions
   */
   static monSvcTaskInfo** _monGetDefaultTaskInfo()
   {
      static monSvcTaskInfo *s_pDfaultTaskInfo = NULL ;
      return &s_pDfaultTaskInfo ;
   }

   void monSetDefaultTaskInfo( monSvcTaskInfo *pTaskInfo )
   {
      *_monGetDefaultTaskInfo() = pTaskInfo ;
   }

   /*
      _monDBCB implement
   */
   _monDBCB::_monDBCB()
   :_curConns( 0 )
   {
      reset() ;
   }

   void _monDBCB::reset()
   {
      ossAtomicExchange64( &totalDataRead, 0 ) ;
      ossAtomicExchange64( &totalIndexRead, 0 ) ;
      ossAtomicExchange64( &totalLobRead, 0 ) ;
      ossAtomicExchange64( &totalDataWrite, 0 ) ;
      ossAtomicExchange64( &totalIndexWrite, 0 ) ;
      ossAtomicExchange64( &totalLobWrite, 0 ) ;

      ossAtomicExchange64( &totalUpdate, 0 ) ;
      ossAtomicExchange64( &totalDelete, 0 ) ;
      ossAtomicExchange64( &totalInsert, 0 ) ;
      ossAtomicExchange64( &totalSelect, 0 ) ;
      ossAtomicExchange64( &totalRead, 0 ) ;

      ossAtomicExchange64( &receiveNum, 0 ) ;

      ossAtomicExchange64( &replUpdate, 0 ) ;
      ossAtomicExchange64( &replInsert, 0 ) ;
      ossAtomicExchange64( &replDelete, 0 ) ;

      ossAtomicExchange64( &_svcNetIn, 0 ) ;
      ossAtomicExchange64( &_svcNetOut, 0 ) ;

      totalReadTime.clear() ;
      totalWriteTime.clear() ;

      ossGetCurrentTime( _resetTimestamp ) ;
   }

   _monDBCB& _monDBCB::operator= ( const _monDBCB &rhs )
   {
      totalDataRead             = rhs.totalDataRead ;
      totalIndexRead            = rhs.totalIndexRead ;
      totalLobRead              = rhs.totalLobRead ;
      totalDataWrite            = rhs.totalDataWrite ;
      totalIndexWrite           = rhs.totalIndexWrite ;
      totalLobWrite             = rhs.totalLobWrite ;

      totalUpdate               = rhs.totalUpdate ;
      totalDelete               = rhs.totalDelete ;
      totalInsert               = rhs.totalInsert ;
      totalSelect               = rhs.totalSelect ;
      totalRead                 = rhs.totalRead ;

      receiveNum                = rhs.receiveNum ;

      replUpdate                = rhs.replUpdate ;
      replDelete                = rhs.replDelete ;
      replInsert                = rhs.replInsert ;

      _svcNetIn                 = rhs._svcNetIn ;
      _svcNetOut                = rhs._svcNetOut ;

      totalReadTime             = rhs.totalReadTime ;
      totalWriteTime            = rhs.totalWriteTime ;
      _activateTimestamp        = rhs._activateTimestamp ;
      _resetTimestamp           = rhs._resetTimestamp ;

      return *this ;
   }

   void _monDBCB::recordActivateTimestamp()
   {
      ossGetCurrentTime( _activateTimestamp ) ;
      _resetTimestamp = _activateTimestamp ;
   }

   BOOLEAN _monDBCB::isConnLimited( UINT32 maxConn )
   {
      if ( maxConn > 0 && _curConns.fetch() > maxConn )
      {
         return TRUE ;
      }
      return FALSE ;
   }

   /*
      _monAppCB implement
   */
   _monAppCB::_monAppCB()
   {
      reset() ;

      _taskInfo = *_monGetDefaultTaskInfo() ;
      mondbcb = pmdGetKRCB()->getMonDBCB() ;
   }

   _monAppCB &_monAppCB::operator= ( const _monAppCB &rhs )
   {
      mondbcb                   = rhs.mondbcb ;
      _taskInfo                 = rhs._taskInfo ;

      totalDataRead             = rhs.totalDataRead ;
      totalIndexRead            = rhs.totalIndexRead ;
      totalLobRead              = rhs.totalLobRead ;
      totalDataWrite            = rhs.totalDataWrite ;
      totalIndexWrite           = rhs.totalIndexWrite ;
      totalLobWrite             = rhs.totalLobWrite ;

      totalUpdate               = rhs.totalUpdate ;
      totalDelete               = rhs.totalDelete ;
      totalInsert               = rhs.totalInsert ;
      totalSelect               = rhs.totalSelect ;
      totalRead                 = rhs.totalRead ;

      totalReadTime             = rhs.totalReadTime ;
      totalWriteTime            = rhs.totalWriteTime ;
      _connectTimestamp         = rhs._connectTimestamp ;
      _resetTimestamp           = rhs._resetTimestamp ;

      _lastOpType               = rhs._lastOpType ;
      _cmdType                  = rhs._cmdType ;
      _lastOpBeginTime          = rhs._lastOpBeginTime ;
      _lastOpEndTime            = rhs._lastOpEndTime ;
      _readTimeSpent            = rhs._readTimeSpent ;
      _writeTimeSpent           = rhs._writeTimeSpent ;
      ossStrcpy( _lastOpDetail, rhs._lastOpDetail ) ;

      return *this ;
   }

   _monAppCB &_monAppCB::operator+= ( const _monAppCB &rhs )
   {
      totalDataRead              += rhs.totalDataRead ;
      totalIndexRead             += rhs.totalIndexRead ;
      totalLobRead               += rhs.totalLobRead ;
      totalDataWrite             += rhs.totalDataWrite ;
      totalIndexWrite            += rhs.totalIndexWrite ;
      totalLobWrite              += rhs.totalLobWrite ;

      totalUpdate                += rhs.totalUpdate ;
      totalDelete                += rhs.totalDelete ;
      totalInsert                += rhs.totalInsert ;
      totalSelect                += rhs.totalSelect ;
      totalRead                  += rhs.totalRead ;

      totalReadTime              += rhs.totalReadTime ;
      totalWriteTime             += rhs.totalWriteTime ;

      _readTimeSpent             += rhs._readTimeSpent ;
      _writeTimeSpent            += rhs._writeTimeSpent ;

      return *this ;
   }

   void _monAppCB::setSvcTaskInfo( monSvcTaskInfo *pSvcTaskInfo )
   {
      if ( pSvcTaskInfo )
      {
         _taskInfo = pSvcTaskInfo ;
      }
      else if ( _taskInfo != *_monGetDefaultTaskInfo() )
      {
         _taskInfo = *_monGetDefaultTaskInfo() ;
      }
   }

   monSvcTaskInfo* _monAppCB::getSvcTaskInfo()
   {
      return _taskInfo ;
   }

   void _monAppCB::reset()
   {
      totalDataRead = 0 ;
      totalIndexRead = 0 ;
      totalLobRead   = 0 ;
      totalDataWrite = 0 ;
      totalIndexWrite = 0 ;
      totalLobWrite   = 0 ;

      totalUpdate = 0 ;
      totalDelete = 0 ;
      totalInsert = 0 ;
      totalSelect = 0 ;
      totalRead  = 0 ;

      totalReadTime.clear() ;
      totalWriteTime.clear() ;

      ossGetCurrentTime( _resetTimestamp ) ;

      _lastOpType = MSG_NULL ;
      _cmdType = CMD_UNKNOW ;
      _lastOpBeginTime.clear() ;
      _lastOpEndTime.clear() ;
      _readTimeSpent.clear() ;
      _writeTimeSpent.clear() ;
      ossMemset( _lastOpDetail, 0, sizeof( _lastOpDetail ) ) ;
   }

   void _monAppCB::startOperator()
   {
      _lastOpBeginTime = pmdGetKRCB()->getCurTime() ;
      _lastOpEndTime.clear() ;
      _lastOpType = MSG_NULL ;
      _cmdType = CMD_UNKNOW ;
      ossMemset( _lastOpDetail, 0, sizeof(_lastOpDetail) ) ;
   }

   void _monAppCB::endOperator()
   {
      if ( (BOOLEAN)_lastOpBeginTime )
      {
         _lastOpEndTime = pmdGetKRCB()->getCurTime() ;
         ossTickDelta delta = _lastOpEndTime - _lastOpBeginTime ;
         opTimeSpentInc( delta ) ;
         _lastOpBeginTime.clear() ;
      }
   }

   void _monAppCB::setLastOpType( INT32 opType )
   {
      _lastOpType = opType ;
   }

   void _monAppCB::setLastCmdType( INT32 cmdType )
   {
      _cmdType = cmdType ;
   }

   void _monAppCB::opTimeSpentInc( ossTickDelta delta )
   {
      switch ( _lastOpType )
      {
         case MSG_BS_QUERY_REQ :
            {
               if ( _cmdType != CMD_UNKNOW )
               {
                  // it is command, do not inc readTimeSpent
                  break ;
               }
            }
         case MSG_BS_GETMORE_REQ :
         /// LOB
         case MSG_BS_LOB_READ_REQ :
            {
               _readTimeSpent += delta ;
               break ;
            }
         case MSG_BS_INSERT_REQ :
         case MSG_BS_UPDATE_REQ :
         case MSG_BS_DELETE_REQ :
         /// LOB
         case MSG_BS_LOB_WRITE_REQ :
         case MSG_BS_LOB_REMOVE_REQ :
         case MSG_BS_LOB_UPDATE_REQ :
         case MSG_BS_LOB_TRUNCATE_REQ :
            {
               _writeTimeSpent += delta ;
               break ;
            }
         default :
            break ;
      }
   }

   void _monAppCB::saveLastOpDetail( const CHAR *format, ... )
   {
      va_list argList ;
      UINT32 curLen = ossStrlen( _lastOpDetail ) ;
      if ( curLen >= sizeof( _lastOpDetail ) - 3 )
      {
         // buffer is full, couldn't save more info
         goto done ;
      }
      else if ( curLen > 0 )
      {
         _lastOpDetail[ curLen ] = ',' ;
         ++curLen ;
         _lastOpDetail[ curLen ] = ' ' ;
         ++curLen ;
         _lastOpDetail[ curLen ] = 0 ;
      }
      va_start( argList, format ) ;
      vsnprintf( _lastOpDetail + curLen,
                 sizeof( _lastOpDetail ) - curLen - 1,
                 format, argList ) ;
      va_end( argList ) ;
   done:
      return ;
   }

   /*
      _monContextCB implement
    */
   _monContextCB::_monContextCB ()
   : _contextID( -1 ),
     _dataRead( 0 ),
     _indexRead( 0 ),
     _lobRead( 0 ),
     _lobWrite( 0 ),
     _returnBatches( 0 ),
     _returnRecords( 0 ),
     _startTimestamp(),
     _waitTime(),
     _queryTime(),
     _executeTime()
   {
   }

   _monContextCB::_monContextCB ( const _monContextCB & monCtxCB )
   : _contextID( monCtxCB._contextID ),
     _dataRead( monCtxCB._dataRead ),
     _indexRead( monCtxCB._indexRead ),
     _lobRead( monCtxCB._lobRead ),
     _lobWrite( monCtxCB._lobWrite ),
     _returnBatches( monCtxCB._returnBatches ),
     _returnRecords( monCtxCB._returnRecords ),
     _startTimestamp( monCtxCB._startTimestamp ),
     _waitTime( monCtxCB._waitTime ),
     _queryTime( monCtxCB._queryTime ),
     _executeTime( monCtxCB._executeTime )
   {
   }

   _monContextCB::~_monContextCB ()
   {
   }

   void _monContextCB::reset ()
   {
      _contextID     = -1 ;
      _dataRead      = 0 ;
      _indexRead     = 0 ;
      _lobRead       = 0 ;
      _lobWrite      = 0 ;
      _returnBatches = 0 ;
      _returnRecords = 0 ;
      _startTimestamp.clear() ;
      _waitTime.clear() ;
      _queryTime.clear() ;
      _executeTime.clear() ;
   }

   _monContextCB & _monContextCB::operator = ( const _monContextCB & monCtxCB )
   {
      _contextID        = monCtxCB._contextID ;
      _dataRead         = monCtxCB._dataRead ;
      _indexRead        = monCtxCB._indexRead ;
      _lobRead          = monCtxCB._lobRead ;
      _lobWrite         = monCtxCB._lobWrite ;
      _returnBatches    = monCtxCB._returnBatches ;
      _returnRecords    = monCtxCB._returnRecords ;
      _startTimestamp   = monCtxCB._startTimestamp ;
      _waitTime         = monCtxCB._waitTime ;
      _queryTime        = monCtxCB._queryTime ;
      _executeTime      = monCtxCB._executeTime ;

      return ( *this ) ;
   }

   /*
      _monSvcTaskInfo implement
   */
   _monSvcTaskInfo::_monSvcTaskInfo()
   {
      reset() ;

      _startTimestamp = _resetTimestamp ;
      _taskID = 0 ;
      ossMemset( _taskName, 0, sizeof( _taskName ) ) ;
   }

   void _monSvcTaskInfo::setTaskInfo( UINT64 taskID, const CHAR *taskName )
   {
      _taskID = taskID ;
      ossStrncpy( _taskName, taskName, MON_SVC_TASK_NAME_LEN ) ;
   }

   void _monSvcTaskInfo::reset()
   {
      ossAtomicExchange64( &_totalTime, 0 ) ;
      ossAtomicExchange64( &_totalDataRead, 0 ) ;
      ossAtomicExchange64( &_totalIndexRead, 0 ) ;
      ossAtomicExchange64( &_totalLobRead, 0 ) ;
      ossAtomicExchange64( &_totalDataWrite, 0 ) ;
      ossAtomicExchange64( &_totalIndexWrite, 0 ) ;
      ossAtomicExchange64( &_totalLobWrite, 0 ) ;
      ossAtomicExchange64( &_totalUpdate, 0 ) ;
      ossAtomicExchange64( &_totalDelete, 0 ) ;
      ossAtomicExchange64( &_totalInsert, 0 ) ;
      ossAtomicExchange64( &_totalSelect, 0 ) ;
      ossAtomicExchange64( &_totalRead, 0 ) ;
      ossAtomicExchange64( &_totalWrite, 0 ) ;
      ossAtomicExchange64( &_totalContexts, 0 ) ;

      ossGetCurrentTime( _resetTimestamp ) ;
   }

}

