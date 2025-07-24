/*******************************************************************************

<<<<<<< HEAD
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
=======

   Copyright (C) 2011-2022 SequoiaDB Ltd.

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
>>>>>>> c4064a6f2c2dfdf2b1bf049c2f904b74db0494b2

   Source File Name = rtn.hpp

   Descriptive Name = RunTime Header

   When/how to use: this program may be used on binary and text-formatted
   versions of Runtime component. This file contains declare for runtime
   functions.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          06/08/2022  YSD  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef RTN_OPRHANDLER_HPP_
#define RTN_OPRHANDLER_HPP_

#include "dmsOprHandler.hpp"

namespace engine
{
   class _IRtnOprHandler : public IDmsOprHandler
   {
      public:
         _IRtnOprHandler() {}
         virtual ~_IRtnOprHandler() {}

      public:
         virtual INT32 getShardingKey( const CHAR* clName,
                                       BSONObj &shardingKey ) = 0 ;
   } ;
   typedef _IRtnOprHandler IRtnOprHandler ;
}

#endif /* RTN_OPRHANDLER_HPP_ */
