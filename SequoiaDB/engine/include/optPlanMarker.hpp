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

   Source File Name = optPlanMarker.hpp

   Descriptive Name = Optimizer Access Plan Marker

   When/how to use: 

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          11/14/2012  ZHY Initial Draft

   Last Changed =

*******************************************************************************/
#include "ossMemPool.hpp"
#include "ossLatch.hpp"
#include "ossRWMutex.hpp"
#include "utilUniqueID.hpp"
#include <mutex>

namespace engine
{
   class _optPlanMarker
   {
   public:
      static constexpr INT32 OPT_MAIN_CL_INVALID_THRESHOLD = 5;
      static constexpr INT32 OPT_PARAM_INVALID_THRESHOLD = 5;

   public:
      void incMainCLInvalid( const CHAR *clFullName );
      void incParamInvalid( const CHAR *clFullName );

      void setMainCLInvalid( const CHAR *clFullName );
      void setParamInvalid( const CHAR *clFullName );

      void erase( const CHAR *clFullName );
      void clearMainCLInvalid( const CHAR *clFullName );
      void clearParamInvalid( const CHAR *clFullName );
      void clear();

      BOOLEAN testMainCLInvalid( const CHAR *clFullName );
      BOOLEAN testParamInvalid( const CHAR *clFullName );

   private:
      // first int count for MAIN-CL Invalid
      // second int count for Parameteried Invalid
      ossPoolMap< ossPoolString, std::pair< INT32, INT32 > > _counter;
      ossRWMutex _latch;
   };
   using optPlanMarker = _optPlanMarker;
} // namespace engine
