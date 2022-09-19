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

namespace engine
{
class _clsCLMetaCache
{
   friend class _clsStorageResource;

public:
   _clsCLMetaCache( const CHAR *clFullName,
                    utilCLUniqueID cluid,
                    const clsIndexInfoSetPtr & );
   _clsCLMetaCache( const CHAR *clFullName,
                    utilCLUniqueID cluid,
                    clsIndexInfoSetPtr && );
   _clsCLMetaCache( std::string &&clFullName,
                    utilCLUniqueID cluid,
                    clsIndexInfoSetPtr && );

private:
   clsIndexInfoSetPtr &_getIndexSet();
   const std::string &_getCLFullName();

private:
   // name is collection full name like [csName].[clName]
   std::string _name;
   utilCLUniqueID _cluid = UTIL_UNIQUEID_NULL;
   clsIndexInfoSetPtr _indexSetPtr = nullptr;
};
using clsCLMetaCache = _clsCLMetaCache;
using clsCLMetaCachePtr = std::shared_ptr< clsCLMetaCache >;
} // namespace engine
#endif