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

   Source File Name = spdFuncDownloader.hpp

   Descriptive Name =

   When/how to use:

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          06/19/2013  YW  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef SPDFUNCDOWNLOADER_HPP_
#define SPDFUNCDOWNLOADER_HPP_

#include "core.hpp"
#include "../bson/bson.h"

using namespace bson ;

namespace engine
{
   class _pmdEDUCB ;

   class _spdFuncDownloader : public SDBObject
   {
   public:
      _spdFuncDownloader(){}
      virtual ~_spdFuncDownloader() {}

   public:
      virtual INT32 next( BSONObj &func ) = 0 ;

      virtual INT32 download( const BSONObj &match ) = 0 ;

   } ;
   typedef class _spdFuncDownloader spdFuncDownloader ;
}

#endif

