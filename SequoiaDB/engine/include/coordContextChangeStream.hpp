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

   Source File Name = coordContextChangeStream.hpp

   Descriptive Name = RunTime Change Stream Coord Context Header

   When/how to use: this program may be used on binary and text-formatted
   versions of Runtime component. This file contains structure for Runtime
   Context.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          08/05/2023  HYQ Initial Draft

   Last Changed =

*******************************************************************************/
#include "coordResource.hpp"
#include "rtnContext.hpp"

namespace engine
{
   /*
      _rtnCoordContextChangeStream define
    */
   class _rtnCoordContextChangeStream : public _rtnContextBase,
                                        public _rtnSubContextHolder
   {
      DECLARE_RTN_CTX_AUTO_REGISTER( _rtnCoordContextChangeStream )

   public:

      _rtnCoordContextChangeStream( INT64 contextID, UINT64 eduID ) ;
      virtual ~_rtnCoordContextChangeStream() {};

      INT32 open( MsgHeader *pMsg,
                  coordResource *resource,
                  pmdEDUCB *cb ) ;

      virtual const CHAR *name() const
      {
         return "COORDCHANGESTREAM" ;
      }

      virtual RTN_CONTEXT_TYPE getType() const
      {
         return RTN_CONTEXT_COORD_CHANGE_STREAM ;
      }

      virtual _dmsStorageUnit *getSU()
      {
         return NULL ;
      }

   protected:
      virtual INT32 _prepareData( pmdEDUCB *cb ) ;
      INT32 _parseTokenAndControlRC( const BSONObj& obj ) ;
      INT32 _parseArguments( MsgHeader *pMsg, coordResource *resource ) ;
      INT32 _retryWatch( pmdEDUCB *cb ) ;
      INT32 _executeWatch( MsgHeader *pMsg, pmdEDUCB *cb) ;

   protected:
      BSONObj _options ;
      coordResource *_pResource = NULL ;
      CHAR _tokenStr[ MSG_STREAM_TOKEN_STING_SIZE + 1 ] = { 0 } ;
      INT32 _controlRC = SDB_OK ;
      CoordGroupList _groupList ;
   } ;

   typedef class _rtnCoordContextChangeStream rtnCoordContextChangeStream ;

}