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

   Source File Name = rtnContextSP.hpp

   Descriptive Name = RunTime Store Procedure Context Header

   When/how to use: this program may be used on binary and text-formatted
   versions of Runtime component. This file contains structure for Runtime
   Context.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          5/26/2017   David Li  Split from rtnContext.hpp

   Last Changed =

*******************************************************************************/
#ifndef RTN_CONTEXT_SP_HPP_
#define RTN_CONTEXT_SP_HPP_

#include "rtnContext.hpp"
#include "spdSession.hpp"

namespace engine
{
   /*
      _rtnContextSP OSS_INLINE functions
   */
   class _dmsStorageUnit ;

   class _rtnContextSP : public _rtnContextBase
   {
      DECLARE_RTN_CTX_AUTO_REGISTER( _rtnContextSP )
   public:
      _rtnContextSP( INT64 contextID, UINT64 eduID ) ;
      virtual ~_rtnContextSP() ;

   public:
      virtual const CHAR*      name() const ;
      virtual RTN_CONTEXT_TYPE getType () const ;
      virtual _dmsStorageUnit* getSU () { return NULL ; }
      INT32 open( _spdSession *sp ) ;

   protected:
      virtual INT32   _prepareData( _pmdEDUCB *cb ) ;

   private:
      _spdSession *_sp ;
   } ;

   typedef class _rtnContextSP rtnContextSP ;
}

#endif /* RTN_CONTEXT_SP_HPP_ */

