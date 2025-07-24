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

   Source File Name = sptUsrStp.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/01/2019  HGM Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef SPT_USR_STP_HPP__
#define SPT_USR_STP_HPP__

#include "sptApi.hpp"
#include "sptUsrStpAssit.hpp"
#include "sptUsrOmaAssit.hpp"

namespace engine
{

   /*
      _sptUsrStp define
    */
   class _sptUsrStp : public SDBObject
   {
      JS_DECLARE_CLASS( _sptUsrStp )

   public:
      _sptUsrStp() ;
      _sptUsrStp( const string &hostName,
                  const string &serviceName,
                  const string &omaServiceName ) ;
      virtual ~_sptUsrStp() ;

   public:
      // constructor
      INT32 construct( const _sptArguments &arg,
                       _sptReturnVal &rval,
                       bson::BSONObj &detail ) ;

      // destructor
      INT32 destruct() ;

      // format to string
      INT32 toString( const _sptArguments &arg,
                      _sptReturnVal &rval,
                      bson::BSONObj &detail ) ;

      // close connection
      INT32 close( const _sptArguments &arg,
                   _sptReturnVal &rval,
                   bson::BSONObj &detail ) ;

      // start STP node
      INT32 start( const _sptArguments &arg,
                   _sptReturnVal &rval,
                   bson::BSONObj &detail ) ;

      // stop STP node
      INT32 stop( const _sptArguments &arg,
                  _sptReturnVal &rval,
                  bson::BSONObj &detail ) ;

      // run STP command
      INT32 runCommand( const _sptArguments &arg,
                        _sptReturnVal &rval,
                        bson::BSONObj &detail ) ;

      // help function
      static INT32 help( const _sptArguments &arg,
                         _sptReturnVal &rval,
                         bson::BSONObj &detail ) ;

   protected:
      INT32 _runCommand( const CHAR *command,
                         const bson::BSONObj &argument,
                         BOOLEAN needResult,
                         _sptReturnVal &rval,
                         BSONObj &detail ) ;
      INT32 _runOmaCommand( const CHAR *command,
                            const bson::BSONObj &argument,
                            BOOLEAN needResult,
                            _sptReturnVal &rval,
                            BSONObj &detail ) ;

   protected:
      // assistant for STP node
      sptUsrStpAssit _assit ;
      // host name of STP node
      string         _hostName ;
      // service name of STP node
      string         _serviceName ;
      // service name of OMA node ( used to send STP command to OMA )
      string         _omaServiceName ;
   } ;

   typedef class _sptUsrStp sptUsrStp ;

}

#endif // SPT_USR_STP_HPP__
