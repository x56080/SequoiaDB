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

   Source File Name = dmsWTPersistUnit.hpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          11/20/2023  HGM Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef DMS_WT_PERSIST_UNIT_HPP_
#define DMS_WT_PERSIST_UNIT_HPP_

#include "dmsPersistUnit.hpp"
#include "wiredtiger/dmsWTSession.hpp"
#include "wiredtiger/dmsWTStorageEngine.hpp"
#include "wiredtiger/dmsWTUtil.hpp"

namespace engine
{
namespace wiredtiger
{

   /*
      _dmsWTPersistUnit define
    */
   class _dmsWTPersistUnit : public _dmsPersistUnit
   {
   public:
      _dmsWTPersistUnit( dmsWTStorageEngine &engine ) ;
      ~_dmsWTPersistUnit() ;
      _dmsWTPersistUnit( const _dmsWTPersistUnit &o ) = delete ;
      _dmsWTPersistUnit &operator =( const _dmsWTPersistUnit & ) = delete ;

      dmsWTSession &getWriteSession()
      {
         return _writeSession ;
      }

      dmsWTSession &getReadSession()
      {
         return _readSession ;
      }

      INT32 initUnit( IExecutor *executor ) ;

   protected:
      virtual INT32 _beginUnit( IExecutor *executor ) ;
      virtual INT32 _prepareUnit( IExecutor *executor ) ;
      virtual INT32 _commitUnit( IExecutor *executor ) ;
      virtual INT32 _abortUnit( IExecutor *executor ) ;

      virtual BOOLEAN _isTransSupported() const
      {
         return FALSE ;
      }

   protected:
      dmsWTStorageEngine &_engine ;
      // session for write operators
      // NOTE: snapshot of read session will not affect write session
      dmsWTSession _writeSession ;
      // session for read operators
      // NOTE: rollback of write session will not affect read session
      dmsWTSession _readSession ;
   } ;

   typedef class _dmsWTPersistUnit dmsWTPersistUnit ;

}
}

#endif // DMS_WT_PERSIST_UNIT_HPP_
