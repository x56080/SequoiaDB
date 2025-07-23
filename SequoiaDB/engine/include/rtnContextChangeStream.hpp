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

   Source File Name = rtnContextChangeStream.hpp

   Descriptive Name = RunTime Change Stream Context Header

   When/how to use: this program may be used on binary and text-formatted
   versions of Runtime component. This file contains structure for Runtime
   Context.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          06/01/2023  HGM Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef RTN_CONTEXT_CHANGE_STREAM_HPP_
#define RTN_CONTEXT_CHANGE_STREAM_HPP_

#include "rtnContext.hpp"
#include "rtnChangeStreamNotifier.hpp"
#include "rtnChangeStreamDispatcher.hpp"
#include "rtnContextStreamBase.hpp"
#include "rtnLogFetcher.hpp"

namespace engine
{

   /*
      _rtnContextChangeStream define
    */
   // change stream context
   class _rtnContextChangeStream : public _rtnContextStreamBase
   {
      DECLARE_RTN_CTX_AUTO_REGISTER( _rtnContextChangeStream )

   public:
      _rtnContextChangeStream( INT64 contextID, UINT64 eduID ) ;
      virtual ~_rtnContextChangeStream() ;

      INT32 open( const bson::BSONObj &boOptions ) ;

      virtual const CHAR *name() const
      {
         return "CHANGESTREAM" ;
      }

      virtual RTN_CONTEXT_TYPE getType() const
      {
         return RTN_CONTEXT_CHANGE_STREAM ;
      }

   protected:
      void _close() ;

      virtual INT32 _prepareData( pmdEDUCB *cb ) ;

   protected:
      // change stream options
      utilChangeStreamOptions _options ;
      // change stream source
      rtnChangeStreamSource _source ;
   } ;

   typedef class _rtnContextChangeStream rtnContextChangeStream ;

}

#endif // RTN_CONTEXT_CHANGE_STREAM_HPP_
