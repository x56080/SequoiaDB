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

   Source File Name = rtnLobMetaCache.hpp

   Descriptive Name = LOB meta cache

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/22/2017  David Li  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef RTN_LOB_META_CACHE_HPP_
#define RTN_LOB_META_CACHE_HPP_

#include "oss.hpp"
#include "ossUtil.hpp"
#include "dmsLobDef.hpp"
#include "rtnLobPieces.hpp"

namespace engine
{
   class _rtnLobMetaCache: public SDBObject
   {
   public:
      _rtnLobMetaCache() ;
      ~_rtnLobMetaCache() ;

   private:
      // disallow copy and assign
      _rtnLobMetaCache( const _rtnLobMetaCache& ) ;
      void operator=( const _rtnLobMetaCache& ) ;

   public:
      INT32 cache( const _dmsLobMeta& meta ) ;
      INT32 merge( const _dmsLobMeta& meta, INT32 lobPageSize ) ;

   public:
      OSS_INLINE const _dmsLobMeta* lobMeta()
      {
         return (_dmsLobMeta*)_metaBuf ;
      }
      OSS_INLINE BOOLEAN needMerge() const
      {
         return _needMerge ;
      }
      OSS_INLINE void setNeedMerge( BOOLEAN needMerge )
      {
         _needMerge = needMerge ;
      }

   private:
      CHAR*       _metaBuf ;
      BOOLEAN     _needMerge ;
   } ;
}

#endif /* RTN_LOB_META_CACHE_HPP_ */

