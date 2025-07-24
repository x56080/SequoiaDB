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

   Source File Name = builtinRecordUpdater.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef VESSEL_BUILTIN_RECORD_UPDATER_H_
#define VESSEL_BUILTIN_RECORD_UPDATER_H_

#include "interface/IRecordUpdater.h"
#include "mthModifier.hpp"

namespace engine
{
namespace vessel
{
   class bsonRecordUpdater : public IRecordUpdater
   {
      public:
         bsonRecordUpdater(){}
         virtual ~bsonRecordUpdater(){}   

      public:
         virtual INT32 update(UINT32 size,
                              const CHAR *data);

         virtual BOOLEAN done()const {return !_result.isEmpty();}

         virtual void clearResult();

         virtual BOOLEAN nothingUpdated()const;
         virtual const CHAR *getResultRecord()const;
         virtual UINT32 getResultRecordSize()const;
         virtual BOOLEAN isWholeRecordReset()const;
         virtual void dumpUpdatedFields(ossPoolVector<const CHAR *> &fields)const;

      public:
         void setModifier(mthModifier *modifier);

      private:
         mthModifier *_modifier = NULL;
         bson::BSONObj _result;
         bson::BSONObj _changed;
   };//class bsonRecordUpdater
} // namespace vessel

} // namespace engine


#endif//VESSEL_BUILTIN_RECORD_UPDATER_H_
