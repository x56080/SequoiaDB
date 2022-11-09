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

   Source File Name = dpsRequestContext.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

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

