/*******************************************************************************


   Copyright (C) 2011-2018 SequoiaDB Ltd.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

   http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

   Source File Name = sptContainer.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          31/03/2014  YW  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef SPT_CONTAINER_HPP_
#define SPT_CONTAINER_HPP_

#include "core.hpp"
#include "oss.hpp"
#include "sptScope.hpp"
#include "ossLatch.hpp"
#include <vector>

namespace engine
{

   typedef std::vector< _sptScope* >            VEC_SCOPE ;
   typedef VEC_SCOPE::iterator                  VEC_SCOPE_IT ;

   /*
      _sptContainer define
   */
   class _sptContainer : public SDBObject
   {
   public:
      _sptContainer() ;
      virtual ~_sptContainer() ;

      INT32    init () ;
      INT32    fini () ;

   public:
      _sptScope   *newScope( SPT_SCOPE_TYPE type = SPT_SCOPE_TYPE_SP,
                             UINT32 loadMask = SPT_OBJ_MASK_ALL ) ;
      void        releaseScope( _sptScope *pScope ) ;

   protected:
      _sptScope* _getFromCache( SPT_SCOPE_TYPE type,
                                UINT32 loadMask ) ;

      _sptScope* _createScope( SPT_SCOPE_TYPE type,
                               UINT32 loadMask ) ;

   private:
      VEC_SCOPE            _vecScopes ;
      ossSpinXLatch        _latch ;

   } ;

   typedef class _sptContainer sptContainer ;
}

#endif // SPT_CONTAINER_HPP_

