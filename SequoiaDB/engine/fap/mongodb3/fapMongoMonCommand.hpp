/*******************************************************************************


   Copyright (C) 2011-2018 SequoiaDB Ltd.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU Affero General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.

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

INT32 fapMongoParseCLInfo( engine::rtnContextBuf &bodyBuf, INT32 &collectionCount,
                           INT64 &objects, INT64 &avgObjSize, INT64 &dataSize,
                           INT64 &totalDataSize, INT32 &indexCount, INT64 &indexSize ) ;

class _mongoCurrentOpCommand : public _mongoDummyCommand
{
   MONGO_DECLARE_CMD_AUTO_REGISTER()
   public:
      virtual MONGO_CMD_TYPE type() const { return CMD_CURRENT_OP ; }
      virtual const CHAR* name() const    { return MONGO_CMD_NAME_CUR_OP ; }
} ;
typedef _mongoCurrentOpCommand mongoCurrentOpCommand ;

}

#endif