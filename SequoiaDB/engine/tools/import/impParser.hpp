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
          7/7/2015  David Li  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef IMP_PARSER_HPP_
#define IMP_PARSER_HPP_

#include "core.hpp"
#include "oss.hpp"
#include "impCommon.hpp"
#include "impOptions.hpp"
#include "impWorker.hpp"
#include "impLogFile.hpp"
#include "impPacker.hpp"

namespace import
{
   class Parser: public WorkerArgs
   {
   public:
      Parser() ;

      ~Parser() ;

      INT32 init( Options* options, DataQueue* dataQueue, Packer* packer,
                  INT32 workerNum ) ;

      INT32 start() ;

      INT32 stop() ;

      inline BOOLEAN isStopped()
      {
         return 0 == _livingNum.fetch() ;
      }

      inline INT64 parsedNum()
      {
         return _parsedNum.fetch() ;
      }

      inline INT64 failedNum()
      {
         return _failedNum.fetch() ;
      }

      inline const string& logFileName() const
      {
         return _logFile.fileName() ;
      }

   private:
      BOOLEAN           _inited ;
      BOOLEAN           _stopped ;

      Options*          _options ;
      DataQueue*        _dataQueue ;
      Packer*           _packer ;

      LogFile           _logFile ;
      vector<Worker*>   _workers ;

      // statistics
      ossAtomicSigned32 _livingNum ;
      ossAtomicSigned64 _parsedNum ;
      ossAtomicSigned64 _failedNum ;

      friend void _parserRoutine( WorkerArgs* args ) ;
   };
}

#endif /* IMP_PARSER_HPP_ */
