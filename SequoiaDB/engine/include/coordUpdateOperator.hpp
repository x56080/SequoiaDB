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

   Source File Name = coordUpdateOperator.hpp

   Descriptive Name =

   When/how to use:

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          04/14/2017  XJH Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef COORD_UPDATE_OPERATOR_HPP__
#define COORD_UPDATE_OPERATOR_HPP__

#include "coordTransOperator.hpp"

using namespace bson ;

namespace engine
{
   /*
      _coordUpdateOperator define
   */
   class _coordUpdateOperator : public _coordTransOperator
   {
      public:
         _coordUpdateOperator() ;
         virtual ~_coordUpdateOperator() ;

         virtual INT32 execute( MsgHeader *pMsg,
                                pmdEDUCB *cb,
                                INT64 &contextID,
                                rtnContextBuf *buf ) ;

         virtual BOOLEAN      isReadOnly() const ;

         UINT32               getUpdatedNum() const ;
         UINT32               getInsertedNum() const ;
         void                 clearStat() ;

      protected:
         virtual void               _prepareForTrans( pmdEDUCB *cb,
                                                      MsgHeader *pMsg ) ;

         virtual INT32              _prepareMainCLOp( coordCataSel &cataSel,
                                                      coordSendMsgIn &inMsg,
                                                      coordSendOptions &options,
                                                      pmdEDUCB *cb,
                                                      coordProcessResult &result ) ;

         virtual void               _doneMainCLOp( coordCataSel &cataSel,
                                                   coordSendMsgIn &inMsg,
                                                   coordSendOptions &options,
                                                   pmdEDUCB *cb,
                                                   coordProcessResult &result ) ;

      private:

         BSONObj  _buildNewSelector( const BSONObj &selector,
                                     const CoordSubCLlist &subCLList ) ;

         INT32    _upsert( const CHAR *pCollectionName,
                           const BSONObj &matcher,
                           const BSONObj &updator,
                           const BSONObj &hint,
                           BOOLEAN strictDataMode,
                           pmdEDUCB *cb,
                           UINT32 &insertNum,
                           INT64 &contextID,
                           rtnContextBuf *buf ) ;

         void     _clearBlock( pmdEDUCB *cb ) ;

      private:
         UINT32               _insertedNum ;
         vector< CHAR* >      _vecBlock ;

   } ;
   typedef _coordUpdateOperator coordUpdateOperator ;

}

#endif // COORD_UPDATE_OPERATOR_HPP__


