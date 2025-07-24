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

   Source File Name = IReadUnit.hpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          11/20/2023  HGM Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef SDB_I_READ_UNIT_HPP_
#define SDB_I_READ_UNIT_HPP_

#include "sdbInterface.hpp"
#include "interface/IStorageSession.hpp"
#include "utilPooledAutoPtr.hpp"
#include "utilPooledObject.hpp"

namespace engine
{

   /*
      IReadUnit define
    */
   class IReadUnit : public _utilPooledObject
   {
   public:
      IReadUnit() = default ;
      virtual ~IReadUnit() = default ;
      IReadUnit( const IReadUnit &o ) = delete ;
      IReadUnit &operator =( const IReadUnit& ) = delete ;
   } ;

}

#endif // SDB_I_READ_UNIT_HPP_