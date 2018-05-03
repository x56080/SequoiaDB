/*******************************************************************************


   Copyright (C) 2011-2018 SequoiaDB Ltd.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

   http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

   Source File Name = ixm.hpp

   Descriptive Name = Index Management Header

   When/how to use: this program may be used on binary and text-formatted
   versions of index management component. This file contains structure for
   Index Record ID (IRID) and Index Control Block.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          03/07/2014  TW  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef IXM_COMMON_HPP_
#define IXM_COMMON_HPP_

#include "core.hpp"
#include "../bson/bson.h"

using namespace std ;
using namespace bson ;

#define IXM_NAME_FIELD              IXM_FIELD_NAME_NAME
#define IXM_KEY_FIELD               IXM_FIELD_NAME_KEY
#define IXM_V_FIELD                 IXM_FIELD_NAME_V
#define IXM_UNIQUE_FIELD            IXM_FIELD_NAME_UNIQUE
#define IXM_ENFORCED_FIELD          IXM_FIELD_NAME_ENFORCED
#define IXM_DROPDUP_FIELD           IXM_FIELD_NAME_DROPDUPS
#define IXM_2DRANGE_FIELD           IXM_FIELD_NAME_2DRANGE

namespace engine
{
   /*
      _ixmHashValue define
   */
   union _ixmHashValue
   {
      struct
      {
         UINT32   hash1 ;
         UINT32   hash2 ;
      } columns ;
      UINT64 hash ;
   } ;
   typedef _ixmHashValue ixmHashValue ;

   /*
      Make object's array field hash value
   */
   void ixmMakeHashValue( const BSONObj &obj, const BSONObj &key,
                          ixmHashValue &hashValue ) ;

   void ixmMakeHashValue( const BSONElement &eleArray,
                          ixmHashValue &hashValue ) ;

}

#endif /* IXM_COMMON_HPP_ */

