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

   Source File Name = clsDEKFetcher.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          03/30/2023  ZHY Initial Draft

   Last Changed =

*******************************************************************************/

#include "utilDEKFetcher.hpp"
#include "clsShardMgr.hpp"

namespace engine
{
   class _clsDEKFetcher : public utilDEKFetcher
   {
      public:
         _clsDEKFetcher( _clsShardMgr & ) ;
         virtual ~_clsDEKFetcher() ;
         virtual INT32 fetch( ossSM4Key dek ) ;

      private:
         _clsShardMgr &_shardMgr ;
   } ;
   typedef _clsDEKFetcher clsDEKFetcher ;
} // namespace engine