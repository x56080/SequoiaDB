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

   Source File Name = dpsOplistContext.hpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef DPS_OPLIST_CONTEXT_HPP__
#define DPS_OPLIST_CONTEXT_HPP__

#include "dpsOplBuildingCtx.hpp"

#include <array>

namespace engine
{
   class _dpsOplistContext : public SDBObject
   {
      public:
         _dpsOplistContext() = default ;
         ~_dpsOplistContext() = default ;
         _dpsOplistContext( const _dpsOplistContext & ) = delete ;
         _dpsOplistContext &operator=( const _dpsOplistContext & ) = delete ;

      public:
         void reset() ;
         OSS_INLINE BOOLEAN hasBuildingOpl() const { return 0 < _size ; }
         OSS_INLINE UINT32 getBuildingOplNum() const { return _size ; }
         INT32 startNewOpl() ;
         
      public:/// should ensure has building opl first
         BOOLEAN isFreshOpl() const ;
         BOOLEAN isOplRollingBack() const ;
         void setOplRollingBack() ;
         DPS_LSN getOplLSN() const ;
         DPS_LSN getPreNodeLSN() const ;

         /// never can be failed
         void push( const DPS_LSN &lsn ) ;
         void completeOpl(dpsOplBuildingCtx *result=nullptr) ;
         void terminateFreshOpl() ;
         
      private:
         static const UINT32 _MAX_BUILDING_OPL_NUM = 2 ;
         using _BUILDING_CTX_CONTAINER = std::array<dpsOplBuildingCtx, _MAX_BUILDING_OPL_NUM> ;

      private:
         void _resetCurrentCtx();

      private:
         UINT32 _size = 0 ;
         _BUILDING_CTX_CONTAINER _bcc ;
         dpsOplBuildingCtx *_current = nullptr ;
   };//class _dpsOplistContext
   using dpsOplistContext = class _dpsOplistContext; 
} // namespace engine


#endif//DPS_OPLIST_CONTEXT_HPP__