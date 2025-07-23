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

   Source File Name = sptUsrOma.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          18/08/2014  XJH Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef SPT_USROMA_HPP__
#define SPT_USROMA_HPP__

#include "sptApi.hpp"
#include "sptUsrOmaAssit.hpp"

namespace engine
{

   /*
      _sptUsrOma define
   */
   class _sptUsrOma : public SDBObject
   {
   JS_DECLARE_CLASS( _sptUsrOma )

   public:
      _sptUsrOma() ;
      virtual ~_sptUsrOma() ;

   public:
      INT32 construct( const _sptArguments &arg,
                       _sptReturnVal &rval,
                       bson::BSONObj &detail) ;

      INT32 destruct() ;

      INT32 toString( const _sptArguments &arg,
                      _sptReturnVal &rval,
                      bson::BSONObj &detail ) ;

      static INT32 help( const _sptArguments &arg,
                         _sptReturnVal &rval,
                         bson::BSONObj &detail ) ;

      INT32 createCoord( const _sptArguments &arg,
                         _sptReturnVal &rval,
                         bson::BSONObj &detail ) ;

      INT32 removeCoord( const _sptArguments &arg,
                         _sptReturnVal &rval,
                         bson::BSONObj &detail ) ;

      INT32 createData( const _sptArguments &arg,
                        _sptReturnVal &rval,
                        bson::BSONObj &detail ) ;

      INT32 removeData( const _sptArguments &arg,
                        _sptReturnVal &rval,
                        bson::BSONObj &detail ) ;

      INT32 createOM( const _sptArguments &arg,
                      _sptReturnVal &rval,
                      bson::BSONObj &detail ) ;

      INT32 removeOM( const _sptArguments &arg,
                      _sptReturnVal &rval,
                      bson::BSONObj &detail ) ;

      INT32 startNode( const _sptArguments &arg,
                       _sptReturnVal &rval,
                       bson::BSONObj &detail ) ;

      INT32 stopNode( const _sptArguments &arg,
                      _sptReturnVal &rval,
                      bson::BSONObj &detail ) ;

      INT32 runCommand( const _sptArguments &arg,
                        _sptReturnVal &rval,
                        bson::BSONObj &detail ) ;

      INT32 close( const _sptArguments &arg,
                   _sptReturnVal &rval,
                   bson::BSONObj &detail ) ;

      /*
         static functions
      */
      static INT32 getOmaInstallInfo( const _sptArguments &arg,
                                      _sptReturnVal &rval,
                                      bson::BSONObj &detail ) ;

      static INT32 getOmaInstallFile( const _sptArguments &arg,
                                      _sptReturnVal &rval,
                                      bson::BSONObj &detail ) ;

      static INT32 getOmaConfigFile( const _sptArguments &arg,
                                     _sptReturnVal &rval,
                                     bson::BSONObj &detail ) ;

      static INT32 getOmaConfigs( const _sptArguments &arg,
                                  _sptReturnVal &rval,
                                  bson::BSONObj &detail ) ;

      static INT32 getIniConfigs( const _sptArguments &arg,
                                  _sptReturnVal &rval,
                                  bson::BSONObj &detail ) ;

      static INT32 setOmaConfigs( const _sptArguments &arg,
                                  _sptReturnVal &rval,
                                  bson::BSONObj &detail ) ;

      static INT32 setIniConfigs( const _sptArguments &arg,
                                  _sptReturnVal &rval,
                                  bson::BSONObj &detail ) ;

      static INT32 getAOmaSvcName( const _sptArguments &arg,
                                   _sptReturnVal &rval,
                                   bson::BSONObj &detail ) ;

      static INT32 addAOmaSvcName( const _sptArguments &arg,
                                   _sptReturnVal &rval,
                                   bson::BSONObj &detail ) ;

      static INT32 delAOmaSvcName( const _sptArguments &arg,
                                   _sptReturnVal &rval,
                                   bson::BSONObj &detail ) ;

      static INT32 start( const _sptArguments &arg,
                          _sptReturnVal &rval,
                          bson::BSONObj &detail ) ;

   protected:
      INT32 _createNode( const _sptArguments &arg,
                         _sptReturnVal &rval,
                         bson::BSONObj &detail,
                         const CHAR *pNodeStr ) ;

      INT32 _removeNode( const _sptArguments &arg,
                         _sptReturnVal &rval,
                         bson::BSONObj &detail,
                         const CHAR *pNodeStr ) ;

      INT32 _mergeArg( const _sptArguments &arg,
                       bson::BSONObj &detail,
                       string &command,
                       bson::BSONObj *mergeObj ) ;

      static INT32 _startSdbcm ( list<const CHAR*> &argv,
                                 OSSPID &pid,
                                 BOOLEAN asProc ) ;

   private:
      sptUsrOmaAssit          _assit ;
      string                  _hostname ;
      string                  _svcname ;

   } ;

}

#endif // SPT_USROMA_HPP__

