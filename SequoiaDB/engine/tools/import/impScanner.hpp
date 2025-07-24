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

   Source File Name = impParser.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          06/02/2020  HJW Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef IMP_SCANNER_HPP_
#define IMP_SCANNER_HPP_

#include "core.hpp"
#include "oss.hpp"
#include "impCommon.hpp"
#include "impOptions.hpp"
#include "impWorker.hpp"
#include "impRecordParser.hpp"

namespace import
{
   class Scanner : public WorkerArgs
   {
   public:
      Scanner() ;
      ~Scanner() ;
      INT32 initParser( Options* options ) ;
      INT32 init( Options* options, DataQueue* dataQueue, INT32 workerNum ) ;
      INT32 start() ;
      INT32 stop() ;
      inline BOOLEAN isStopped() const { return _stopped ; }

   private:
      INT32             _workerNum ;
      BOOLEAN           _inited ;
      BOOLEAN           _stopped ;

      Options*          _options ;
      DataQueue*        _dataQueue ;
      Worker*           _worker ;
      RecordParser*     _parser ;

      impBufferBlock    _bufferBlock1 ;
      impBufferBlock    _bufferBlock2 ;

      vector<CHAR*>     _fields ;

      friend BOOLEAN _waitBufferBlock( Scanner* self, BOOLEAN useFirstBlock,
                                       impBufferBlock*& block ) ;
      friend void _scannerRoutine( WorkerArgs* args ) ;
   };
}

#endif /* IMP_SCANNER_HPP_ */
