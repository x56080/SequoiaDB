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

   Source File Name = rtnStreamSource.hpp

   Descriptive Name = Stream Source

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          06/01/2023  HGM Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef RTN_STREAM_SOURCE_HPP__
#define RTN_STREAM_SOURCE_HPP__

#include "monStreamMonitorManager.hpp"
#include "rtnContext.hpp"
#include "utilPooledObject.hpp"
#include "../bson/bson.hpp"
#include "utilStreamToken.hpp"

namespace engine
{

   /*
      _rtnStreamSourceProcessor define
    */
   // process of stream source
   class _rtnStreamSourceProcessorBase
   {
   public:
      _rtnStreamSourceProcessorBase() = default ;
      virtual ~_rtnStreamSourceProcessorBase() = default ;

      // process control record
      virtual INT32 processControlRecord( const bson::BSONObj &result ) = 0 ;
      // process change record
      virtual INT32 processChangeRecord( const bson::BSONObj &result ) = 0 ;
      // process data record
      virtual INT32 processDataRecord( const bson::BSONObj &result ) = 0 ;
      // get stream monitor
      virtual const monStreamMonitor &getMonitor() const = 0 ;
      // get current size of batch
      virtual UINT32 getCurrentBatchSize() const = 0 ;
   } ;

   typedef class _rtnStreamSourceProcessorBase rtnStreamSourceProcessorBase ;
   typedef class _rtnStreamSourceProcessorBase rtnStreamSourceProcessor ;

   /*
      _rtnStreamSourceBase define
    */
   // base class of stream source
   class _rtnStreamSourceBase : public _utilPooledObject,
                                public _monStreamSourceMonitor
   {
   public:
      _rtnStreamSourceBase( rtnStreamSourceProcessor &processor )
      : _processor( processor )
      {
      }

      virtual ~_rtnStreamSourceBase() = default ;

      // get records
      virtual INT32 getRecords( INT64 timeout ) = 0 ;

      // check if processor is full
      BOOLEAN isProcessorFull() const
      {
         return _processor.getCurrentBatchSize() >= _maxBatchSize ;
      }

   protected:
      // max batch size ( 128MB ) to return
      static const UINT32 _maxBatchSize = 128 * 1024 * 1024 ;
      // stream record processor
      rtnStreamSourceProcessor &_processor ;
   } ;

   typedef class _rtnStreamSourceBase rtnStreamSourceBase ;
   typedef class _rtnStreamSourceBase rtnStreamSource ;

}

#endif // RTN_STREAM_SOURCE_HPP__
