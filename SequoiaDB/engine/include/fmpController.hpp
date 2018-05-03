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

   Source File Name = fmpController.hpp

   Descriptive Name =

   When/how to use:

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          06/19/2013  YW  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef FMPCONTROLLER_HPP_
#define FMPCONTROLLER_HPP_

#include "core.hpp"
#include "ossIO.hpp"
#include "../bson/bson.h"

using namespace bson ;

class _fmpVM ;

class _fmpController
{
public:
   _fmpController() ;
   virtual ~_fmpController() ;

public:
   INT32 run() ;

private:
   INT32 _runLoop() ;

   INT32 _handleOneLoop( const BSONObj &obj,
                         INT32 step ) ;

   INT32 _readMsg( BSONObj &msg ) ;

   INT32 _writeMsg( const BSONObj &msg ) ;

   INT32 _createVM( SINT32 type ) ;

   void _clear() ;

private:
   OSSFILE _in ;
   OSSFILE _out ;
   _fmpVM *_vm ;
   CHAR *_inBuf ;
   UINT32 _inBufSize ;
   INT32  _step ;
} ;

typedef class _fmpController fmpController ;

#endif

