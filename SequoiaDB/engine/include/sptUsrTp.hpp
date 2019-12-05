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

   Source File Name = sptUsrTp.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          18/08/2014  XJH Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef SPT_USR_TP_HPP__
#define SPT_USR_TP_HPP__

#include "sptApi.hpp"
#include "sptUsrTpAssit.hpp"
#include "sptUsrOmaAssit.hpp"

namespace engine
{

   /*
      _sptUsrTp define
    */
   class _sptUsrTp : public SDBObject
   {
   JS_DECLARE_CLASS( _sptUsrTp )

   public:
      _sptUsrTp() ;
      _sptUsrTp( const string &hostName,
                 const string &serviceName,
                 const string &omaServiceName ) ;
      virtual ~_sptUsrTp() ;

   public:
      INT32 construct( const _sptArguments &arg,
                       _sptReturnVal &rval,
                       bson::BSONObj &detail ) ;

      INT32 destruct() ;

      INT32 toString( const _sptArguments &arg,
                      _sptReturnVal &rval,
                      bson::BSONObj &detail ) ;

      INT32 close( const _sptArguments &arg,
                   _sptReturnVal &rval,
                   bson::BSONObj &detail ) ;

      INT32 start( const _sptArguments &arg,
                   _sptReturnVal &rval,
                   bson::BSONObj &detail ) ;

      INT32 stop( const _sptArguments &arg,
                  _sptReturnVal &rval,
                  bson::BSONObj &detail ) ;

      INT32 runCommand( const _sptArguments &arg,
                        _sptReturnVal &rval,
                        bson::BSONObj &detail ) ;

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
      sptUsrTpAssit _assit ;
      string         _hostName ;
      string         _serviceName ;
      string         _omaServiceName ;
   } ;

   typedef class _sptUsrTp sptUsrTp ;

}

#endif // SPT_USR_TP_HPP__

