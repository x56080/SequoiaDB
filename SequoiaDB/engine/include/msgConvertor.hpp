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

   Source File Name = msgConvertor.hpp

   Descriptive Name = Message convertor interface

   When/how to use: this program may be used on binary and text-formatted
   versions of Messaging component. This file contains message structure for
   client-server communication.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          11/11/2021  YSD Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef MSG_CONVERTOR_HPP__
#define MSG_CONVERTOR_HPP__

#include "utilPooledObject.hpp"
namespace engine
{
   /**
    * Message convertor interface.
    */
   class _IMsgConvertor : public utilPooledObject
   {
   public:
      virtual ~_IMsgConvertor() {}

      /**
       * Reset the convertor, called before reusing the convertor.
       */
      virtual void reset( BOOLEAN releaseBuff ) = 0 ;

      /**
       * The first part being pushed into the convertor should always be the .
       * header.
       */
      virtual INT32 push( const CHAR *data, UINT32 size ) = 0 ;

      virtual INT32 output( CHAR *&data, UINT32 &len ) = 0 ;
   } ;
   typedef _IMsgConvertor IMsgConvertor ;
}

#endif /* MSG_CONVERTOR_HPP__ */
