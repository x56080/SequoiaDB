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

   Source File Name = rtnExtOprDef.hpp

   Descriptive Name = External Operation Definitions.

   When/how to use: this program may be used on binary and text-formatted
   versions of data management component. This file contains structure for
   DMS storage unit and its methods.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          15/11/2017  YSD Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef RTN_EXTOPR_DEF_HPP__
#define RTN_EXTOPR_DEF_HPP__

namespace engine
{
   enum _rtnExtOprType
   {
      RTN_EXT_INVALID = 0,
      RTN_EXT_INSERT = 1,
      RTN_EXT_DELETE,
      RTN_EXT_UPDATE,
      RTN_EXT_UPDATE_WITH_ID,
      RTN_EXT_DUMMY = 10000
   } ;
}

#endif /* RTN_EXTOPR_DEF_HPP__ */

