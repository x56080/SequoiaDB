/*******************************************************************************


   Copyright (C) 2011-2018 SequoiaDB Ltd.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU Affero General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.

   Source File Name = builtinRecordUpdater.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

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
