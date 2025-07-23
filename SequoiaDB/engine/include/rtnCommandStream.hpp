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

   Source File Name = rtnCommandStream.hpp

   Descriptive Name = Runtime Stream Commands

   When/how to use: this program may be used on binary and text-formatted
   versions of runtime component. This file contains code logic for
   common functions for coordinator node.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          06/01/2023  HGM Initial Draft
   Last Changed =

*******************************************************************************/
#ifndef RTN_CMD_STREAM_HPP__
#define RTN_CMD_STREAM_HPP__

#include "rtnCommand.hpp"

namespace engine
{

   /*
      _rtnCMDWatch define
    */
   class _rtnCMDWatch : public _rtnCommand
   {
      DECLARE_CMD_AUTO_REGISTER() ;

   public:
      _rtnCMDWatch() ;
      virtual ~_rtnCMDWatch() ;

      virtual const CHAR *name()
      {
         return NAME_WATCH ;
      }

      virtual RTN_COMMAND_TYPE type()
      {
         return CMD_WATCH ;
      }

      virtual BOOLEAN writable()
      {
         return FALSE ;
      }

      virtual INT32 init( INT32 flags,
                          INT64 numToSkip,
                          INT64 numToReturn,
                          const CHAR *pMatcherBuff,
                          const CHAR *pSelectBuff,
                          const CHAR *pOrderByBuff,
                          const CHAR *pHintBuff ) ;

      virtual INT32 doit( _pmdEDUCB *cb,
                          _SDB_DMSCB *dmsCB,
                          _SDB_RTNCB *rtnCB,
                          _dpsLogWrapper *dpsCB,
                          INT16 w = 1,
                          INT64 *pContextID = NULL ) ;

   private:
      INT32 _checkPrivileges( _pmdEDUCB *cb );

   protected:
      bson::BSONObj _boOptions ;
   } ;

   typedef class _rtnCMDWatch rtnCMDWatch ;

}

#endif // RTN_CMD_STREAM_HPP__
