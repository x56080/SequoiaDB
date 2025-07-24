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

   Source File Name = dpsRequest.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#include "dpsRequest.hpp"
#include "ossLikely.hpp"
#include "pdTrace.hpp"

namespace engine
{
   dpsWriteRequest::dpsWriteRequest(dpsWriteRequest &&o) noexcept:
   _type(o._type),
   _flags(o._flags),
   _body(std::move(o._body))
   {
      o.reset();
   }

   dpsWriteRequest &dpsWriteRequest::operator=(dpsWriteRequest &&o) noexcept
   {
      _type = o._type;
      _flags = o._flags;
      _body = std::move(o._body);
      o.reset();
      return *this;
   }

   void dpsWriteRequest::reset()
   {
      _type = LOG_TYPE_DUMMY;
      _flags = 0;
      _body.reset();
      return;
   }

   BOOLEAN dpsWriteRequest::seek(DPS_TAG tag, utilSlice &value) const
   {
      BOOLEAN r = FALSE ; 
      dpsRecordElements::iterator itr = _body.seek(tag) ;
      if ( itr.isValid() )
      {
         value = itr.getValue() ;
         r = TRUE ;
      }
      else
      {
         value.reset() ;
      }

      return r ;
   }
} // namespace engine
