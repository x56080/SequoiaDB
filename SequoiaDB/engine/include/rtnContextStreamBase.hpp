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
