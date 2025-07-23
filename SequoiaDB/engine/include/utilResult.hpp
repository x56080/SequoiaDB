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

   Source File Name = utilResult.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          02/18/2019   LYB Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef UTIL_RESULT_HPP_
#define UTIL_RESULT_HPP_

#include "oss.hpp"

namespace engine
{
   class utilResult : public SDBObject
   {
   public:
      utilResult() ;
      virtual ~utilResult() ;
   } ;

   class utilWriteResult : public utilResult
   {
   public:
      utilWriteResult() ;
      virtual ~utilWriteResult() ;
   } ;
}

#endif /* UTIL_RESULT_HPP_ */



