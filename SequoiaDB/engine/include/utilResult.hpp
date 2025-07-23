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

   Source File Name = utilResult.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          02/18/2019   LYB Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef UTIL_RESULT_HPP_
#define UTIL_RESULT_HPP_

#include "oss.hpp"
#include "../bson/bson.hpp"
#include "ossMemPool.hpp"
#include "dms.hpp"

using namespace bson ;

namespace engine
{

   #define UTIL_RESULT_MASK_IDX              ( 0x00000001 )
   #define UTIL_RESULT_MASK_ID               ( 0x00000002 )
   #define UTIL_RESULT_MASK_ALL              ( 0xFFFFFFFF )
   #define UTIL_RESULT_MASK_DFT              ( UTIL_RESULT_MASK_IDX | \
                                               UTIL_RESULT_MASK_ID )

   /*
      utilResult define
   */
   class utilResult : public SDBObject
   {
   public:
      utilResult( UINT32 mask = UTIL_RESULT_MASK_DFT ) ;
      virtual ~utilResult() ;

      void              enableMask( UINT32 mask ) ;
      void              disableMask( UINT32 mask ) ;
      BOOLEAN           isMaskEnabled( UINT32 mask ) const ;

      void              setResultObj( const BSONObj &obj ) ;
      void              resetResultObj() ;
      BSONObj           getResultObj() const ;
      BOOLEAN           isResultObjEmpty() const ;

      void              reset() ;
      void              resetStat() ;
      void              resetInfo( BOOLEAN includeResult = TRUE ) ;

      BSONObj           toBSON() const ;
      void              toBSON( BSONObjBuilder &builder ) const ;

   protected:
      virtual void      _resetStat() = 0 ;
      virtual void      _resetInfo() = 0 ;
      virtual void      _toBSON( BSONObjBuilder &builder ) const = 0 ;
      virtual BOOLEAN   _filterResultElement( const BSONElement &e ) const = 0 ;

   private:
      UINT32            _resultMask ;
      BSONObj           _resultObj ;

   } ;

   /*
      utilIdxDupErrAssit define
   */
   class utilIdxDupErrAssit : public SDBObject
   {
   public:
      utilIdxDupErrAssit( const BSONObj &idxKeyPattern,
                          const BSONObj &idxValue,
                          const CHAR *idxName = NULL ) ;
      
      ~utilIdxDupErrAssit() ;

      INT32    getIdxMatcher( BSONObj &idxMatcher,
                              BOOLEAN cvtUndefined = TRUE ) ;

      INT32    getIdxHint( BSONObj &hint) const;

   private:
      BSONObj        _idxKeyPattern ;
      BSONObj        _idxValue ;
      const CHAR *   _idxName ;

   } ;

   /*
      utilWriteResult define
   */
   class utilWriteResult : public utilResult
   {
   public:
      utilWriteResult( UINT32 mask = UTIL_RESULT_MASK_DFT ) ;
      virtual ~utilWriteResult() ;

   public:
      void     resetIndexErrInfo() ;
      BOOLEAN  isIndexErrInfoEmpty() const ;

      INT32    setIndexErrInfo( const CHAR *idxName,
                                const BSONObj& idxKeyPattern,
                                const BSONObj& idxValue,
                                const BSONObj& curObj= BSONObj() ) ;

      INT32    setCurrentID( const BSONObj &obj ) ;
      INT32    setPeerID( const BSONObj &obj ) ;
      void     setCurRID( const dmsRecordID &rid ) ;
      void     setPeerRID( const dmsRecordID &rid ) ;

      void     setErrInfo( const utilWriteResult *pResult ) ;

      const ossPoolString& getIdxName() const ;
      BSONObj              getIdxKeyPattern() const ;
      BSONObj              getIdxValue() const ;
      BSONObj              getCurID() const ;
      BSONObj              getPeerID() const ;
      const dmsRecordID&   getCurRID() const ;
      const dmsRecordID&   getPeerRID() const ;

      BOOLEAN              isSameID() const ;

   protected:
      virtual void      _resetStat() ;
      virtual void      _resetInfo() ;
      virtual void      _toBSON( BSONObjBuilder &builder ) const ;
      virtual BOOLEAN   _filterResultElement( const BSONElement &e ) const ;

   protected:
      ossPoolString        _idxName ;
      BSONObj              _idxKeyPattern ;
      BSONObj              _idxValue ;
      BSONObj              _curID ;
      BSONObj              _peerID ;
      dmsRecordID          _curRID ;
      dmsRecordID          _peerRID ;

   } ;
}

#endif /* UTIL_RESULT_HPP_ */



