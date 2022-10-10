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

   Source File Name = clsCLMetaCache.cpp

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
#include "clsCLMetaCache.hpp"

namespace engine
{
_clsCLMetaCache::_clsCLMetaCache( const CHAR *clFullName,
                                  utilCLUniqueID cluid,
                                  const clsIndexInfoSetPtr &o )
: _name( clFullName ), _cluid( cluid ), _indexSetPtr( o )
{
}

_clsCLMetaCache::_clsCLMetaCache( const CHAR *clFullName,
                                  utilCLUniqueID cluid,
                                  clsIndexInfoSetPtr &&o )
: _name( clFullName ), _cluid( cluid ), _indexSetPtr( std::move( o ) )
{
}

_clsCLMetaCache::_clsCLMetaCache( std::string &&clFullName,
                                  utilCLUniqueID cluid,
                                  clsIndexInfoSetPtr &&o )
: _name( std::move( clFullName ) )
, _cluid( cluid )
, _indexSetPtr( std::move( o ) )
{
}

clsIndexInfoSetPtr &_clsCLMetaCache::_getIndexSet()
{
   return _indexSetPtr;
}

const std::string& _clsCLMetaCache::_getCLFullName()
{
   return _name;
}
} // namespace engine