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
>>>>>>> c4064a6f2c2dfdf2b1bf049c2f904b74db0494b2

   Source File Name = coordSequenceOperator.hpp

   Descriptive Name =

   When/how to use:

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          10/10/2020  LSQ Initial Draft

   Last Changed =

*******************************************************************************/
<<<<<<< HEAD
=======

>>>>>>> c4064a6f2c2dfdf2b1bf049c2f904b74db0494b2
#ifndef COORD_SEQUENCE_OPERATOR_HPP__
#define COORD_SEQUENCE_OPERATOR_HPP__

#include "coordOperator.hpp"

namespace engine
{
   /*
      _coordSeqFetchOperator define
   */
   class _coordSeqFetchOperator : public _coordOperator
   {
      public:
         _coordSeqFetchOperator() ;
         virtual ~_coordSeqFetchOperator() ;

         virtual const CHAR* getName() const ;

         virtual INT32 execute( MsgHeader *pMsg,
                                pmdEDUCB *cb,
                                INT64 &contextID,
                                rtnContextBuf *buf ) ;

   } ;
   typedef _coordSeqFetchOperator coordSeqFetchOperator ;
}

#endif // COORD_SEQUENCE_OPERATOR_HPP__
