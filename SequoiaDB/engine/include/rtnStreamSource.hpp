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
