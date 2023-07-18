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

   Source File Name = coordCommandStream.hpp

   Descriptive Name = Coord Stream Commands

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

#ifndef COORD_CMD_STREAM_HPP__
#define COORD_CMD_STREAM_HPP__

#include "coordCommandBase.hpp"
#include "coordFactory.hpp"
#include "coordCommandCommon.hpp"

namespace engine
{

   /*
      _coordCMDWatch define
    */
   class _coordCMDWatch : public _coordCommandBase
   {
      COORD_DECLARE_CMD_AUTO_REGISTER() ;

   public:
      _coordCMDWatch() ;
      virtual ~_coordCMDWatch() ;

   protected:
      virtual INT32 execute( MsgHeader *pMsg,
                             pmdEDUCB *cb,
                             INT64 &contextID,
                             rtnContextBuf *buf ) ;
   } ;

   typedef class _coordCMDWatch coordCMDWatch ;

}

#endif // COORD_CMD_STREAM_HPP__
