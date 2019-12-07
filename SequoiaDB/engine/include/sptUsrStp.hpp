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
