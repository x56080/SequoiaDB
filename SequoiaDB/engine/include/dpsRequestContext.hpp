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

   Source File Name = dpsRequestContext.hpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef DPS_REQUEST_CONTEXT_HPP__
#define DPS_REQUEST_CONTEXT_HPP__

#include "dpsRequest.hpp"
#include "dpsWriteReqBuilder.hpp"
#include "dpsOplistContext.hpp"

namespace engine
{
   class _dpsRequestContext : public SDBObject
   {
      public:
         _dpsRequestContext() = default ;
         ~_dpsRequestContext() = default ;
         explicit _dpsRequestContext( const dpsWriteOptions &wo,
                                      dpsOplistContext *ctx ) noexcept ;
         _dpsRequestContext( const _dpsRequestContext & ) = delete ;
         _dpsRequestContext &operator=( const _dpsRequestContext & ) = delete ;

      public:
         OSS_INLINE void setWriteOptions( const dpsWriteOptions &o ) { _o = o ;}
         OSS_INLINE dpsWriteOptions &getWriteOptionsToSet() { return _o ;}
         OSS_INLINE const dpsWriteOptions &getWriteOptions() const { return _o ;}

         ///WARNING: _dpsRequestContext does not own the ctx ptr!
         OSS_INLINE void setOplCtx( dpsOplistContext *ctx ) { _oplCtx = ctx ; }
         OSS_INLINE dpsOplistContext *getOplCtx() { return _oplCtx; }
         OSS_INLINE BOOLEAN hasOplCtx() const { return nullptr != _oplCtx ; }
         OSS_INLINE void setToCompleteOpl() { _toCompleteOpl = TRUE ;}
         OSS_INLINE BOOLEAN isToCompleteOpl() const { return _toCompleteOpl ; }
         OSS_INLINE void setOplRollbackTarget( const DPS_LSN &target )
         {
             _oplRollbackTarget = target ;
         }
         OSS_INLINE const DPS_LSN &getOplRollbackTarget() const { return _oplRollbackTarget ; }
         OSS_INLINE BOOLEAN hasOplRollbackTarget() const
         {
            return !_oplRollbackTarget.invalid() ;
         }
         
         OSS_INLINE const dpsWriteRequest &getReq() const { return _req ; }

         OSS_INLINE dpsWriteReqBuilder &getBuilder() { return _builder ; }
         OSS_INLINE const dpsLogRecordHeader &getResult() const { return _result ; }
         OSS_INLINE dpsLogRecordHeader *getResultPtr() { return &_result ; }

      public:
         void reset() ;
         void endToBuildRequest() ;
      
      private:
         dpsWriteOptions _o ;
         dpsOplistContext *_oplCtx = nullptr ;
         BOOLEAN _toCompleteOpl = FALSE ;
         DPS_LSN _oplRollbackTarget ;
         dpsWriteReqBuilder _builder ;
         dpsWriteRequest _req ;
         dpsLogRecordHeader _result ;
         
   };// class _dpsRequestContext
   using dpsRequestContext = class _dpsRequestContext ;
} // namespace engine


#endif//DPS_REQUEST_CONTEXT_HPP__