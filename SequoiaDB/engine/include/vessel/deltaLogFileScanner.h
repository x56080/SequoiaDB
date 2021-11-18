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

   Source File Name = deltaLogFileScanner.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_DELTA_LOG_FILE_SCANNER_H_
#define VESSEL_DELTA_LOG_FILE_SCANNER_H_

#include "vessel/deltaLogFileDef.h"
#include "vessel/partialImpCache.h"

namespace engine
{
namespace vessel
{
   class deltaLogFileScanner : public SDBObject
   {
      public:
         deltaLogFileScanner(){}
         ~deltaLogFileScanner(){}
         deltaLogFileScanner(const deltaLogFileScanner &) = delete;
         deltaLogFileScanner &operator=(const deltaLogFileScanner &) = delete;

      public:
         INT32 open(const deltaLogFile *file);
         void close();

         slice getCurrent(PAGE_ID &imp, UINT32 &offset)const;
         BOOLEAN hasMore()const;
         INT32 next();

      private:
         const deltaLogFile *_file = NULL;
         deltaLogFileHead _header;
         UINT32 _pos = 0;
   };//class deltaLogFileScanner
} // namespace vessel

} // namespace engine


#endif//VESSEL_DELTA_LOG_FILE_SCANNER_H_