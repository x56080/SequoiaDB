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

   Source File Name = rtnFecthBase.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          01/09/2016  Xu Jianhui  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef RTN_FETCH_BASE_HPP__
#define RTN_FETCH_BASE_HPP__

#include "core.hpp"
#include "oss.hpp"
#include "pmdEDU.hpp"
#include <map>
#include "../bson/bson.h"

using namespace bson ;

namespace engine
{

   #define DECLARE_FETCH_AUTO_REGISTER() \
   public: \
      static _rtnFetchBase *newThis () ; \
      virtual RTN_FETCH_TYPE getType() const ; \


   #define IMPLEMENT_FETCH_AUTO_REGISTER(theClass, type) \
   _rtnFetchBase *theClass::newThis () \
   { \
      return SDB_OSS_NEW theClass() ;\
   } \
   RTN_FETCH_TYPE theClass::getType() const \
   { \
      return (RTN_FETCH_TYPE)(type) ; \
   } \
   _rtnFetchAssit theClass##Assit ( theClass::newThis ) ; \


   /*
      RTN_FETCH_TYPE define
   */
   enum RTN_FETCH_TYPE
   {
      RTN_FETCH_TRANS      = 1,        /// transactions
      RTN_FETCH_CONTEXT,               /// context
      RTN_FETCH_SESSION,               /// session
      RTN_FETCH_COLLECTION,            /// collection
      RTN_FETCH_COLLECTIONSPACE,       /// collectionspace
      RTN_FETCH_DATABASE,              /// database
      RTN_FETCH_SYSTEM,                /// system
      RTN_FETCH_STORAGEUNIT,           /// storageunit
      RTN_FETCH_INDEX,                 /// index
      RTN_FETCH_DATABLOCK,             /// datablock
      RTN_FETCH_BACKUP,                /// backup

      RTN_FETCH_MAX
   } ;

   /*
      _rtnFetchBase define
   */
   class _rtnFetchBase : public SDBObject
   {
      public :
         _rtnFetchBase() {}
         virtual ~_rtnFetchBase() {}

         virtual RTN_FETCH_TYPE  getType() const = 0 ;

         virtual INT32           init( pmdEDUCB *cb,
                                       BOOLEAN isCurrent,
                                       BOOLEAN isDetail,
                                       UINT32 addInfoMask,
                                       const BSONObj obj = BSONObj() ) = 0 ;

         virtual const CHAR*     getName() const = 0 ;

      public:
         virtual BOOLEAN   isHitEnd() const = 0 ;
         virtual INT32     fetch( BSONObj &obj ) = 0 ;

   } ;
   typedef _rtnFetchBase rtnFetchBase ;

   typedef _rtnFetchBase* (*FETCH_NEW_FUNC)() ;

   /*
      _rtnFetchAssit define
   */
   class _rtnFetchAssit : public SDBObject
   {
      public:
         _rtnFetchAssit ( FETCH_NEW_FUNC pFunc ) ;
         ~_rtnFetchAssit () ;
   } ;

   /*
      _rtnFetchBuilder define
   */
   class _rtnFetchBuilder : public SDBObject
   {
      friend class _rtnFetchAssit ;

      typedef std::map< INT32, FETCH_NEW_FUNC >    MAP_FUNCS ;

      public:
         _rtnFetchBuilder () ;
         ~_rtnFetchBuilder () ;
      public:
         _rtnFetchBase  *create ( INT32 type ) ;
         void            release ( _rtnFetchBase *pFetch ) ;

      protected:
         INT32 _register( INT32 type, FETCH_NEW_FUNC pFunc ) ;

      private:
         MAP_FUNCS            _mapFuncs ;

   };

   _rtnFetchBuilder *getRtnFetchBuilder () ;

}

#endif //RTN_FETCH_BASE_HPP__

