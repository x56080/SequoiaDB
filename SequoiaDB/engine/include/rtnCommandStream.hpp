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
