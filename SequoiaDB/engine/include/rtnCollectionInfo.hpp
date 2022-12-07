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