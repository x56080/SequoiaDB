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

   Source File Name = dpsOperationList.hpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef DPS_OPLIST_HPP__
#define DPS_OPLIST_HPP__

#include "dpsOplistUnit.hpp"
#include "ossMemPool.hpp"

namespace engine
{
   class _dpsOperationList : public SDBObject
   {
      friend class _dpsOplistMaker ;

      public:
         enum STATUS : INT32 
         {
            START = 0x00,
            BUILDING = 0x01,
            ROLLING_BACK = 0x02,
            COMPLETED = 0x03,
         } ;

      public:
         OSS_INLINE STATUS getStatus() const { return _status ; }

         OSS_INLINE BOOLEAN isStart() const { return STATUS::START == _status ; }

         OSS_INLINE BOOLEAN isBuilding() const { return STATUS::BUILDING == _status ; }
         
         OSS_INLINE BOOLEAN isCompleted() const { return STATUS::COMPLETED == _status ; }

         OSS_INLINE BOOLEAN isRollingBack() const { return STATUS::ROLLING_BACK == _status ; }

         OSS_INLINE UINT32 getSize() const { return _units.size() ; }

         OSS_INLINE DPS_LSN_OFFSET getLSN() const
         {
            return _units.empty() ? DPS_INVALID_LSN_OFFSET : _units.front().getLSN() ;
         }

         OSS_INLINE DPS_LSN_OFFSET getCurrentTailLSN() const
         {
            return _units.empty() ? DPS_INVALID_LSN_OFFSET : _units.back().getLSN() ;
         }

         OSS_INLINE void reset()
         {
            _status = STATUS::START ; 
            _units.clear() ;
         }

      public:
         INT32 append(const dpsOplistUnit &unit) ;

      private:
         using _UNIT_CONTAINER = ossPoolList<dpsOplistUnit> ;


      private:
         STATUS _status = STATUS::START ;
         _UNIT_CONTAINER _units ;
   } ;//class _dpsOperationList

   using dpsOperationList = class _dpsOperationList ;
} // namespace engine


#endif//DPS_OPLIST_HPP__