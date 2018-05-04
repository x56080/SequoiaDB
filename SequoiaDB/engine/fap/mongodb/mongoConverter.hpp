/*******************************************************************************


   Copyright (C) 2011-2018 SequoiaDB Ltd.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

   http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

   Source File Name = mongoConverter.hpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains functions for agent processing.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          01/27/2015  LZ  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef _SDB_MONGO_CONVERTER_HPP_
#define _SDB_MONGO_CONVERTER_HPP_

#include "util.hpp"
#include "oss.hpp"
#include "parser.hpp"
#include "commands.hpp"
#include "msg.hpp"

class mongoConverter : public baseConverter
{
public:
   mongoConverter()
   {
      _bigEndian = checkBigEndian() ;
      _parser.setEndian( _bigEndian ) ;
   }

   ~mongoConverter()
   {
   }

   BOOLEAN isBigEndian() const
   {
      return _bigEndian ;
   }

   const UINT32 getOpType()
   {
      return _parser.currentOption() ;
   }

   msgParser& getParser()
   {
      return _parser ;
   }

   // virtual function for baseConverter
   virtual INT32 convert( msgBuffer &out ) ;
   INT32 reConvert( msgBuffer &out, MsgOpReply *reply ) ;

private:
   BOOLEAN _bigEndian ;
   msgParser _parser ;
};
#endif
