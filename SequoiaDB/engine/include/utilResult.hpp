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

   Source File Name = utilResult.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          02/18/2019   LYB Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef UTIL_RESULT_HPP_
#define UTIL_RESULT_HPP_

#include "oss.hpp"

namespace engine
{
   class utilResult : public SDBObject
   {
   public:
      utilResult() ;
      virtual ~utilResult() ;
   } ;

   class utilWriteResult : public utilResult
   {
   public:
      utilWriteResult() ;
      virtual ~utilWriteResult() ;
   } ;
}

#endif /* UTIL_RESULT_HPP_ */



