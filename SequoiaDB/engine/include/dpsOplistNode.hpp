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

   Source File Name = dpsOplistNode.hpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef DPS_OPLIST_NODE_HPP__
#define DPS_OPLIST_NODE_HPP__

#include "dpsLogRecord.hpp"
#include "dpsRecordElements.hpp"
#include "dpsPubElementDef.hpp"

namespace engine
{
   class _dpsOplistNode : public SDBObject
   {
      public:
         _dpsOplistNode( utilUniqueBuffer && ) ;
         _dpsOplistNode( const _dpsOplistNode & ) = delete ;
         _dpsOplistNode &operator=( const _dpsOplistNode & ) = delete ;
         _dpsOplistNode( _dpsOplistNode && ) noexcept ;
         _dpsOplistNode &operator=( _dpsOplistNode && ) noexcept ;

      public:
         OSS_INLINE void reset()
         {
            _rh = nullptr ;
            _rb.reset() ;
            _buf.reset() ;
         }
         OSS_INLINE BOOLEAN isValid() const { return nullptr != _rh ; }
         OSS_INLINE const utilUniqueBuffer &getRecordBuf() const { return _buf ; }
         OSS_INLINE const dpsLogRecordHeader *getHeader() const { return _rh ; }
         OSS_INLINE const dpsRecordElements &getBody() const { return _rb ; }
         OSS_INLINE DPS_LSN_OFFSET getLsnOffset() const { return _rh->_lsn ; }
         OSS_INLINE DPS_LSN getLSN() const
         {
            return DPS_LSN( _rh->_lsn, _rh->_version ) ;
         }

      public:
         DPS_OPL_NODE_TYPE getType() const ;
         BOOLEAN hasOplNodeInfo() const ;
         const dpsOplNodeEle *getOplNodeInfo() const;
         BOOLEAN hasOplRollbackInfo() const ;
         const dpsOplRollbackInfoEle *getOplRollbackInfo() const ;

      private:
         BOOLEAN _init( const utilUniqueBuffer &buf ) ;
         
      private:
         const dpsLogRecordHeader *_rh = nullptr ;
         dpsRecordElements _rb ;
         utilUniqueBuffer _buf ; 
   } ;// class _dpsOplistNode
   using dpsOplistNode = class _dpsOplistNode ;
} // namespace engine


#endif//DPS_OPLIST_NODE_HPP__
