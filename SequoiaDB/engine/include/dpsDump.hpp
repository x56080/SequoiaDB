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

   Source File Name = dpsDump.hpp

   Descriptive Name = Data Management Service Storage Unit Header

   When/how to use: this program may be used on binary and text-formatted
   versions of data management component. This file contains structure for
   DMS storage unit and its methods.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          31/12/2013  XJH Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef DPSDUMP_HPP__
#define DPSDUMP_HPP__

#include "oss.hpp"
#include "core.hpp"

namespace engine
{

   /*
      _dpsDump define
   */
   class _dpsDump : public SDBObject
   {
      public:
         _dpsDump () {}
         ~_dpsDump () {}

      public:

         static UINT32 dumpLogFileHead ( CHAR *inBuf, UINT32 inSize,
                                         CHAR *outBuf, UINT32 outSize,
                                         UINT32 options ) ;

   } ;
   typedef _dpsDump dpsDump ;

}

#endif //DPSDUMP_HPP__

