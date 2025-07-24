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

   Source File Name = dpsOplBuildingCtx.hpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
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