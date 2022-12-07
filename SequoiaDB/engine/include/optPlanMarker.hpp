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
