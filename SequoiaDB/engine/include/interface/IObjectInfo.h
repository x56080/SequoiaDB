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

   Source File Name = IObjectInfo.hpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          11/25/2022  ZHY  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef SDB_I_OBJECT_INFO_H_
#define SDB_I_OBJECT_INFO_H_

#include "rtnStatMCVSet.hpp"
#include "utilStringView.hpp"
#include "utilUniqueID.hpp"
namespace engine
{
   class IIndexMetaInfo : public SDBObject
   {
   public:
      IIndexMetaInfo() = default;

      virtual ~IIndexMetaInfo() = default;

   public:
      virtual const CHAR *getIndexName() const = 0;

      virtual const CHAR *getCLFullName() const = 0;

      virtual utilIdxInnerID getIdxInnerID() const = 0;

      virtual utilCLUniqueID getCLUniqueID() const = 0;

      virtual const OID &getOID() const = 0;

      virtual const BSONObj &getKeyPattern() const = 0;

      virtual UINT32 getNumKeys() const = 0;

      virtual UINT64 getStpEffectiveTime() const = 0;

      virtual INT32 getExtentID() const = 0;

      virtual INT64 getLogicalID() const = 0;

      virtual UINT16 getIndexType() const = 0;

      virtual INT32 getIndexStatus() const = 0;

      virtual BOOLEAN isNormal() const = 0;

      virtual BOOLEAN isUnique() const = 0;

      virtual BOOLEAN isEnforced() const = 0;

      virtual BOOLEAN isNotNull() const = 0;

      virtual BOOLEAN isNotArray() const = 0;

      virtual BOOLEAN isDropDups() const = 0;

      virtual BOOLEAN isIDIndex() const = 0;
   };
   using INDEX_META_INFO_PTR = std::shared_ptr< IIndexMetaInfo >;
   using CONST_INDEX_META_INFO_PTR = std::shared_ptr< const IIndexMetaInfo >;

   class ICollectionMetaInfo : public SDBObject
   {
   public:
      ICollectionMetaInfo() = default;

      virtual ~ICollectionMetaInfo() = default;

   public:
      virtual const CHAR *getCLFullName() const = 0;

      virtual utilCLUniqueID getCLUniqueID() const = 0;

      virtual UINT32 getAttributes() const = 0;

      virtual UINT32 getPageSizeLog2() const = 0;

      virtual UINT32 getTotalDataPages() const = 0;

      virtual UINT64 getGlobTransAvailTime() const = 0;

      virtual UINT32 getIndexNum() const = 0;

      virtual std::shared_ptr< const IIndexMetaInfo > at( UINT32 position ) const = 0;

      virtual std::shared_ptr< const IIndexMetaInfo > seek( const CHAR *indexName ) const = 0;

      virtual std::shared_ptr< const IIndexMetaInfo > seek( const OID &indexOID ) const = 0;
   };
   using CL_META_INFO_PTR = std::shared_ptr< ICollectionMetaInfo >;
   using CONST_CL_META_INFO_PTR = std::shared_ptr< const ICollectionMetaInfo >;

   class IIndexStatInfo : public SDBObject
   {
   public:
      IIndexStatInfo() = default;

      virtual ~IIndexStatInfo() = default;

   public:
      virtual const CHAR *getIndexName() const = 0;

      virtual const CHAR *getCLFullName() const = 0;

      virtual UINT64 getCreateTime() const = 0;

      virtual UINT64 getSampleRecords() const = 0;

      virtual UINT64 getTotalRecords() const = 0;

      virtual UINT32 getIndexPages() const = 0;

      virtual UINT32 getIndexLevels() const = 0;

      virtual BOOLEAN isUnique() const = 0;

      virtual const BSONObj &getKeyPattern() const = 0;

      virtual UINT32 getNumKeys() const = 0;

      virtual UINT64 getDistinctValues() const = 0;

      virtual FLOAT64 getNullFrac() const = 0;

      virtual FLOAT64 getUndefFrac() const = 0;

      virtual BOOLEAN isValidForEstimate() const = 0;

      virtual INT32 evalRangeOperator( rtnStatKey &startKey,
                                       rtnStatKey &stopKey,
                                       FLOAT64 &predSelectivity,
                                       FLOAT64 &scanSelectivity ) const = 0;

      virtual INT32 evalETOperator( rtnStatKey &key,
                                    FLOAT64 &predSelectivity,
                                    FLOAT64 &scanSelectivity ) const = 0;

      virtual INT32 evalGTOperator( rtnStatKey &startKey,
                                    FLOAT64 &predSelectivity,
                                    FLOAT64 &scanSelectivity ) const = 0;

      virtual INT32 evalLTOperator( rtnStatKey &stopKey,
                                    FLOAT64 &predSelectivity,
                                    FLOAT64 &scanSelectivity ) const = 0;
   };
   using INDEX_STAT_INFO_PTR = std::shared_ptr< IIndexStatInfo >;
   using CONST_INDEX_STAT_INFO_PTR = std::shared_ptr< const IIndexStatInfo >;

   class ICollectionStatInfo : public SDBObject
   {
   public:
      ICollectionStatInfo() = default;

      virtual ~ICollectionStatInfo() = default;

   public:
      virtual const CHAR *getCLFullName() const = 0;

      virtual UINT64 getCreateTime() const = 0;

      virtual UINT64 getSampleRecords() const = 0;

      virtual UINT64 getTotalRecords() const = 0;

      virtual UINT32 getTotalDataPages() const = 0;

      virtual UINT64 getTotalDataSize() const = 0;

      virtual UINT32 getAvgNumFields() const = 0;

      virtual UINT32 getIndexNum() const = 0;

      virtual std::shared_ptr< const IIndexStatInfo > at( UINT32 position ) const = 0;

      virtual std::shared_ptr< const IIndexStatInfo > seek( const CHAR *indexName ) const = 0;
   };
   using CL_STAT_INFO_PTR = std::shared_ptr< ICollectionStatInfo >;
   using CONST_CL_STAT_INFO_PTR = std::shared_ptr< const ICollectionStatInfo >;

} // namespace engine

#endif