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

   Source File Name = utilELEncryptor.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          03/30/2023  ZHY Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef UTIL_ELCRYPTOR_HPP_
#define UTIL_ELCRYPTOR_HPP_
#include "oss.hpp"

namespace engine
{
   // Equal-Length Cryptor
   class _utilELCryptor : public SDBObject
   {
      public:
         virtual ~_utilELCryptor() {} ;
         virtual INT32 encrypt( const UINT8 *in, UINT32 ilen, UINT8 *out, UINT32 *olen = NULL ) const = 0 ;
         virtual INT32 decrypt( const UINT8 *in, UINT32 ilen, UINT8 *out, UINT32 *olen = NULL ) const = 0 ;
   } ;
   typedef _utilELCryptor utilELCryptor ;
}
#endif