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

   Source File Name = ixmKey.hpp

   Descriptive Name = Index Management Key Header

   When/how to use: this program may be used on binary and text-formatted
   versions of index management component. This file contains structure for
   index keys. One index key may refer existing information in file, or contains
   its own buffer. So we have two classes for each purpose.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/14/2012  TW  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef IXMKEY_HPP_
#define IXMKEY_HPP_

#include "core.hpp"
#include "oss.hpp"
#include "../bson/bson.h"
#include "pd.hpp"
#include <string>
#include <vector>
using namespace bson ;
using namespace std ;
namespace engine
{
   // _keyData contains raw data, which may or may not be regular BSON
   // there is 1 byte indicator to show whether if it's BSON object
   // _ixmKey(BSONObj) is converting from regular BSON to key
   // toBson() is converting from key to BSON

   // _ixmKey class doesn't own the buffer, that means the _keyData is pointing
   // to some place that allocated by other threads/functions
   // If user want to use owned-buffered key, they should use _ixmKeyOwned
   class _ixmKey : public SDBObject
   {
      // disable = operator
      void operator=(const _ixmKey&) ;
   protected:
      enum { IsBSON = 0xff } ;
      // starting of raw data
      const UINT8 *_keyData ;
      // this function should only be used internally, when we know it's not
      // compact format
      BSONObj _bson() const
      {
         SDB_ASSERT ( !isCompactFormat(),
                      "Key shouldn't be compacted when calling _bson" ) ;
         // first byte is reserved for isCompact
         return BSONObj((const CHAR *)_keyData + 1) ;
      }
   private:
      INT32 _compareHybrid ( const _ixmKey &r, const Ordering &order) const ;
   public:

      // since _ixmKey itself never convert a real BSON into buffer, so it's
      // always refering from existing buffer or keys
      _ixmKey() { _keyData = NULL ; }
      _ixmKey ( const _ixmKey &r )
      {
         this->_keyData = r._keyData ;
      }
      _ixmKey ( const CHAR *keyData )
      {
         this->_keyData = (UINT8*)keyData ;
      }
      void assign(const _ixmKey &r )
      {
         this->_keyData = r._keyData ;
      }

      // well ordered compare
      INT32 woCompare ( const _ixmKey &right, const Ordering &o ) const ;
      // well ordered equao
      BOOLEAN woEqual ( const _ixmKey &right ) const ;
      // to BSON object
      void _toBson(BSONObjBuilder &b, BSONObjIterator *keyIter = NULL ) const ;
      BSONObj toBson( BufBuilder *bb = NULL ) const ;
      INT32 toRecord( const BSONObj keyPattern,
                      BSONObjBuilder &resultBuilder ) const ;
      // convert to std::string
      std::string toString(BOOLEAN isArray = FALSE, BOOLEAN full=FALSE) const
      {
         return toBson().toString(isArray, full);
      }

      BOOLEAN hasNullOrUndefined() const ;

      BOOLEAN isUndefined() const ;

      // get raw data
      OSS_INLINE const CHAR *data() const
      {
         return (const CHAR *) _keyData ;
      }

      // get the size of key
      INT32 dataSize() const ;
      // check if a key is valid
      OSS_INLINE BOOLEAN isValid() const
      {
         return (_keyData != NULL) ;
      }
      // whether the data is compact format
      OSS_INLINE BOOLEAN isCompactFormat() const
      {
         return *_keyData != IsBSON ;
      }
   } ;
   typedef class _ixmKey ixmKey ;

   // owned index key means the keyitself allocated buffer
   class _ixmKeyOwned : public _ixmKey
   {
      void operator=(const _ixmKeyOwned&);
   public:
      // convert from BSON to key
      _ixmKeyOwned ( const BSONObj &obj ) ;
      // make a copy of key
      _ixmKeyOwned ( const _ixmKey &r ) ;
      // make empty key
      _ixmKeyOwned () {_keyData = NULL; }
   protected:
      StackBufBuilder _b ;
      // create standard BSON object as key
      void _traditional ( const BSONObj &obj ) ;
   } ;
   typedef class _ixmKeyOwned ixmKeyOwned ;

   /*
      _ixmKeyCache define
    */
   class _ixmKeyCache : public _ixmKeyOwned
   {
   private:
      // disable copy
      void operator =( const _ixmKeyCache & ) ;

   public:
      _ixmKeyCache() ;
      ~_ixmKeyCache() ;

      void reset()
      {
         _b.reset() ;
         _keyData = NULL ;
         _bsonBuilder.reset() ;
         _bsonKey = BSONObj() ;
      }

      const BSONObj &getBSONObj() const
      {
         if ( _bsonKey.isEmpty() )
         {
            _convToBSON() ;
         }
         return _bsonKey ;
      }

      void setBSONObj( const BSONObj &obj )
      {
         SDB_ASSERT( !obj.isEmpty(), "invalid BSON object" ) ;
         reset() ;
         _bsonKey = obj ;
      }

      void setKey( const _ixmKey &key )
      {
         reset() ;
         _b.appendBuf( key.data(), key.dataSize() ) ;
         _keyData = (const UINT8 *)( _b.buf() ) ;
      }

   protected:
      void _convToBSON() const ;

   public:
      mutable bson::BSONObjBuilder _bsonBuilder ;
      mutable bson::BSONObj        _bsonKey ;
   } ;

   typedef class _ixmKeyCache ixmKeyCache ;

   /*
      _ixmKeyElement define
    */
   class _ixmKeyIterator : public SDBObject
   {
   public:
      _ixmKeyIterator( const _ixmKey &key )
      : _head( (const UINT8 *)( key.data() ) ),
        _offset( NULL ),
        _next( NULL )
      {
      }

      ~_ixmKeyIterator()
      {
      }

      INT32 woCompare( const BSONElement &r ) const ;
      INT32 woCompare( const _ixmKeyIterator &rKey ) const ;

      BOOLEAN moveNext() const ;
      BOOLEAN hasMore() const ;

   protected:
      const UINT8 *_getNext() const ;

   protected:
      const UINT8 *_head ;
      mutable const UINT8 * _offset ;
      mutable const UINT8 * _next ;
   } ;

   typedef class _ixmKeyIterator ixmKeyIterator ;

}

#endif
