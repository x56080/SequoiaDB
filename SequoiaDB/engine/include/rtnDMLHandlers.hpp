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

   Source File Name = rtnDMLHandlers.hpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef SDB_RTN_DML_HANDLERS_HPP_
#define SDB_RTN_DML_HANDLERS_HPP_

#include "rtnHandler.hpp"
#include "utilUniqueID.hpp"
#include "dmsEngineOptions.hpp"
#include "utilInsertResult.hpp"

namespace engine
{
   class rtnInsertHandler : public rtnHandler
   {
      public:
         rtnInsertHandler(){}
         virtual ~rtnInsertHandler(){}

      public:
         virtual INT32 launch(pmdEDUCB *cb);

      public:
         void init(const CHAR *name,
                   const utilCLUniqueID &uniqueId,
                   const bson::BSONObj &rows,
                   UINT32 rowCount);
         OSS_INLINE void setResultPtr(utilInsertResult *result)
         {
            _result = result;
         }

      private:
         const CHAR *_fullName = NULL;
         utilCLUniqueID _uniqueId = UTIL_UNIQUEID_NULL;
         bson::BSONObj _rows;
         UINT32 _count = 0;
         utilInsertResult *_result = NULL;
   };//class rtnInsertHandler
} // namespace engine


#endif//SDB_RTN_DML_HANDLERS_HPP_