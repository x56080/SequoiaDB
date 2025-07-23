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

   Source File Name = fapMongoMonCommand.hpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains functions for agent processing.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who         Description
   ====== =========== =========== ==============================================
          06/27/2023  fangjiabin  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef _FAP_MONGO_MON_COMMAND_HPP_
#define _FAP_MONGO_MON_COMMAND_HPP_

#include "ossSocket.hpp"
#include "fapMongoCommand.hpp"

using namespace std ;
using namespace bson ;

#define FAP_OS_TYPE       "Linux"
#define FAP_OS_TYPE_LEN   5

namespace fap
{
class _mongoConnectionStatusCommand : public _mongoGlobalCommand
{
   MONGO_DECLARE_CMD_AUTO_REGISTER()
   public:
      _mongoConnectionStatusCommand() {}
      virtual ~_mongoConnectionStatusCommand() {}

      virtual MONGO_CMD_TYPE type() const { return CMD_CONNECT_STATUS ; }
      virtual const CHAR* name() const    { return MONGO_CMD_NAME_CONNECT_STATUS ; }

      virtual INT32 buildMongoReply( const MsgOpReply &sdbReply,
                                     engine::rtnContextBuf &replyBuf,
                                     _mongoResponseBuffer &resHeader ) ;
} ;
typedef _mongoConnectionStatusCommand mongoConnectionStatusCommand ;

struct _mongoHostInfo
{
   // system info
   time_t  currentTime ;
   CHAR    hostName[ OSS_MAX_HOSTNAME + 1 ] ;
   INT32   cpuAddrSize ;
   INT64   memSizeMB ;
   INT64   memLimitMB ;
   INT32   cpuCoreNum ;
   INT32   cpuPhyCoreNum ;
   INT32   cpuSocketNum ;
   std::string cpuArch ;
   BOOLEAN numaEnable ;
   UINT32  numaNodes ;

   // os info
   CHAR osType[ FAP_OS_TYPE_LEN + 1 ] ;
   std::string osName ;
   std::string osVersion ;

   // extra info
   std::string version ;
   std::string libcVersion ;
   std::string versionSignature ;
   std::string kernelVersion ;
   std::string cpuFrequencyMHz ;
   std::string cpuFeatures ;
   INT64 pageSize ;
   INT64 pageNum ;
   INT64 maxOpenFiles ;

   _mongoHostInfo()
   {
      ossMemset( hostName, 0, OSS_MAX_HOSTNAME + 1 ) ;
      ossMemset( osType, 0, FAP_OS_TYPE_LEN + 1 ) ;
   }
} ;
typedef struct _mongoHostInfo mongoHostInfo ;

class _mongoHostInfoCommand : public _mongoGlobalCommand
{
   MONGO_DECLARE_CMD_AUTO_REGISTER()
   public:
      _mongoHostInfoCommand() {}
      virtual ~_mongoHostInfoCommand() {}

      virtual MONGO_CMD_TYPE type() const { return CMD_HOST_INFO ; }
      virtual const CHAR* name() const    { return MONGO_CMD_NAME_HOST_INFO ; }

      virtual INT32 buildMongoReply( const MsgOpReply &sdbReply,
                                     engine::rtnContextBuf &replyBuf,
                                     _mongoResponseBuffer &resHeader ) ;
   private:
      INT32 _generateMongoHostInfo( mongoHostInfo &hostInfo ) ;
      INT32 _generateSystemInfo( mongoHostInfo &hostInfo ) ;
      INT32 _generateOsInfo( mongoHostInfo &hostInfo ) ;
      INT32 _generateExtraInfo( mongoHostInfo &hostInfo ) ;
      INT32 _getCpuInfo( mongoHostInfo &hostInfo ) ;

   private:
      ossOSInfo _osInfo ;
} ;
typedef _mongoHostInfoCommand mongoHostInfoCommand ;

class _mongoDatabaseStatsCommand : public _mongoDatabaseCommand
{
   MONGO_DECLARE_CMD_AUTO_REGISTER()
   public:
      _mongoDatabaseStatsCommand() {}
      virtual ~_mongoDatabaseStatsCommand() {}

      virtual MONGO_CMD_TYPE type() const { return CMD_DB_STATS ; }
      virtual const CHAR* name() const { return MONGO_CMD_NAME_DB_STATS ; }

      virtual INT32 buildSdbRequest( mongoMsgBuffer &sdbMsg,
                                     mongoSessionCtx &ctx,
                                     BOOLEAN &getMoreAll ) ;

