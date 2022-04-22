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

   Source File Name = IDataJournal.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef SDB_I_DATA_JOURNAL_H_
#define SDB_I_DATA_JOURNAL_H_

#include "sdbInterface.hpp"
#include "dpsDef.hpp"
#include "dpsLogDef.hpp"
#include "dpsJournalPad.hpp"

namespace engine
{
   class IDataJournal : public SDBObject
   {
      public:
         IDataJournal(){}
         virtual ~IDataJournal(){}
         IDataJournal(const IDataJournal &) = delete;
         IDataJournal &operator=(const IDataJournal &) = delete;

      public:
         struct writeOptions : public SDBObject
         {
            BOOLEAN flushImmediately = FALSE;
         };//struct writeOptions

      public:
         virtual DPS_LSN getMinFileLSN() = 0;
         virtual DPS_LSN getMinBufLSN() = 0;
         virtual DPS_LSN getCurrentLSN() = 0;
         virtual DPS_LSN getNextLSN() = 0;
         virtual DPS_LSN getMinUncommitedLSN() = 0;

      public:
         INT32 write(IExecutor *executor,
                     );

   };//class IDataJournal
} // namespace engine


#endif//SDB_I_DATA_JOURNAL_H_