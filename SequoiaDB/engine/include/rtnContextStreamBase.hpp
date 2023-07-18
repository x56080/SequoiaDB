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

   Source File Name = rtnContextStreamBase.hpp

   Descriptive Name = RunTime Stream Base Context Header

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

#ifndef RTN_CONTEXT_STREAM_BASE_HPP_
#define RTN_CONTEXT_STREAM_BASE_HPP_

#include "rtnChangeStreamSource.hpp"
#include "rtnContext.hpp"
#include "rtnChangeStreamNotifier.hpp"
#include "rtnChangeStreamDispatcher.hpp"
#include "rtnLogFetcher.hpp"
#include "utilStream.hpp"
#include "utilStreamToken.hpp"

namespace engine
{

   /*
      _rtnContextStreamBase define
    */
   // base class of stream context
   class _rtnContextStreamBase : public _rtnContextBase,
                                 public _rtnStreamSourceProcessorBase
   {
   public:
      _rtnContextStreamBase( utilStreamType streamType,
                             INT64 contextID,
                             UINT64 eduID,
                             monStreamSourceMonitor &sourceMonitor ) ;
      virtual ~_rtnContextStreamBase() = default ;

      virtual _dmsStorageUnit *getSU()
      {
         return NULL ;
      }

      // override functions of _rtnStreamSourceProcessorBase
      virtual INT32 processControlRecord( const bson::BSONObj &result ) ;
      virtual INT32 processChangeRecord( const bson::BSONObj &result ) ;
      virtual INT32 processDataRecord( const bson::BSONObj &result ) ;

      virtual const monStreamMonitor &getMonitor() const
      {
         return _monitor ;
      }

      virtual UINT32 getCurrentBatchSize() const
      {
         return (UINT32)( buffSize() ) ;
      }

   protected:
      // stream monitor
      monStreamMonitor _monitor ;
   } ;

   typedef class _rtnContextStreamBase rtnContextStreamBase ;

}

#endif // RTN_CONTEXT_STREAM_BASE_HPP_
