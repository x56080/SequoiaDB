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

   Source File Name = dpsRequest.hpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef DPS_REQUEST_HPP_
#define DPS_REQUEST_HPP_

#include "dpsLogDef.hpp"
#include "dms.hpp"

namespace engine
{
   class dpsPackedRequest : public SDBObject
   {
      public:
         dpsPackedRequest() = default;
         explicit dpsPackedRequest(UINT16 type,
                                   UINT16 flags,
                                   UINT32 packedEleSize,
                                   const CHAR *eleBuffer):
         _type(type),
         _flags(flags),
         _packedEleSize(packedEleSize),
         _elementBuffer(eleBuffer){}

         ~dpsPackedRequest() = default;
         dpsPackedRequest(const dpsPackedRequest &) = default;
         dpsPackedRequest &operator=(const dpsPackedRequest &) = default;

      public:
         OSS_INLINE UINT16 getType()const {return _type;}
         OSS_INLINE UINT16 getFlags()const {return _flags;}
         OSS_INLINE UINT32 getPackedElementsSize()const {return _packedEleSize;}
         OSS_INLINE const CHAR *getElementBuffer()const {return _elementBuffer;}

      private:
         UINT16 _type = LOG_TYPE_DUMMY;
         UINT16 _flags = 0;
         UINT32 _packedEleSize = 0;
         const CHAR *_elementBuffer = nullptr;         
   };//class dpsPackedRequest

   struct dpsWriteOptions : public SDBObject
   {
      UINT32 csid = DMS_INVALID_LOGICCSID;
      UINT32 clid = DMS_INVALID_LOGICCLID;
      INT32 extentPos = -1;
      UINT64 transTime = DPS_INVALID_TRANS_TIME;
      BOOLEAN notify = FALSE;
      BOOLEAN transEnabled = FALSE;
      BOOLEAN irrversible = FALSE;
      BOOLEAN flushAtOnce = FALSE;
   };//struct dpsWriteOptions

   struct dpsSearchOptions : public SDBObject
   {
      BOOLEAN searchMem = TRUE;
      BOOLEAN searchFile = TRUE;
      BOOLEAN onlyHeader = FALSE;
      INT32 limits = 1;
      INT32 maxTime = -1;
      INT32 maxSize = 5242880;
   };//struct dpsSearchOptions

} // namespace engine


#endif//DPS_REQUEST_HPP_
