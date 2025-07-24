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
                                      const CHAR *pAction ) ;

         INT32       _executeByGroups( MsgHeader *pMsg,
                                       pmdEDUCB *cb,
                                       CoordGroupList &groupLst,
                                       const CHAR *pAction ) ;

   } ;

   /*
      rtnCoordGetDCInfo define
   */
   class rtnCoordGetDCInfo : public rtnCoordCMDQueryBase
   {
   protected:
      virtual INT32 _preProcess( rtnQueryOptions &queryOpt,
                                 string &clName ) ;
   } ;

}

#endif // RTNCOORD_DC_COMMANDS_HPP__

