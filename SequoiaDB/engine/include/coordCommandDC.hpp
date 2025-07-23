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

   Source File Name = coordCommandDC.hpp

   Descriptive Name = Coord Common

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
#ifndef COORD_COMMAND_DC_HPP__
#define COORD_COMMAND_DC_HPP__

#include "coordCommandCommon.hpp"
#include "coordFactory.hpp"

using namespace bson ;

namespace engine
{
   /*
      _coordAlterDC define
   */
   class _coordAlterDC : public _coordCommandBase
   {
      COORD_DECLARE_CMD_AUTO_REGISTER() ;
      public:
         _coordAlterDC() ;
         virtual ~_coordAlterDC() ;

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
   typedef _coordAlterDC coordAlterDC ;

   /*
      _coordGetDCInfo define
   */
   class _coordGetDCInfo : public _coordCMDQueryBase
   {
      COORD_DECLARE_CMD_AUTO_REGISTER() ;
      public:
         _coordGetDCInfo() ;
         virtual ~_coordGetDCInfo() ;

      protected:
         virtual INT32 _preProcess( rtnQueryOptions &queryOpt,
                                    string &clName,
                                    BSONObj &outSelector ) ;
   } ;
   typedef _coordGetDCInfo coordGetDCInfo ;

}

#endif // COORD_COMMAND_DC_HPP__

