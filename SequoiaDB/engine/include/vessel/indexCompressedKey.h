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

   Source File Name = indexCompressedKey.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_INDEX_COMPRESSED_KEY_H_
#define VESSEL_INDEX_COMPRESSED_KEY_H_

#include "ixmKey.hpp"

namespace engine
{
namespace vessel
{
   class indexCompressedKey : public SDBObject
   {
      public:
         indexCompressedKey(){}
         explicit indexCompressedKey(const CHAR *data):
         _data(data){}
         ~indexCompressedKey(){}
         indexCompressedKey(const indexCompressedKey &o):
         _data(o._data){}
         indexCompressedKey &operator=(const indexCompressedKey &o)
         {
            _data = o._data;
            return *this;
         }

      public:
         OSS_INLINE BOOLEAN isValid()const
         {
            return NULL != _data;
         }
         UINT32 getKeyDataSize()const;
      private:
         /// flag size (1 byte) included
         /// res < 0 when failed to parse.
         INT32 getThisKeySize(const CHAR *flags)const;

      private:
         const CHAR *_data = NULL;
   };//class indexCompressedKey
} // namespace vessel

} // namespace engine


#endif//VESSEL_INDEX_COMPRESSED_KEY_H_