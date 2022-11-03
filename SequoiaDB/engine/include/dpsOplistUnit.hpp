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

   Source File Name = dpsOplistUnit.hpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef DPS_OPLIST_UNIT_HPP__
#define DPS_OPLIST_UNIT_HPP__

#include "dpsLogRecord.hpp"

namespace engine
{
   class _dpsOplistUnit : public SDBObject
   {
      public:
         _dpsOplistUnit() = default ;
         _dpsOplistUnit( const dpsLogRecordHeader &rh ):
         _rh(rh) {}

      public:
         OSS_INLINE const dpsLogRecordHeader &getRecordHeader() const
         {
            return _rh ;
         }
         OSS_INLINE DPS_LSN_OFFSET getLSN() const
         {
            return _rh._lsn ;
         }

      private:
         dpsLogRecordHeader _rh ;
   } ;// class _dpsOplistUnit
   using dpsOplistUnit = class _dpsOplistUnit ;
} // namespace engine


#endif//DPS_OPLIST_UNIT_HPP__
