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

   Source File Name = pmdIProcessor.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          01/12/2014  Lin Youbin  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef PMD_IPROCESSOR_HPP_
#define PMD_IPROCESSOR_HPP_

#include "oss.hpp"
#include "sdbInterface.hpp"
#include "rtnContextBuff.hpp"

namespace engine
{
   /*
      SDB_PROCESSOR_TYPE define
   */
   enum SDB_PROCESSOR_TYPE
   {
      SDB_PROCESSOR_DATA        = 1,      // data node processor
      SDB_PROCESSOR_COORD,                // coord node processor
      SDB_PROCESSOR_CATA,
      SDB_PROCESSOR_OM,

      SDB_PROCESSOR_MAX
   } ;

   /*
      _IProcessor define
   */
   class _IProcessor : public SDBObject
   {
      public:
         _IProcessor() {}
         virtual ~_IProcessor() {}

      public:

         virtual INT32                 processMsg( MsgHeader *msg,
                                                   rtnContextBuf &contextBuff,
                                                   INT64 &contextID,
                                                   BOOLEAN &needReply,
                                                   BOOLEAN &needRollback,
                                                   BSONObjBuilder &builder ) = 0 ;

         virtual INT32                 doRollback() = 0 ;
         virtual INT32                 doCommit() = 0 ;

         virtual const CHAR*           processorName() const = 0 ;
         virtual SDB_PROCESSOR_TYPE    processorType() const = 0 ;

         virtual ISession*             getSession() = 0 ;

      protected:
         virtual void                  _onAttach () {}
         virtual void                  _onDetach () {}

   } ;
   typedef _IProcessor IProcessor ;

}

#endif /*PMD_IPROCESSOR_HPP_*/

