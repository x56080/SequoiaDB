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

   Source File Name = dmsWTStoreHolder.hpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          11/20/2023  HGM Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef DMS_WT_STORE_HOLDER_HPP_
#define DMS_WT_STORE_HOLDER_HPP_

#include "wiredtiger/dmsWTStore.hpp"
#include "wiredtiger/dmsWTStats.hpp"
#include "wiredtiger/dmsWTUtil.hpp"
#include "wiredtiger/dmsWTStorageEngine.hpp"

namespace engine
{
namespace wiredtiger
{

   /*
      _dmsWTStoreHolder define
    */
   class _dmsWTStoreHolder
   {
   public:
      _dmsWTStoreHolder( dmsWTStorageEngine &engine,
                         const dmsWTStore &store )
      : _engine( engine ),
        _store( store )
      {
      }

      ~_dmsWTStoreHolder() = default ;

      dmsWTStorageEngine &getEngine()
      {
         return _engine ;
      }

      dmsWTStore &getStore()
      {
         return _store ;
      }

      INT32 getCount( UINT64 &count, IExecutor *executor ) ;
      INT32 getStats( INT32 statsKey,
                      dmsWTStatsCatalog statsCatalog,
                      INT64 &statsValue,
                      IExecutor *executor ) ;
      INT32 getStoreTotalSize( UINT64 &totalSize, IExecutor *executor ) ;
      INT32 getStoreFreeSize( UINT64 &freeSize, IExecutor *executor ) ;

   protected:
      class _dmsWTStoreValidator
      {
      public:
         _dmsWTStoreValidator() = default ;
         virtual ~_dmsWTStoreValidator() = default ;

         virtual INT32 validate( const dmsWTItem &keyItem,
                                 const dmsWTItem &valueItem ) = 0 ;
      } ;

      INT32 _validateStore( _dmsWTStoreValidator &validator,
                            IExecutor *executor ) ;

   protected:
      dmsWTStorageEngine &_engine ;
      dmsWTStore _store ;
   } ;

   typedef class _dmsWTStoreHolder dmsWTStoreHolder ;

}
}

#endif // DMS_WT_STORE_HOLDER_HPP_
