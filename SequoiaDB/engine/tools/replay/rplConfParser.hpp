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

   Source File Name = rplConfParser.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          01/04/2019  Linyoubin  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef REPLAY_CONFPARSER_HPP_
#define REPLAY_CONFPARSER_HPP_

#include "utilJsonFile.hpp"
#include "../bson/bson.hpp"
#include "oss.hpp"

using namespace std ;
using namespace bson;
using namespace engine ;

namespace replay
{
   class rplConfParser : public SDBObject
   {
   public:
      rplConfParser() ;
      ~rplConfParser() ;

   public:
      INT32 init( const CHAR *confFile ) ;
      string getType() const ;
      BSONObj getConf() const ;

   private:
      BSONObj _conf ;
   } ;
}

#endif  /* REPLAY_CONFPARSER_HPP_ */


