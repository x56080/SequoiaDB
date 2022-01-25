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

   Source File Name = bigRecordStream.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          12/31/2021  LYC  Initial Draft

   Last Changed =

******************************************************************************/
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