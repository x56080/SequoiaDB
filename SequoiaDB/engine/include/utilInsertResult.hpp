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

   Source File Name = utilInsertResult.hpp

   Dependencies: N/A

   Restrictions: N/AdmsStorageDataCommon

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          02/13/2019   LYB Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef UTIL_INSERT_RESULT_HPP_
#define UTIL_INSERT_RESULT_HPP_

#include "oss.hpp"
#include "utilResult.hpp"

using namespace bson ;

namespace engine
{

   /*
      utilInsertResult define
   */
   class utilInsertResult : public utilWriteResult
   {
   public:
      utilInsertResult() ;
      virtual ~utilInsertResult() ;

   protected:
      virtual void      _resetStat() ;
      virtual void      _resetInfo() ;
      virtual void      _toBSON( BSONObjBuilder &builder ) const ;
      virtual BOOLEAN   _filterResultElement( const BSONElement &e ) const ;

   public:
      void     enableIndexErrInfo() ;
      void     disableIndexErrInfo() ;
      BOOLEAN  isEnaleIndexErrInfo() const ;

      void     enableReturnIDInfo() ;
      void     disableReturnIDInfo() ;
      BOOLEAN  isEnableReturnIDInfo() const ;

      UINT64               insertedNum() const { return _insertedNum ; }
      UINT64               duplicatedNum() const { return _duplicatedNum ; }
      UINT64               modifiedNum() const { return _modifiedNum ; }

      void                 incInsertedNum( UINT64 step = 1 )
      {
         _insertedNum += step ;
      }

      void                 incDuplicatedNum( UINT64 step = 1 )
      {
         _duplicatedNum += step ;
      }

      void                 incModifiedNum( UINT64 step = 1 )
      {
         _modifiedNum += step ;
      }

      void                 setReturnIDByObj( const BSONObj &obj ) ;
      BSONObj              getReturnIDObj() const ;

   private:
      UINT64               _insertedNum ;
      UINT64               _duplicatedNum ;
      BOOLEAN              _enableReturnID ;
      BSONObj              _returnIDObj ;

   protected:
      UINT64               _modifiedNum ;    // replace or update on duplication
   } ;

   /*
      utilUpdateResult define
   */
   class utilUpdateResult : public utilInsertResult
   {
   public:
      utilUpdateResult() ;
      virtual ~utilUpdateResult() ;

   protected:
      virtual void      _resetStat() ;
      virtual void      _resetInfo() ;
      virtual void      _toBSON( BSONObjBuilder &builder ) const ;
      virtual BOOLEAN   _filterResultElement( const BSONElement &e ) const ;

   public:
      UINT64               updateNum() const { return _updatedNum ; }

      void                 incUpdatedNum( UINT64 step = 1 )
      {
         _updatedNum += step ;
      }

      void                 setCurrentField( BSONElement &errEle ) ;

   private:
      UINT64               _updatedNum ;
      BSONObj              _currentFieldObj ;

   } ;

   /*
      utilDeleteResult define
   */
   class utilDeleteResult : public utilWriteResult
   {
   public:
      utilDeleteResult() ;
      virtual ~utilDeleteResult() ;

      UINT64            deletedNum() const { return _deletedNum ; }

      void              incDeletedNum( UINT64 step = 1 )
      {
         _deletedNum += step ;
      }

   protected:
      virtual void      _resetStat() ;
      virtual void      _resetInfo() ;
      virtual void      _toBSON( BSONObjBuilder &builder ) const ;
      virtual BOOLEAN   _filterResultElement( const BSONElement &e ) const ;

   private:
      UINT64               _deletedNum ;

   } ;

}

#endif /* UTIL_INSERT_RESULT_HPP_ */

