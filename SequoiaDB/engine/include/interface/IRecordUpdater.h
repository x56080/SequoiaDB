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

   Source File Name = IRecordUpdater.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef VESSEL_I_RECORD_UPDATER_H_
#define VESSEL_I_RECORD_UPDATER_H_

#include "ossMemPool.hpp"

namespace engine
{
   class IRecordUpdater : public SDBObject
   {
      public:
         IRecordUpdater(){}
         virtual ~IRecordUpdater(){}
         IRecordUpdater(const IRecordUpdater &) = delete;
         IRecordUpdater &operator=(const IRecordUpdater &) = delete;

      public:
         virtual INT32 update(UINT32 size,
                              const CHAR *data) = 0;

         virtual BOOLEAN done()const = 0;
         virtual void clearResult() = 0;

      public:/// must be done
         virtual BOOLEAN nothingUpdated()const = 0;
         virtual const CHAR *getResultRecord()const = 0;
         virtual UINT32 getResultRecordSize()const = 0;
         virtual BOOLEAN isWholeRecordReset()const = 0;
         virtual void dumpUpdatedFields(ossPoolVector<const CHAR *> &fields)const = 0;
   };//class IRecordUpdater

} // namespace engine


#endif//VESSEL_I_RECORD_UPDATER_H_