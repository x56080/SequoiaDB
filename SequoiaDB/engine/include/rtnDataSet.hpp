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

   Source File Name = rtnDataSet.hpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of Runtime component. This file contains structure for Runtime
   Context.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          24/06/2014  YW  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef RTN_BSONSET_HPP_
#define RTN_BSONSET_HPP_

#include "rtnContextBuff.hpp"
#include "rtnQueryOptions.hpp"

using namespace bson ;

namespace engine
{
   class _pmdEDUCB ;
   class _SDB_RTNCB ;

   class _rtnDataSet : public SDBObject
   {
   public:
      _rtnDataSet( const rtnQueryOptions &options, _pmdEDUCB *cb ) ;
      _rtnDataSet( SINT64 contextID, _pmdEDUCB *cb ) ;
      _rtnDataSet( MsgOpReply *pReply, _pmdEDUCB *cb,
                   BOOLEAN ownned = FALSE ) ;

      virtual ~_rtnDataSet() ;

   protected:
      void     clear() ;

      INT32    initByQuery( const rtnQueryOptions &options,
                            _pmdEDUCB *cb ) ;

      INT32    initByReply( MsgOpReply *pReply, _pmdEDUCB *cb,
                            BOOLEAN ownned = FALSE ) ;

   public:
      INT32 next( BSONObj &obj ) ;

   private:
      rtnContextBuf     _contextBuf ;
      SINT64            _contextID ;
      _pmdEDUCB         *_cb ;
      INT32             _lastErr ;
      _SDB_RTNCB        *_rtnCB ;
      CHAR              *_pBuff ;
      BOOLEAN           _ownned ;
   } ;
   typedef class _rtnDataSet rtnDataSet ;
}

#endif // RTN_BSONSET_HPP_

