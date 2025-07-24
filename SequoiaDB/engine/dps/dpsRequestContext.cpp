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

   Source File Name = dpsRequestContext.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#include "dpsRequestContext.hpp"
#include "pdTrace.hpp"
#include "ossLikely.hpp"


namespace engine
{
   void _dpsRequestContext::reset()
   {
      _o = dpsWriteOptions() ;
      _oplCtx = nullptr ;
      _toCompleteOpl = FALSE ;
      _oplRollbackTarget.reset() ;
      _builder.reset() ;
      _req.reset() ;
      _result = dpsLogRecordHeader() ;
      return ;
   }

   void _dpsRequestContext::endToBuildRequest()
   {
      _req = _builder.reap() ;
   }
} // namespace engine

