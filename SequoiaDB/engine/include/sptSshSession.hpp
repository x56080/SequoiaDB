/*******************************************************************************


   Copyright (C) 2011-2018 SequoiaDB Ltd.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

   http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

   Source File Name = sptSshSession.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          31/03/2014  YW  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef SPT_SSHSESSION_HPP_
#define SPT_SSHSESSION_HPP_

#include "core.hpp"
#include "oss.hpp"
#include "ossSocket.hpp"
#include <string>

using namespace std ;

namespace engine
{
   enum SPT_CP_PROTOCOL
   {
      SPT_CP_PROTOCOL_SCP = 0,
      SPT_CP_PROTOCOL_FTP,
      SPT_CP_PROTOCOL_SFTP,
   } ;

   #define SPT_SSH_PORT             ( 22 )

   class _sptSshSession : public SDBObject
   {
   public:
      _sptSshSession( const CHAR *host,
                      const CHAR *usrname,
                      const CHAR *passwd,
                      INT32 *port = NULL ) ;
      virtual ~_sptSshSession() ;

      const CHAR* getPassword() const { return _passwd.c_str() ; }

      string getLocalIPAddr() ;
      string getPeerIPAddr() ;

      void   close() ;
      BOOLEAN isOpened() const { return _isOpen ; }

   public:
      INT32 open() ;

      virtual INT32 exec( const CHAR *cmd, INT32 &exit,
                          std::string &outStr ) = 0 ;

//      virtual INT32 read( CHAR *buf, UINT32 len, UINT32 &readSize ) = 0 ;

//      virtual INT32 done( INT32 &eixtcode, std::string &exitsignal ) = 0 ;

      virtual INT32 copy2Remote( SPT_CP_PROTOCOL protocol,
                                 const CHAR *local,   /// full path
                                 const CHAR *dst,   /// full path
                                 INT32 mode ) = 0 ;

      virtual INT32 copyFromRemote( SPT_CP_PROTOCOL protocol,
                                    const CHAR *remote,
                                    const CHAR *local,
                                    INT32 mode ) = 0 ;

      virtual void getLastError( std::string &errMsg ) = 0 ;

   private:
      virtual INT32 _openSshSession() = 0 ;

   protected:
      std::string _host ;
      INT32       _port ;
      std::string _usr ;
      std::string _passwd ;
      BOOLEAN     _isOpen ;
      _ossSocket *_sock ;

   } ;
   typedef class _sptSshSession sptSshSession ;
}

#endif

