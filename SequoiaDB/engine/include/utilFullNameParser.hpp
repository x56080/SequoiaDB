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

   Source File Name = utilFullNameParser.hpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef SDB_UTIL_FULL_NAME_PARSER_HPP_
#define SDB_UTIL_FULL_NAME_PARSER_HPP_

#include "dms.hpp"
#include "ossMemPool.hpp"

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
         static ossPoolString buildFullName(const CHAR *csName,
                                            const CHAR *clName);

      private:
         CHAR _csName[DMS_COLLECTION_SPACE_NAME_SZ + 1] = {};
   };//class utilFullNameParser
} // namespace engine


#endif//SDB_UTIL_FULL_NAME_PARSER_HPP_