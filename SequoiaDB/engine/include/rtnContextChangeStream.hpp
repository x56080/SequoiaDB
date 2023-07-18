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
