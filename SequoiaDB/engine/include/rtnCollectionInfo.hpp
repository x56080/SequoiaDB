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

   Source File Name = rtnCollectionInfo.hpp

   Descriptive Name = N/A

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who         Description
   ====== =========== =========== =============================================
          23/07/2025  fangjiabin  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef RTN_COLLECTION_INFO_HPP__
#define RTN_COLLECTION_INFO_HPP__

#include "interface/IObjectInfo.h"

namespace engine {
class _rtnCollectionInfo : public SDBObject
   {
   public:
      _rtnCollectionInfo( const CONST_CL_META_INFO_PTR &meta,
                          const CONST_CL_STAT_INFO_PTR &stat )
      : _meta( meta ), _stat( stat )
      {
      }

   public:
      OSS_INLINE BOOLEAN isValid() const
      {
         return _meta != nullptr;
      }

      OSS_INLINE BOOLEAN hasStatInfo() const
      {
         return _stat != nullptr;
      }

      OSS_INLINE CONST_CL_META_INFO_PTR getMetaInfo() const
      {
         return _meta;
      }

      OSS_INLINE CONST_CL_STAT_INFO_PTR getStatInfo() const
      {
         return _stat;
      }

   private:
      CONST_CL_META_INFO_PTR _meta = nullptr;
      CONST_CL_STAT_INFO_PTR _stat = nullptr;
   };
   using rtnCollectionInfo = _rtnCollectionInfo;
}

#endif