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

   Source File Name = bigRecordStream.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          12/31/2021  LYC  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef VESSEL_BIG_RECORD_STREAM_H_
#define VESSEL_BIG_RECORD_STREAM_H_

#include "vessel/slice.h"
#include "vessel/recordID.h"
#include "utilCompression.hpp"

namespace engine
{
namespace vessel
{
   class bigRecordStream : public SDBObject
   {
      public:
         bigRecordStream(){}
         bigRecordStream(const slice &record);
         virtual ~bigRecordStream(){}
         bigRecordStream(const bigRecordStream &s);
         bigRecordStream &operator=(const bigRecordStream &s);
      
      public:
         void reset();

         BOOLEAN isValid()const;

         BOOLEAN isEndOfStream()const {return 0 == _remainingSize;}

         UINT32 getRemainingSize()const {return _remainingSize;}

         UINT32 getRecordSize()const;

      public:
         // Note: This method will affect the validity of the last slice addr.
         const CHAR *reserveSlice(UINT32 sliceSize);

         void fillLastSlice(const recordID &rid); 

         recordID getLastSliceAddr()const;
         
         UINT32 getSliceCount()const {return _sliceAddrs.size();}

         recordID getSliceAddr(UINT32 pos)const;

      private:
         slice _recordData;
         UINT32 _remainingSize = 0;
         ossPoolVector<recordID> _sliceAddrs;
   };
}
}
#endif