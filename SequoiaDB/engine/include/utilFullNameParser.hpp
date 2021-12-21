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

   Source File Name = utilFullNameParser.hpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef SDB_UTIL_FULL_NAME_PARSER_HPP_
#define SDB_UTIL_FULL_NAME_PARSER_HPP_

#include "dms.hpp"

namespace engine
{
   class utilFullNameParser : public SDBObject
   {
      public:
         utilFullNameParser(){}
         ~utilFullNameParser(){}

      public:
         BOOLEAN parse(const CHAR *fullName, const CHAR **clName);
         OSS_INLINE const CHAR *getCSName()const {return _csName;}

      private:
         CHAR _csName[DMS_COLLECTION_SPACE_NAME_SZ + 1] = {};
   };//class utilFullNameParser
} // namespace engine


#endif//SDB_UTIL_FULL_NAME_PARSER_HPP_