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

   Source File Name = coordCommandDomain.hpp

   Descriptive Name = Coord Commands for Data Manager

   When/how to use: this program may be used on binary and text-formatted
   versions of runtime component. This file contains code logic for
   common functions for coordinator node.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          04/27/2017  XJH Init
   Last Changed =

*******************************************************************************/
#ifndef COORD_COMMAND_DOMAIN_HPP__
#define COORD_COMMAND_DOMAIN_HPP__

#include "coordCommandBase.hpp"
#include "coordCommandData.hpp"
#include "coordFactory.hpp"

using namespace bson ;

namespace engine
{

   /*
      _coordCMDCreateDomain define
   */
   class _coordCMDCreateDomain : public _coordCommandBase
   {
      COORD_DECLARE_CMD_AUTO_REGISTER() ;
      public:
         _coordCMDCreateDomain() ;
         virtual ~_coordCMDCreateDomain() ;

         virtual INT32 execute( MsgHeader *pMsg,
                                pmdEDUCB *cb,
                                INT64 &contextID,
                                rtnContextBuf *buf ) ;
   } ;
   typedef _coordCMDCreateDomain coordCMDCreateDomain ;

   /*
      _coordCMDDropDomain define
   */
   class _coordCMDDropDomain : public _coordCommandBase
   {
      COORD_DECLARE_CMD_AUTO_REGISTER() ;
      public:
         _coordCMDDropDomain() ;
         virtual ~_coordCMDDropDomain() ;

         virtual INT32 execute( MsgHeader *pMsg,
                                pmdEDUCB *cb,
                                INT64 &contextID,
                                rtnContextBuf *buf ) ;
   } ;
   typedef _coordCMDDropDomain coordCMDDropDomain ;

//   /*
//      _coordCMDAlterDomain define
//   */
//   class _coordCMDAlterDomain : public _coordCommandBase
//   {
//      COORD_DECLARE_CMD_AUTO_REGISTER() ;
//      public:
//         _coordCMDAlterDomain() ;
//         virtual ~_coordCMDAlterDomain() ;
//
//         virtual INT32 execute( MsgHeader *pMsg,
//                                pmdEDUCB *cb,
//                                INT64 &contextID,
//                                rtnContextBuf *buf ) ;
//   } ;
//   typedef _coordCMDAlterDomain coordCMDAlterDomain ;

   /*
      _coordCMDAlterDomain define
    */
   class _coordCMDAlterDomain : public _coordDataCMDAlter
   {
      COORD_DECLARE_CMD_AUTO_REGISTER() ;

      public:
         _coordCMDAlterDomain () ;
         virtual ~_coordCMDAlterDomain () ;

      protected :
         OSS_INLINE virtual RTN_ALTER_OBJECT_TYPE _getObjectType () const
         {
            return RTN_ALTER_DOMAIN ;
         }

         OSS_INLINE virtual MSG_TYPE _getCatalogMessageType () const
         {
            return MSG_CAT_ALTER_DOMAIN_REQ ;
         }

         // Not a collection command
         virtual BOOLEAN _flagDoOnCollection () { return FALSE ; }

         virtual INT32 _doOnDataGroup ( MsgHeader *pMsg,
                                        pmdEDUCB *cb,
                                        rtnContextCoord::sharePtr *ppContext,
                                        coordCMDArguments *pArgs,
                                        const CoordGroupList &groupLst,
                                        const vector<BSONObj> &cataObjs,
                                        CoordGroupList &sucGroupLst ) ;
   } ;

   typedef _coordCMDAlterDomain coordCMDAlterDomain ;

}

#endif // COORD_COMMAND_DOMAIN_HPP__
