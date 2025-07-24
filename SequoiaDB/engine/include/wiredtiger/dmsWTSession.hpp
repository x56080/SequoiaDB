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

   Source File Name = dmsWTSession.hpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          11/20/2023  HGM Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef DMS_WT_SESSION_HPP_
#define DMS_WT_SESSION_HPP_

#include "interface/IStorageService.hpp"
#include "interface/IStorageSession.hpp"
#include "wiredtiger/dmsWTEngineOptions.hpp"
#include "wiredtiger/dmsWTUtil.hpp"
#include "interface/IOperationContext.hpp"

#include <wiredtiger.h>

namespace engine
{
namespace wiredtiger
{

   /*
      _dmsWTSessionIsolation define
    */
   enum class dmsWTSessIsolation
   {
      READ_UNCOMMITTED,
      READ_COMMITTED,
      SNAPSHOT
   } ;

   /*
      _dmsWTSession define
    */
   class _dmsWTSession : public IStorageSession
   {
   public:
      _dmsWTSession() ;
      ~_dmsWTSession() ;
      _dmsWTSession( const _dmsWTSession &o ) = delete ;
      _dmsWTSession &operator =( const _dmsWTSession & ) = delete ;

      INT32 open( WT_CONNECTION *conn,
                  dmsWTSessIsolation isolation = dmsWTSessIsolation::SNAPSHOT ) ;
      INT32 close() ;

      INT32 beginTrans() ;
      INT32 prepareTrans() ;
      INT32 commitTrans() ;
      INT32 abortTrans() ;

      WT_SESSION *getSession()
      {
         return _session ;
      }

      BOOLEAN isOpened() const
      {
         return nullptr != _session ;
      }

   protected:
      WT_SESSION *_session = nullptr ;
   } ;

   typedef class _dmsWTSession dmsWTSession ;

   /*
      _dmsWTSessionHolder define
    */
   class _dmsWTSessionHolder : public _utilPooledObject
   {
   public:
      _dmsWTSessionHolder() = default ;
      ~_dmsWTSessionHolder() = default ;
      _dmsWTSessionHolder( const _dmsWTSessionHolder &o ) = delete ;
      _dmsWTSessionHolder &operator =( const _dmsWTSessionHolder & ) = delete ;

      dmsWTSession &getSession()
      {
         return _session ? *_session : _tmpSession ;
      }

      void setSession( dmsWTSession *session )
      {
         _session = session ;
      }

   protected:
      dmsWTSession *_session = nullptr ;
      dmsWTSession _tmpSession ;
   } ;

   typedef class _dmsWTSessionHolder dmsWTSessionHolder ;

}
}

#endif // DMS_WT_ENGINE_HPP_
