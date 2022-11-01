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

   Source File Name = clsCLMetaCache.hpp

   Descriptive Name =

   When/how to use:

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/30/2022  ZHY Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef CLS_CL_META_CACHE_HPP__
#define CLS_CL_META_CACHE_HPP__
#include "clsIndexInfo.hpp"
#include "ossMemPool.hpp"

namespace engine
{
   class _clsCLMetaCache
   {
      friend class _clsStorageResource;

   public:
      _clsCLMetaCache() = default;
      _clsCLMetaCache( ossPoolString &&clFullName,
                       utilCLUniqueID cluid,
                       const CLS_CL_STAT_PTR & = CLS_DEFAULT_CL_STAT,
                       CLS_INDEX_INFO_SET_PTR && = nullptr );

   public:
      INT32 init( const CHAR *clFullName,
                  utilCLUniqueID clUID,
                  const CLS_CL_STAT_PTR & = CLS_DEFAULT_CL_STAT,
                  CLS_INDEX_INFO_SET_PTR && = nullptr );
      void reset();
      void resetCLStat();
      void setCLStat( const CLS_CL_STAT_PTR &clStatPtr );
      void setIndexInfoSetPtr( const CLS_INDEX_INFO_SET_PTR &infoSetPtr );

   private:
      const ossPoolString &_getCLFullName() const;
      CLS_INDEX_INFO_SET_PTR &_getIndexInfoSet();
      CLS_CL_STAT_PTR &_getCLStat();

   private:
      // name is collection full name like [csName].[clName]
      ossPoolString _name;
      utilCLUniqueID _clUID = UTIL_UNIQUEID_NULL;
      CLS_CL_STAT_PTR _clStatPtr = CLS_DEFAULT_CL_STAT;
      CLS_INDEX_INFO_SET_PTR _indexInfoSetPtr = nullptr;
   };
   using clsCLMetaCache = _clsCLMetaCache;
   using clsCLMetaCachePtr = std::shared_ptr< clsCLMetaCache >;
} // namespace engine
#endif