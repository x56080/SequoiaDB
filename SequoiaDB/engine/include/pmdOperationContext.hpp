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

   Source File Name = pmdOperationContext.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          11/20/2023  HGM Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef PMD_OPERATION_CONTEXT_HPP_
#define PMD_OPERATION_CONTEXT_HPP_

#include "ossUtil.hpp"
#include "sdbInterface.hpp"
#include "interface/IOperationContext.hpp"

namespace engine
{

   /*
      _pmdOperationContext define
    */
   class _pmdOperationContext : public IOperationContext
   {
   public:
      _pmdOperationContext() = default ;
      virtual ~_pmdOperationContext() = default ;
      _pmdOperationContext( const _pmdOperationContext &o ) = delete ;
      _pmdOperationContext &operator =( const _pmdOperationContext &o ) = delete ;

      virtual IPersistUnit *getPersistUnit()
      {
         return _persistUnit.get() ;
      }

      virtual void setPersistUnit( std::unique_ptr<IPersistUnit> persistUnit )
      {
         _persistUnit = std::move( persistUnit ) ;
      }

      virtual IReadUnit *getReadUnit()
      {
         return _readUnit ;
      }

      virtual void setReadUnit( IReadUnit *readUnit )
      {
         _readUnit = readUnit ;
      }

   protected:
      std::unique_ptr<IPersistUnit> _persistUnit ;
      IReadUnit *_readUnit = nullptr ;
   } ;

   typedef class _pmdOperationContext pmdOperationContext ;

}

#endif // PMD_OPERATION_CONTEXT_HPP_