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

   Source File Name = dpsWriteContext.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#include "dpsWriteContext.hpp"
#include "pdTrace.hpp"
#include "pmdEDU.hpp"

namespace engine
{
   dpsWriteContext::dpsWriteContext(IExecutor *executor,
                                    const dpsWriteRequest *req,
                                    const dpsWriteOptions *o):
   _executor(executor),
   _req(req),
   _o(o)
   {
      _init();
   }

   UINT32 dpsWriteContext::getRecordBodySizeAuto() const
   {
      return _compressedRecord.isValid() ?
             _compressedRecord.getSize() : _req->getElementDataSize() ;
   }

   utilSlice dpsWriteContext::getRecordBodyData() const
   {
      return _compressedRecord.isValid() ?
             _compressedRecord.getSlice() : _req->getElements().getSlice() ;
   }

   void dpsWriteContext::_init()
   {
      SDB_ASSERT(nullptr != _req && nullptr != _o, "can not be invalid");
      _irreversible = (nullptr == _executor ||
                       !_executor->getTransID().isGlobTrans() ||
                       !_o->transEnabled);
      

   }
} // namespace engine
