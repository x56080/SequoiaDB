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