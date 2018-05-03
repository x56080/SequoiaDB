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

   Source File Name = pmdProc.hpp

   Descriptive Name = pmdProc

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains main function for sdbcm,
   which is used to do cluster managing.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          12/26/2013  JHL  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef PMDPROC_HPP_
#define PMDPROC_HPP_

#include "ossTypes.h"
#include "oss.hpp"

namespace engine
{
   /*
      iPmdProc define
   */
   class _iPmdProc : public SDBObject
   {
   public:
      _iPmdProc() ;
      virtual ~_iPmdProc() ;
      virtual INT32 regSignalHandler() ;

   public:
      static BOOLEAN isRunning() ;
      static void stop( INT32 sigNum ) ;

   private:
      static BOOLEAN                _isRunning ;

   } ;
   typedef _iPmdProc iPmdProc ;

}

#endif // PMDPROC_HPP_

