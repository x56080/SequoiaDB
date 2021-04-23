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

   Source File Name = inMemIndexDefObj.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/inMemIndexDefObj.h"
#include "vessel/indexDef.h"

namespace engine
{
namespace vessel
{
   UINT32 inMemIndexDefObj::estimate(const strSlice &indexName,
                                     const indexKeyPattern &keyPattern)
   {
      return INDEX_DEF_RECORD_LEN +
             indexName.strLen() + 1 +
             keyPattern.getPattern().objsize();
   }

   inMemIndexDefObj::inMemIndexDefObj()
   {}

   inMemIndexDefObj::~inMemIndexDefObj()
   {
      SAFE_OSS_FREE(_buffer);
   }

   void inMemIndexDefObj::reset()
   {
      _record = indexDefRecord();
      _indexName.reset();
      _keyPattern.reset();
      return;
   }

}//namespace vessel
}//namespace engine