      virtual INT32 buildMongoReply( const MsgOpReply &sdbReply,
                                     engine::rtnContextBuf &replyBuf,
                                     _mongoResponseBuffer &resHeader ) ;
} ;
typedef _mongoDatabaseStatsCommand mongoDatabaseStatsCommand ;

class _mongoCollectionStatsCommand : public _mongoCollectionCommand
{
   enum MONGO_COLL_STATS_STEP
   {
      MONGO_COLL_STATS_SNAP_IDX_STEP = 1,
      MONGO_COLL_STATS_SNAP_CL_STEP = 2,
   } ;
   MONGO_DECLARE_CMD_AUTO_REGISTER()
   public:
      _mongoCollectionStatsCommand()
      {
         _step = MONGO_COLL_STATS_SNAP_IDX_STEP ;
         _hasProcessAllMsg = FALSE ;
         _collectionCount = 0 ;
         _objects = 0 ;
         _avgObjSize = 0 ;
         _dataSize = 0 ;
         _totalDataSize = 0 ;
         _indexCount = 0 ;
         _indexSize = 0 ;
      }
      virtual ~_mongoCollectionStatsCommand() {}

      virtual MONGO_CMD_TYPE type() const { return CMD_COLL_STATS ; }
      virtual const CHAR* name() const { return MONGO_CMD_NAME_COLL_STATS ; }

      virtual BOOLEAN hasProcessAllMsg() const { return _hasProcessAllMsg ; }

      virtual INT32 buildSdbRequest( mongoMsgBuffer &sdbMsg,
                                     mongoSessionCtx &ctx,
                                     BOOLEAN &getMoreAll ) ;

      virtual INT32 parseSdbReply( const MsgOpReply &sdbReply,
                                   engine::rtnContextBuf &bodyBuf,
                                   INT32 &result ) ;

      virtual INT32 buildMongoReply( const MsgOpReply &sdbReply,
                                     engine::rtnContextBuf &replyBuf,
                                     _mongoResponseBuffer &resHeader ) ;

   private:
      INT32 _buildSnapIdxRequest( mongoMsgBuffer &sdbMsg, mongoSessionCtx &ctx ) ;
      INT32 _buildSnapClRequest( mongoMsgBuffer &sdbMsg, mongoSessionCtx &ctx ) ;
      INT32 _parseSnapIdxReply( const MsgOpReply &sdbReply,
                                engine::rtnContextBuf &bodyBuf ) ;
      INT32 _parseSnapClReply( const MsgOpReply &sdbReply,
                               engine::rtnContextBuf &bodyBuf ) ;

   private:
      MONGO_COLL_STATS_STEP _step ;
      BOOLEAN _hasProcessAllMsg ;
      // map< idxName, idxSize >
      std::map< string, INT64 > _idxMap ;
      INT32 _collectionCount ;
      INT64 _objects ;
      INT64 _avgObjSize ;
      INT64 _dataSize ;
      INT64 _totalDataSize ;
      INT32 _indexCount ;
      INT64 _indexSize ;
} ;
typedef _mongoCollectionStatsCommand mongoCollectionStatsCommand ;

INT32 fapMongoParseCLInfo( engine::rtnContextBuf &bodyBuf, INT32 &collectionCount,
                           INT64 &objects, INT64 &avgObjSize, INT64 &dataSize,
                           INT64 &totalDataSize, INT32 &indexCount, INT64 &indexSize ) ;

class _mongoTopCommand : public _mongoGlobalCommand
{
   enum MONGO_TOP_STEP
   {
      // Get the total number of queries from the master and secondary nodes
      MONGO_TOP_STEP1 = 1,
      // Get the total number of insert, update and remove from the master nodes
      MONGO_TOP_STEP2 = 2,
   } ;
   MONGO_DECLARE_CMD_AUTO_REGISTER()
   public:
      _mongoTopCommand() ;
      virtual ~_mongoTopCommand() {}

      virtual MONGO_CMD_TYPE type() const { return CMD_TOP ; }
      virtual const CHAR* name() const { return MONGO_CMD_NAME_TOP ; }

      virtual BOOLEAN needProcessByEngine() const { return TRUE ; }

      virtual BOOLEAN hasProcessAllMsg() const { return _hasProcessAllMsg ; }

      virtual INT32 buildSdbRequest( mongoMsgBuffer &sdbMsg,
                                     mongoSessionCtx &ctx,
                                     BOOLEAN &getMoreAll ) ;

      virtual INT32 parseSdbReply( const MsgOpReply &sdbReply,
                                   engine::rtnContextBuf &bodyBuf,
                                   INT32 &result ) ;

      virtual INT32 buildMongoReply( const MsgOpReply &sdbReply,
                                     engine::rtnContextBuf &replyBuf,
                                     _mongoResponseBuffer &resHeader ) ;

   private:
      INT32 _buildStep1Request( mongoMsgBuffer &sdbMsg, mongoSessionCtx &ctx ) ;
      INT32 _buildStep2Request( mongoMsgBuffer &sdbMsg, mongoSessionCtx &ctx ) ;
      INT32 _parseStep1Reply( const MsgOpReply &sdbReply,
                              engine::rtnContextBuf &bodyBuf ) ;
      INT32 _parseStep2Reply( const MsgOpReply &sdbReply,
                              engine::rtnContextBuf &bodyBuf ) ;

