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

   Source File Name = rtnSnapshotProcessor.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/19/2022  YQC  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef RTN_SNAPSHOT_PROCESSOR_HPP_
#define RTN_SNAPSHOT_PROCESSOR_HPP_

#include "ossTypes.h"
#include "rtnFetchBase.hpp"
#include "utilPooledObject.hpp"
#include "utilUniqueID.hpp"

using namespace bson ;

namespace engine
{
   typedef ossPoolList< BSONObj > SNAP_INFO_RESULT_LIST ;

   /*
      _rtnCSSnapshotProcessor
   */
   class _rtnCSSnapshotProcessor : public _IRtnMonProcessor
   {
   public:
      _rtnCSSnapshotProcessor() ;
      virtual ~_rtnCSSnapshotProcessor() ;

      virtual INT32   pushIn( const BSONObj &obj ) ;
      virtual INT32   output( BSONObj &obj, BOOLEAN &hasOut ) ;

      virtual INT32   done( BOOLEAN &hasOut ) ;
      virtual BOOLEAN eof() const ;

   protected:
      BOOLEAN _eof ;
      BOOLEAN _hasDone ;
      BSONObj _obj ;
   } ;
   typedef _rtnCSSnapshotProcessor rtnCSSnapshotProcessor ;
   typedef utilSharePtr< _rtnCSSnapshotProcessor > rtnCSSnapshotProcessorPtr ;

}

#endif // RTN_SNAPSHOT_PROCESSOR_HPP_