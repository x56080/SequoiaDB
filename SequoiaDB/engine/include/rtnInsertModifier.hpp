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

   Source File Name = rtnInsertModifier.hpp

   Descriptive Name = Insert hint modifier header

   When/how to use: this program may be used on binary and text-formatted
   versions of Runtime component. This file contains declare for runtime
   functions.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          05/15/2022  YSD  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef RTN_INSERTMODIFIER_HPP_
#define RTN_INSERTMODIFIER_HPP_

#include "rtnHintModifier.hpp"

namespace engine
{
   class _rtnInsertModifier: public _rtnHintModifier
   {
   public:
      _rtnInsertModifier() {}
      ~_rtnInsertModifier() {}

      const BSONObj& getUpdator() const ;

   protected:
      INT32 _onInit() ;
   } ;
   typedef _rtnInsertModifier rtnInsertModifier ;
}

#endif /* RTN_INSERTMODIFIER_HPP_ */
