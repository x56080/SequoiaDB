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

   Source File Name = rtnDetectDeadlock.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          08/15/2021  JT  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef RTN_DETECT_DEADLOCK_HPP_
#define RTN_DETECT_DEADLOCK_HPP_

#include "rtnFetchBase.hpp"
#include "dpsDeadlockDetector.hpp"

using namespace bson ;

namespace engine
{
   typedef ossPoolList< BSONObj >             RTN_DEADLOCK_RESULT_LIST ;
   typedef RTN_DEADLOCK_RESULT_LIST::iterator RTN_DEADLOCK_RESULT_LIST_IT ;

   /*
      _rtnDetectDeadlock 
   */
   class _rtnDetectDeadlock : public _IRtnMonProcessor
   {
   public:
      _rtnDetectDeadlock() ;
      virtual ~_rtnDetectDeadlock() ;
   
      virtual INT32   pushIn( const BSONObj &obj ) ;
      virtual INT32   output( BSONObj &obj, BOOLEAN &hasOut ) ;

      virtual INT32   done( BOOLEAN &hasOut ) ;
      virtual BOOLEAN eof() const ;

   protected:
      SDB_ROLE                 _dbRole ;
      DPS_TRANS_WAIT_SET       _waitInfoSet ;
      DPS_TRANS_RELATEDID_MAP  _relatedIDMap ;
      RTN_DEADLOCK_RESULT_LIST _deadlockResList ;
      BOOLEAN                  _eof ;
      BOOLEAN                  _hasDone ;
   };
   typedef _rtnDetectDeadlock rtnDetectDeadlock ; 
   typedef utilSharePtr< _rtnDetectDeadlock > rtnDetectDeadlockPtr ;
}

#endif // RTN_DETECT_DEADLOCK_HPP_
