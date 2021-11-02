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
#include "vessel/slice.h"

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
      
      UINT32 getFieldCount()const;
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

   private:
      StackBufBuilder _b ;
      // create standard BSON object as key
      void _traditional ( const BSONObj &obj ) ;
   } ;
   typedef class _ixmKeyOwned ixmKeyOwned ;

   class ixmKeyCompressor : public SDBObject
   {
      public:
         ixmKeyCompressor(){}
         ~ixmKeyCompressor(){}
         ixmKeyCompressor(const ixmKeyCompressor &) = delete;
         ixmKeyCompressor &operator=(const ixmKeyCompressor &) = delete;

      public:
         class result : public SDBObject
         {
            friend class ixmKeyCompressor;
            public:
               result(){}
               ~result(){}
               result(const result &) = delete;
               result &operator=(const result &) = delete;

            public:
               OSS_INLINE void reset()
               {
                  _fieldsCompressed = 0;
                  _suffixBuilder.reset();
               }
               OSS_INLINE BOOLEAN ok()const
               {
                  return 0 < _fieldsCompressed;
               }
               OSS_INLINE UINT32 getFieldsCompressed()const
               {
                  return _fieldsCompressed;
               }
               OSS_INLINE BOOLEAN isPerfectlyCompressed()const
               {
                  return ok() && 0 == _suffixBuilder.len();
               }
               OSS_INLINE void getSuffuix(ixmKey &suffix)const
               {
                  if (ok() && 0 < _suffixBuilder.len())
                  {
                     suffix.assign(_suffixBuilder.buf());
                  }
                  else
                  {
                     suffix.assign(NULL);
                  }
                  return;
               }

            private:
               UINT32 _fieldsCompressed = 0;
               StackBufBuilder _suffixBuilder;
         };//class result

      public:
         void reset()
         {
            _prefix = NULL;
            _nfields = 0;
         }
         INT32 initPrefix(UINT32 nfields, const CHAR *prefix);
         INT32 compress(const ixmKey &key, result &r)const;

         static BOOLEAN buildUncompressedKey(const ixmKey &prefix,
                                             const ixmKey &suffix,
                                             StackBufBuilder &builder);

      private:
         BOOLEAN compressColumn(const CHAR *prefix,
                                const CHAR *key,
                                BOOLEAN hasMore,
                                StackBufBuilder &builder)const;

         void saveColumnsAsSuffix(const ixmKey &key,
                                  StackBufBuilder &builder)const;
      private:
         const CHAR *_prefix = NULL;
         UINT32 _nfields = 0;
   };//class ixmKeyCompressor

   class ixmKeyPrefixGenerator : public SDBObject
   {
      public:
         ixmKeyPrefixGenerator(){}
         ~ixmKeyPrefixGenerator(){}
         ixmKeyPrefixGenerator(const ixmKeyPrefixGenerator &) = delete;
         ixmKeyPrefixGenerator &operator=(const ixmKeyPrefixGenerator &) = delete;

      public:
         class result : public SDBObject
         {
            friend class ixmKeyPrefixGenerator;
            public:
               result(){}
               ~result(){}
               result(const result &) = delete;
               result &operator=(const result &) = delete;

            public:
               OSS_INLINE void reset()
               {
                  _prefixBuilder.reset();
               }
               OSS_INLINE BOOLEAN ok()const
               {
                  return 0 < _prefixBuilder.len();
               }
               OSS_INLINE void getPrefix(ixmKey &prefix)const
               {
                  prefix.assign(ok() ? _prefixBuilder.buf() : NULL);
               }

            private:
               StackBufBuilder _prefixBuilder;
         };//class result

      public:
         BOOLEAN generate(UINT32 prefixFieldNum,
                          const ixmKey &l,
                          const ixmKey &r,
                          result &res)const;

      private:
         BOOLEAN extractCommonPrefix(const UINT8 *l,
                                     const UINT8 *r,
                                     BOOLEAN lastColumn,
                                     StackBufBuilder &builder)const;
      
   };//class ixmKeyPrefixGenerator

   class ixmKeyUtils : public SDBObject
   {
      public:
         static void buildMinKey(UINT32 nfields,
                                 const bson::Ordering &ordering,
                                 StackBufBuilder &builder);
   };//class ixmKeyUtils
}

#endif
