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

   Source File Name = coordCommandSchema.hpp

   Descriptive Name = N/A

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who         Description
   ====== =========== =========== =============================================
          23/07/2025  fangjiabin  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef COORD_CMD_SCHEMA_HPP__
#define COORD_CMD_SCHEMA_HPP__

#include "coordCommandData.hpp"
#include "coordFactory.hpp"

namespace engine
{

   /*
      _coordCMDCreateSchema define
    */
   class _coordCMDCreateSchema : public _coordCommandBase
   {
      COORD_DECLARE_CMD_AUTO_REGISTER() ;
   public:
      _coordCMDCreateSchema() ;
      virtual ~_coordCMDCreateSchema() ;

      virtual INT32 execute( MsgHeader *pMsg,
                             pmdEDUCB *cb,
                             INT64 &contextID,
                             rtnContextBuf *buf ) ;
   } ;
   typedef class _coordCMDCreateSchema coordCMDCreateSchema ;

   /*
      _coordCMDDropSchema define
    */
   class _coordCMDDropSchema : public _coordCommandBase
   {
      COORD_DECLARE_CMD_AUTO_REGISTER()
   public:
      _coordCMDDropSchema() ;
      virtual ~_coordCMDDropSchema() ;

      virtual INT32 execute( MsgHeader *pMsg,
                             pmdEDUCB *cb,
                             INT64 &contextID,
                             rtnContextBuf *buf ) ;
   } ;
   typedef class _coordCMDDropSchema coordCMDDropSchema ;

   /*
      _coordCMDAlterSchema define
    */
   class _coordCMDAlterSchema : public _coordDataCMD3Phase
   {
      COORD_DECLARE_CMD_AUTO_REGISTER()
   public:
      _coordCMDAlterSchema() ;
      virtual ~_coordCMDAlterSchema() ;

   protected:
      virtual INT32 _parseMsg( MsgHeader *pMsg,
                               coordCMDArguments *pArgs )
      {
         return SDB_OK ;
      }

      virtual INT32 _generateCataMsg( MsgHeader *pMsg,
                                      pmdEDUCB *cb,
                                      coordCMDArguments *pArgs,
                                      CHAR **ppMsgBuf,
                                      INT32 *pBufSize )
      {
         *ppMsgBuf = (CHAR*)pMsg ;
         *pBufSize = pMsg->messageLength ;
         return SDB_OK ;
      }

      virtual INT32 _generateDataMsg( MsgHeader *pMsg,
                                      pmdEDUCB *cb,
                                      coordCMDArguments *pArgs,
                                      const vector<BSONObj> &cataObjs,
                                      CHAR **ppMsgBuf,
                                      INT32 *pBufSize ) ;

      /*
         command on collection
      */
      virtual BOOLEAN _flagDoOnCollection () { return _hasCollection ; }

   protected:
      BOOLEAN _hasCollection ;
   } ;
   typedef class _coordCMDAlterSchema coordCMDAlterSchema ;

}

#endif /* COORD_CMD_SCHEMA_HPP__ */
