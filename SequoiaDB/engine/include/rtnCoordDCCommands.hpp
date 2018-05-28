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

   Source File Name = rtnCoordDCCommands.hpp

   Descriptive Name = Runtime Coord Common

   When/how to use: this program may be used on binary and text-formatted
   versions of runtime component. This file contains code logic for
   common functions for coordinator node.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          02/11/15    XJH Init
   Last Changed =

*******************************************************************************/

#ifndef RTNCOORD_DC_COMMANDS_HPP__
#define RTNCOORD_DC_COMMANDS_HPP__

#include "rtnCoordCommands.hpp"

namespace engine
{
   /*
      rtnCoordAlterDC define
   */
   class rtnCoordAlterDC : public rtnCoordCommand
   {
      public:
         virtual INT32 execute( MsgHeader *pMsg,
                                pmdEDUCB *cb,
                                INT64 &contextID,
                                rtnContextBuf *buf ) ;

      protected:
         INT32       _executeByNodes( MsgHeader *pMsg,
                                      pmdEDUCB *cb,
                                      CoordGroupList &groupLst,
                                      const CHAR *pAction,
                                      rtnContextBuf *buf ) ;

         INT32       _executeByGroups( MsgHeader *pMsg,
                                       pmdEDUCB *cb,
                                       CoordGroupList &groupLst,
                                       const CHAR *pAction,
                                       rtnContextBuf *buf ) ;

   } ;

   /*
      rtnCoordGetDCInfo define
   */
   class rtnCoordGetDCInfo : public rtnCoordCMDQueryBase
   {
   protected:
      virtual INT32 _preProcess( rtnQueryOptions &queryOpt,
                                 string &clName,
                                 BSONObj &outSelector ) ;
   } ;

}

#endif // RTNCOORD_DC_COMMANDS_HPP__

