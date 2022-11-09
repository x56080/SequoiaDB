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

   Source File Name = dpsOplBuildingCtx.hpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef DPS_OPL_BUILDING_CTX_HPP__
#define DPS_OPL_BUILDING_CTX_HPP__

#include "dpsOplistDef.hpp"
#include "dpsLogDef.hpp"

namespace engine
{
   class _dpsOplBuildingCtx : public SDBObject
   {
      public:
         OSS_INLINE const DPS_LSN &getLSN() const { return _lsn ; }
         OSS_INLINE const DPS_LSN &getCurrentTailLSN() const { return _currentTail ; }
         OSS_INLINE UINT32 getSize() const { return _size ; }
         OSS_INLINE DPS_OPLIST_STATUS getStatus() const { return _status ; }
         OSS_INLINE BOOLEAN isCompleted() const
         {
            return DPS_OPLIST_STATUS::COMPLETED == _status ;
         }

      public:
         void reset() ;
         void push( const DPS_LSN &node ) ;
         void beginToRollBack() ;
         void setCompleted() ;

      private:
         DPS_LSN _lsn ;
         DPS_LSN _currentTail ;
         UINT32 _size = 0 ;
         DPS_OPLIST_STATUS _status = DPS_OPLIST_STATUS::START ;

   };//class _dpsOplBuildingCtx
   using dpsOplBuildingCtx = class _dpsOplBuildingCtx ;
} // namespace engine


#endif//DPS_OPL_BUILDING_CTX_HPP__