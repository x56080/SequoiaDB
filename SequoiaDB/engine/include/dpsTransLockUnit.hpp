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

   Source File Name = dpsTransLockUnit.hpp

   Descriptive Name = Operating System Services Types Header

   When/how to use: this program may be used on binary and text-formatted
   versions of OSS component. This file contains declare for data types used in
   SequoiaDB.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/14/2012  YW  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef DPSTRANSLOCKUNIT_HPP_
#define DPSTRANSLOCKUNIT_HPP_

#include "pmdEDU.hpp"
#include <map>

namespace engine
{
   typedef std::map< UINT32, DPS_TRANSLOCK_TYPE >    dpsTransLockRunList;
   class dpsTransLockUnit : public SDBObject
   {
   public:
      dpsTransLockUnit()
      {
         _pWaitCB = NULL ;
      }
      ~dpsTransLockUnit(){};
   public:
      _pmdEDUCB   *_pWaitCB;
      dpsTransLockRunList  _runList;
   };
}

#endif // DPSTRANSLOCKUNIT_HPP_
