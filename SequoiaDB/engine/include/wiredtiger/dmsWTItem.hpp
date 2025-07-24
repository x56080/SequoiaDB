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

   Source File Name = dmsWTItem.hpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          11/20/2023  HGM Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef DMS_WT_ITEM_HPP_
#define DMS_WT_ITEM_HPP_

#include "dmsDef.hpp"
#include "ossMemPool.hpp"
#include "../bson/bson.hpp"
#include "utilSlice.hpp"

#include <wiredtiger.h>

namespace engine
{
namespace wiredtiger
{

   /*
      _dmsWTItem define
    */
   class _dmsWTItem : public SDBObject
   {
   public:
      _dmsWTItem()
      {
         _item.data = nullptr ;
         _item.size = 0 ;
      }

      _dmsWTItem( const void* d, size_t s )
      {
         _item.data = d ;
         _item.size = s ;
      }

      ~_dmsWTItem() = default ;
      _dmsWTItem( const _dmsWTItem &o ) = delete ;
      _dmsWTItem &operator =( const _dmsWTItem & ) = delete ;

      _dmsWTItem( const bson::BSONObj &obj )
      : _dmsWTItem( obj.objdata(), obj.objsize() )
      {
      }

      _dmsWTItem( const utilSlice &slice )
      : _dmsWTItem( slice.getData(), slice.getSize() )
      {
      }

      void init( const bson::BSONObj &obj )
      {
         _item.data = obj.objdata() ;
         _item.size = obj.objsize() ;
      }

      void init( const utilSlice &slice )
      {
         _item.data = slice.getData() ;
         _item.size = slice.getSize() ;
      }

      WT_ITEM *get()
      {
         return &_item ;
      }

      const WT_ITEM *get() const
      {
         return &_item ;
      }

      const void *getData() const
      {
         return _item.data ;
      }

      UINT32 getSize() const
      {
         return _item.size ;
      }

      void reset()
      {
         _item.data = nullptr ;
         _item.size = 0 ;
      }

   protected:
      WT_ITEM _item ;
   } ;

   typedef class _dmsWTItem dmsWTItem ;

}
}

#endif // DMS_WT_ITEM_HPP_
