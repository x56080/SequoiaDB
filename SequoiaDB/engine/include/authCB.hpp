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

   Source File Name = authCB.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          07/12/2012  YW  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef AUTHCB_HPP__
#define AUTHCB_HPP__

#include "core.hpp"
#include "oss.hpp"
#include "../bson/bson.h"
#include "sdbInterface.hpp"

using namespace bson ;

namespace engine
{

   class _pmdEDUCB ;

   /*
      _authCB define
   */
   class _authCB : public _IControlBlock
   {
   public:
      _authCB() ;
      ~_authCB() ;

      virtual SDB_CB_TYPE cbType() const { return SDB_CB_AUTH ; }
      virtual const CHAR* cbName() const { return "AUTHCB" ; }

      virtual INT32  init () ;
      virtual INT32  active () ;
      virtual INT32  deactive () ;
      virtual INT32  fini () ;
      virtual void   onConfigChange() ;

   public:

      INT32 createUsr( BSONObj &obj, _pmdEDUCB *cb, INT32 w = 1 ) ;

      INT32 updatePasswd( const string &user, const string &oldPasswd, 
                          const string &newPasswd, _pmdEDUCB *cb ) ;

      INT32 removeUsr( BSONObj &obj, _pmdEDUCB *cb, INT32 w = 1 ) ;

      INT32 authenticate( BSONObj &obj, _pmdEDUCB *cb,
                          BOOLEAN chkPasswd = TRUE ) ;

      INT32 needAuthenticate( _pmdEDUCB *cb, BOOLEAN &need ) ;

      BOOLEAN authEnabled() const
      {
         return _authEnabled ;
      }

   private:
      INT32 _initAuthentication( _pmdEDUCB *cb ) ;
      INT32 _createUsr( BSONObj &obj, _pmdEDUCB *cb, INT32 w = 1 ) ;
      INT32 _valid( BSONObj &obj, BOOLEAN notEmpty ) ;
      INT32 _validSource( BSONObj &obj, BOOLEAN chkPasswd ) ;

   private:
      BOOLEAN _authEnabled ;
   } ;

   typedef class _authCB SDB_AUTHCB ;

   /*
      get gloabl SDB_AUTHCB cb
   */
   SDB_AUTHCB* sdbGetAuthCB () ;

}

#endif // AUTHCB_HPP__

