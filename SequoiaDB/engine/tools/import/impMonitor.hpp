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

   Source File Name = impMonitor.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          4/8/2015  David Li  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef IMP_MONITOR_HPP_
#define IMP_MONITOR_HPP_

#include "core.hpp"
#include "oss.hpp"
#include "ossAtomic.hpp"

namespace import
{
   class Monitor: public SDBObject
   {
   public:
      Monitor();
      ~Monitor();

      inline INT64 recordsMem() { return _recordsMem.fetch(); }
      inline void recordsMemInc(INT64 size) { _recordsMem.add(size); }
      inline void recordsMemDec(INT64 size) { _recordsMem.sub(size); }

      inline INT64 recordsNum() { return _recordsNum.fetch(); }
      inline void recordsNumInc(INT64 size) { _recordsNum.add(size); }
      inline void recordsNumDec(INT64 size) { _recordsNum.sub(size); }

   private:
      ossAtomicSigned64  _recordsMem;
      ossAtomicSigned64  _recordsNum;
   };

   Monitor* impGetMonitor();
}

#endif /* IMP_MONITOR_HPP_ */
