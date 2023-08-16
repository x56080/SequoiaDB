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