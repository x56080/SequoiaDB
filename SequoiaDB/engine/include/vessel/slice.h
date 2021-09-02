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

   Source File Name = slice.h

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains functions for agent processing.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_SLICE_H_
#define VESSEL_SLICE_H_

#include "oss.hpp"
#include "ossTypes.hpp"

namespace engine
{
namespace vessel
{
   class slice : public SDBObject
   {
      public:
         OSS_INLINE slice():_len(0), _data(NULL)
         {
            unvalidIfNecessary();
         }
         OSS_INLINE slice(UINT32 len, const CHAR *data):
                    _len(len), _data(data)
         {
            unvalidIfNecessary();
         }
         OSS_INLINE slice(UINT32 len, const void *data):
                    _len(len), _data((const CHAR *)data)
         {
            unvalidIfNecessary();
         }
         OSS_INLINE slice(const slice &r):
                    _len(r._len), _data(r._data){}

         OSS_INLINE ~slice(){ _len = 0; _data = NULL;}

      public:
         OSS_INLINE slice &operator=(const slice &r)
         {
            _len = r._len;
            _data = r._data;
            return *this;
         }

         OSS_INLINE UINT32 len()const
         {
            return _len;
         }

         OSS_INLINE const CHAR *data()const
         {
            return _data;
         }

         OSS_INLINE void reset()
         {
            _len = 0;
            _data = NULL;
            return;
         }

         OSS_INLINE void reset(UINT32 len, const CHAR *data)
         {
            _len = len;
            _data = data;
            unvalidIfNecessary();
            return;
         }

         OSS_INLINE BOOLEAN isValid()const
         {
            return 0 < _len && NULL != _data;
         }
      
      private:
         OSS_INLINE void unvalidIfNecessary()
         {
            if (!isValid())
            {
               if (0 != _len)
               {
                  _len = 0;
               }
               if (NULL != _data)
               {
                  _data = NULL;
               }
               return;
            }
         }
         
      private:
         UINT32 _len = 0;
         const CHAR *_data = NULL;
   };

} // namespace vessel
} // namespace engine

#endif // VESSEL_SLICE_H_
