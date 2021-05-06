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

   Source File Name = result.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_RESULT_H_
#define VESSEL_RESULT_H_

#include "core.hpp"
#include "oss.hpp"

namespace engine
{
namespace vessel
{
   class result : public SDBObject
   {
      public:
         OSS_INLINE result(){}
         OSS_INLINE ~result(){}
         OSS_INLINE result(const result &o):
         _rc(o._rc)
         {}
         OSS_INLINE result &operator=(const result &o)
         {
            _rc = o._rc;
            return *this;
         }
         OSS_INLINE result &operator=(INT32 rc)
         {
            _rc = rc;
            return *this;
         }

      public:
         OSS_INLINE INT32 rc()const
         {
            return _rc;
         }
         OSS_INLINE BOOLEAN isOk()const
         {
            return SDB_OK == _rc;
         }
         OSS_INLINE void reset()
         {
            _rc = SDB_OK;
         }

      private:
         INT32 _rc = SDB_OK;
   };//class result
}//namespace vessel
}//namespace engine

#endif//VESSEL_FUNC_RESULT_H_