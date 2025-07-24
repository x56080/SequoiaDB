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

   Source File Name = sptLibssh2Session.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          31/03/2014  YW  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef SPT_LIBSSH2SESSION_HPP_
#define SPT_LIBSSH2SESSION_HPP_

#include "sptSshSession.hpp"
#include "libssh2.h"

class _OSS_FILE ;
namespace engine
{
   class _sptLibssh2Session : public _sptSshSession
   {
   public:
      _sptLibssh2Session( const CHAR *host,
                          const CHAR *usrname,
                          const CHAR *passwd,
                          INT32 *port = NULL ) ;
      virtual ~_sptLibssh2Session() ;

   public:
      virtual INT32 exec( const CHAR *cmd, INT32 &exit,
                          std::string &outStr ) ;

//      virtual INT32 read( CHAR *buf, UINT32 len, UINT32 &readSize ) ;

//      virtual INT32 done( INT32 &eixtcode, std::string &exitsignal ) ;

      virtual INT32 copy2Remote( SPT_CP_PROTOCOL protocol,
                                 const CHAR *local,
                                 const CHAR *dst,
                                 INT32 mode ) ;

      virtual INT32 copyFromRemote( SPT_CP_PROTOCOL protocol,
                                    const CHAR *remote,
                                    const CHAR *local,
                                    INT32 mode) ;

      virtual void getLastError( std::string &errMsg ) ;

   private:
      virtual INT32 _openSshSession() ;

      INT32 _read( std::string &outStr, INT32 streamId = 0 ) ;

      INT32 _done( INT32 &eixtcode, std::string &exitsignal ) ;

      void _clearChannel() ;

      void _clearSession() ;

      INT32 _scpSend( const CHAR *local, const CHAR *dst, INT32 mode ) ;

      INT32 _scpRecv( const CHAR *remote, const CHAR *local, INT32 mode ) ;

      INT32 _write2Channel( LIBSSH2_CHANNEL *channel,
                            const CHAR *buf, SINT64 len ) ;

      INT32 _readFromChannel( LIBSSH2_CHANNEL *channel,
                              SINT64 len,
                              CHAR *buf ) ;

      INT32 _wirte2File( _OSS_FILE *file, const CHAR *buf, SINT64 len ) ;

      void _getLastError( std::string &errMsg ) ;

      INT32 _getLastErrorRC() ;

   private:
      LIBSSH2_SESSION *_session ;
      LIBSSH2_CHANNEL *_channel ;
      std::string _errmsg ;
   } ;
   typedef class _sptLibssh2Session sptLibssh2Session ;
}

#endif

