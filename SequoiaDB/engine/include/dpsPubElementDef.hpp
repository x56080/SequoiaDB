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

   Source File Name = dpsPubElementDef.hpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef DPS_PUB_ELEMENT_DEF_HPP__
#define DPS_PUB_ELEMENT_DEF_HPP__

#include "dpsLogDef.hpp"

namespace engine
{
#pragma pack(4)
   struct dpsOplNodeEle
   {
      dpsOplNodeEle() = default ;
      dpsOplNodeEle(const DPS_LSN &lsn, const DPS_LSN &pre):
      oplLSN(lsn),
      preLSN(pre) {}

      DPS_LSN oplLSN;
      DPS_LSN preLSN;
   };

   struct dpsOplRollbackInfoEle
   {
      dpsOplRollbackInfoEle() = default;
      dpsOplRollbackInfoEle(UINT64 offset, UINT32 version):
      targetLSN(offset, version){}
      dpsOplRollbackInfoEle(const DPS_LSN &target):
      targetLSN(target){}
      DPS_LSN targetLSN;
   };

   struct dpsPageAddrEle
   {
      INT32 lpid = -1;
      INT64 gpid = -1;
   };

#pragma pack()
} // namespace engine


#endif//DPS_PUB_ELEMENT_DEF_HPP__