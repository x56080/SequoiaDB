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

   Source File Name = dpsOplistContext.hpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
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