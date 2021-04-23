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

   Source File Name = copyOnWriteDeltaLog.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_COPY_ON_WRITE_DELTA_LOG_H_
#define VESSEL_COPY_ON_WRITE_DELTA_LOG_H_

#include "ossMemPool.hpp"
#include "vessel/strSlice.h"
#include "vessel/vesselFileName.h"

namespace engine
{
namespace vessel
{
   class requestContext;
   class copyOnWriteDeltaLogFile;

   class copyOnWriteDeltaLog : public SDBObject
   {
      public:
         copyOnWriteDeltaLog();
         ~copyOnWriteDeltaLog();

      public:
         INT32 openFile(requestContext *context,
                        const strSlice &fullPath,
                        const vesselFileName &fn);

      private:
         typedef ossPoolList<copyOnWriteDeltaLogFile*> _FILE_LIST;
      
      private:
         _FILE_LIST _fileList;
   };
}//namespace vessel
}//namespace engine

#endif//VESSEL_COPY_ON_WRITE_DELTA_LOG_H_