   private:
      MONGO_TOP_STEP _step ;
      BOOLEAN _hasProcessAllMsg ;
      std::map< string, std::vector<INT64> > _clCRUDCountMap ;
} ;
typedef _mongoTopCommand mongoTopCommand ;

class _mongoServerStatusCommand : public _mongoGlobalCommand
{
   MONGO_DECLARE_CMD_AUTO_REGISTER()
   public:
      _mongoServerStatusCommand() ;
      virtual ~_mongoServerStatusCommand() {}

      virtual MONGO_CMD_TYPE type() const { return CMD_SERVER_STATUS ; }
      virtual const CHAR* name() const { return MONGO_CMD_NAME_SERVER_STATUS ; }

      virtual BOOLEAN needProcessByEngine() const { return TRUE ; }

      virtual INT32 buildSdbRequest( mongoMsgBuffer &sdbMsg,
                                     mongoSessionCtx &ctx,
                                     BOOLEAN &getMoreAll ) ;

      virtual INT32 parseSdbReply( const MsgOpReply &sdbReply,
                                   engine::rtnContextBuf &bodyBuf,
                                   INT32 &result ) ;

      virtual INT32 buildMongoReply( const MsgOpReply &sdbReply,
                                     engine::rtnContextBuf &replyBuf,
                                     _mongoResponseBuffer &resHeader ) ;

   private:
      INT64 _netIn ;
      INT64 _netOut ;
      INT64 _rss ;
      INT64 _vsize ;
      INT64 _insertCount ;
      INT64 _deleteCount ;
      INT64 _updateCount ;
      INT64 _selectCount ;
      INT64 _connCount ;
} ;
typedef _mongoServerStatusCommand mongoServerStatusCommand ;

class _mongoCurrentOpCommand : public _mongoGlobalCommand
{
   MONGO_DECLARE_CMD_AUTO_REGISTER()
   struct _sessionOpInfo
   {
      INT64 sessionID ;
      INT64 milliSecRunning ;
      std::string clName ;
      BOOLEAN isBlocked ;

      _sessionOpInfo()
      {
         reset() ;
      }

      void reset()
      {
         sessionID = 0 ;
         milliSecRunning = 0 ;
         clName = "" ;
         isBlocked = FALSE ;
      }
   } ;
   typedef struct _sessionOpInfo sessionOpInfo ;

   public:
      virtual MONGO_CMD_TYPE type() const { return CMD_CURRENT_OP ; }
      virtual const CHAR* name() const    { return MONGO_CMD_NAME_CUR_OP ; }

      virtual BOOLEAN needProcessByEngine() const { return TRUE ; }

      virtual INT32 buildSdbRequest( mongoMsgBuffer &sdbMsg,
                                     mongoSessionCtx &ctx,
                                     BOOLEAN &getMoreAll ) ;

      virtual INT32 parseSdbReply( const MsgOpReply &sdbReply,
                                   engine::rtnContextBuf &bodyBuf,
                                   INT32 &result ) ;

      virtual INT32 buildMongoReply( const MsgOpReply &sdbReply,
                                     engine::rtnContextBuf &replyBuf,
                                     _mongoResponseBuffer &resHeader ) ;

   private:
      INT32 _getCLNameFromLastOpInfo( const std::string &lastOpInfo, std::string &clName ) ;

   private:
      std::vector<_sessionOpInfo> _sessionInfoVec ;
} ;
typedef _mongoCurrentOpCommand mongoCurrentOpCommand ;

class _mongoGetFreeMonStatusCommand : public _mongoGlobalCommand
{
   MONGO_DECLARE_CMD_AUTO_REGISTER()
   public:
      virtual MONGO_CMD_TYPE type() const { return CMD_FREE_MON ; }
      virtual const CHAR* name() const    { return MONGO_CMD_NAME_GETFREEMONSTATUS ; }

      virtual INT32 buildMongoReply( const MsgOpReply &sdbReply,
                                     engine::rtnContextBuf &replyBuf,
                                     _mongoResponseBuffer &resHeader ) ;
} ;
typedef _mongoGetFreeMonStatusCommand mongoGetFreeMonStatusCommand ;

class _mongoAtlasVersionCommand : public _mongoDummyCommand
{
   MONGO_DECLARE_CMD_AUTO_REGISTER()
   public:
      virtual MONGO_CMD_TYPE type() const { return CMD_ATLAS_VER ; }
      virtual const CHAR* name() const    { return MONGO_CMD_NAME_ATLAS_VERSION ; }
} ;
typedef _mongoAtlasVersionCommand mongoAtlasVersionCommand ;

class _mongoKillOpCommand : public _mongoDummyCommand
{
   MONGO_DECLARE_CMD_AUTO_REGISTER()
   public:
      virtual MONGO_CMD_TYPE type() const { return CMD_KILL_OP ; }
      virtual const CHAR* name() const    { return MONGO_CMD_NAME_KILL_OP ; }
} ;
typedef _mongoKillOpCommand mongoKillOpCommand ;

}

#endif
