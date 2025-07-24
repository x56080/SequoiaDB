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

   Source File Name = impRoutine.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          7/7/2015  David Li  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef IMP_ROUTINE_HPP_
#define IMP_ROUTINE_HPP_

#include "core.hpp"
#include "oss.hpp"
#include "impCommon.hpp"
#include "impOptions.hpp"
#include "impScanner.hpp"
#include "impParser.hpp"
#include "impImporter.hpp"

using namespace std;

namespace import
{
   class Routine
   {
   public:
      Routine( Options& options ) ;

      ~Routine() ;

      INT32 run() ;

      void printStatistics() ;

   private:
      INT32 _init() ;

      INT32 _startScanner() ;
      INT32 _stopScanner() ;

      INT32 _startParser() ;
      INT32 _waitParserStop() ;

      INT32 _startImporter() ;
      INT32 _stopImporter() ;

   private:
      BOOLEAN           _isInit ;
      INT32             _dataQueueNum ;

      DataQueue*        _dataQueue ;
      BsonPageQueue*    _freeQueue ;
      PageQueue*        _importQueue ;

      Options&          _options ;

      PageQueueBuffer   _importQueueBuffer ;

      Scanner           _scanner ;
      Parser            _parser ;
      Importer          _importer ;
      Packer            _packer ;
   };
}

#endif /* IMP_ROUTINE_HPP_ */
