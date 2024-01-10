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

   Source File Name = dmsWTCursor.hpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          11/20/2023  HGM Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef DMS_WT_CURSOR_HPP_
#define DMS_WT_CURSOR_HPP_

#include "interface/IStorageService.hpp"
#include "ossTypes.h"
#include "wiredtiger/dmsWTEngineOptions.hpp"
#include "wiredtiger/dmsWTUtil.hpp"
#include "wiredtiger/dmsWTItem.hpp"

#include <wiredtiger.h>

namespace engine
{
namespace wiredtiger
{

   class _dmsWTSession ;

   /*
      _dmsWTCursor define
    */
   class _dmsWTCursor : public SDBObject
   {
   public:
      _dmsWTCursor( _dmsWTSession &session ) ;
      ~_dmsWTCursor() ;
      _dmsWTCursor( const _dmsWTCursor &o ) = delete ;
      _dmsWTCursor &operator =( const _dmsWTCursor & ) = delete ;

      INT32 open( const ossPoolString &uri,
                  const ossPoolString &config ) ;
      INT32 close() ;

      WT_CURSOR *getCursor()
      {
         return _cursor ;
      }

      BOOLEAN isOpened() const
      {
         return nullptr != _cursor ;
      }

      BOOLEAN isClosed() const
      {
         return nullptr == _cursor ;
      }

      INT32 next() ;
      INT32 prev() ;

      INT32 getKey( UINT64 &key ) ;
      INT32 getKey( const CHAR *&key ) ;
      INT32 getKey( const dmsWTItem &key ) ;

      INT32 getValue( dmsWTItem &value ) ;
      INT32 getValue( INT64 &value ) ;

      INT32 insert( UINT64 key, const dmsWTItem &value ) ;
      INT32 update( UINT64 key, const dmsWTItem &value ) ;
      INT32 remove( UINT64 key ) ;

      INT32 insert( const CHAR *key, const dmsWTItem &value ) ;
      INT32 update( const CHAR *key, const dmsWTItem &value ) ;
      INT32 remove( const CHAR *key ) ;

      INT32 insert( const dmsWTItem &key, const dmsWTItem &value ) ;
      INT32 update( const dmsWTItem &key, const dmsWTItem &value ) ;
      INT32 remove( const dmsWTItem &key ) ;

      INT32 search( UINT64 key ) ;
      INT32 search( const CHAR *key ) ;
      INT32 search( const dmsWTItem &key ) ;

      INT32 searchNext( UINT64 key, BOOLEAN isAfter, BOOLEAN &isFound ) ;
      INT32 searchNext( const CHAR *key, BOOLEAN isAfter, BOOLEAN &isFound ) ;
      INT32 searchNext( const dmsWTItem &key, BOOLEAN isAfter, BOOLEAN &isFound ) ;

      INT32 searchPrev( UINT64 key, BOOLEAN isBefore, BOOLEAN &isFound ) ;
      INT32 searchPrev( const CHAR *key, BOOLEAN isBefore, BOOLEAN &isFound ) ;
      INT32 searchPrev( const dmsWTItem &key, BOOLEAN isBefore, BOOLEAN &isFound ) ;

      INT32 searchAndGetValue( UINT64 key, dmsWTItem &value ) ;
      INT32 searchAndGetValue( const CHAR *key, dmsWTItem &value ) ;
      INT32 searchAndGetValue( const dmsWTItem &key, dmsWTItem &value ) ;

      INT32 searchPrefix( const dmsWTItem &key,
                          dmsWTItem &existsKey,
                          dmsWTItem &existsValue,
                          BOOLEAN &isFound,
                          BOOLEAN &isExactMatch ) ;
      INT32 moveToHead() ;
      INT32 moveToTail() ;

      INT32 getCount( UINT64 &count ) ;

      INT32 pause() ;

   protected:
      _dmsWTSession &_session ;
      ossPoolString _configString ;
      WT_CURSOR *_cursor = nullptr ;
   } ;

   typedef class _dmsWTCursor dmsWTCursor ;

}
}

#endif // DMS_WT_CURSOR_HPP_
