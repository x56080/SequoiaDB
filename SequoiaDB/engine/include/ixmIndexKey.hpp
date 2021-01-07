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

   Source File Name = ixmIndexKey.hpp

   Descriptive Name = Index Management Index Key Header

   When/how to use: this program may be used on binary and text-formatted
   versions of index management component. This file contains structure for
   index key generation from a given index definition and a data record.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/14/2012  TW  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef IXMINDEXKEY_HPP_
#define IXMINDEXKEY_HPP_

#include "core.hpp"
#include "utilPooledObject.hpp"
#include "../bson/bson.h"
#include "pd.hpp"
#include "ossMemPool.hpp"
#include "utilArray.hpp"

using namespace bson;

namespace engine
{

   // pre-definition
   class _ixmIndexCB ;
   class _ixmIndexKeyGen ;
   class _ixmKeyGenBase ;

   /*
      IXM get undefined key object
    */
   BSONObj ixmGetUndefineKeyObj( INT32 fieldNum ) ;

   /*
      _ixmKeyField define
    */
   // parsed key field in key generator
   class _ixmKeyField : public utilPooledObject
   {
   public:
      _ixmKeyField()
      : _order( 0 ),
        _nameLen( 0 ),
        _name( NULL )
      {
      }

      _ixmKeyField( const _ixmKeyField &field )
      : _order( field._order ),
        _nameLen( field._nameLen ),
        _name( field._name )
      {
      }

      ~_ixmKeyField()
      {
      }

      _ixmKeyField &operator =( const _ixmKeyField &field )
      {
         _order = field._order ;
         _nameLen = field._nameLen ;
         _name = field._name ;
         return ( *this ) ;
      }

      INT32 init( const BSONElement &element ) ;

      OSS_INLINE INT32 getOrder() const
      {
         return _order ;
      }

      OSS_INLINE UINT32 getNameLen() const
      {
         return _nameLen ;
      }

      OSS_INLINE const CHAR *getName() const
      {
         return _name ;
      }

   protected:
      // order of key
      INT32          _order ;
      // name length of key
      UINT32         _nameLen ;
      // field name of key
      const CHAR *   _name ;
   } ;

   typedef class _ixmKeyField ixmKeyField ;
   typedef _utilArray< ixmKeyField > IXM_KEY_FIELD_ARRAY ;

   // for element cache
   typedef _utilArray< BSONElement > IXM_KEY_ELEMENT_ARRAY ;

   /*
      _ixmIndexKeyBuilder define
    */
   class _ixmKeyBuilder : public utilPooledObject
   {
   public:
      _ixmKeyBuilder( BOOLEAN temporary = TRUE )
      : _temporary( temporary )
      {
      }

      ~_ixmKeyBuilder()
      {
      }

      IXM_KEY_ELEMENT_ARRAY &getKeyCache()
      {
         return _keyCache ;
      }

      BSONObjBuilder &getBuilder()
      {
         return _builder ;
      }

      BOOLEAN isTemporary() const
      {
         return _temporary ;
      }

   protected:
      IXM_KEY_ELEMENT_ARRAY   _keyCache ;
      BSONObjBuilder          _builder ;
      BOOLEAN                 _temporary ;
   } ;

   typedef class _ixmKeyBuilder ixmKeyBuilder ;

   // Index KeyGen is the operator to extract keys from given object
   // It depends on its underlying ixmIndexDetails control block
   // ixmIndexKeyGen is local to each thread
   class _ixmIndexKeyGen : public utilPooledObject
   {
   protected:
      // index key pattern
      BSONObj              _keyPattern ;

      // if key fields support array
      BOOLEAN              _notArray ;

      BOOLEAN              _isIDIndex ;

      // number of fields
      UINT32               _nFields ;
      // list of parsed key fields
      IXM_KEY_FIELD_ARRAY  _keyFields ;
      // undefined key contains specified number of elements
      // used to shortcut for generate undefined keys if the given
      // object matches no key pattern
      BSONObj              _undefinedKey ;

      // reusable key builder for building keys
      _ixmKeyBuilder *     _pKeyBuilder ;

      friend class _ixmKeyGenBase ;

   public:
      // default constructor
      _ixmIndexKeyGen() ;
      // create key generator from index control block
      _ixmIndexKeyGen ( const _ixmIndexCB *indexCB ) ;
      // create key generator from key def
      _ixmIndexKeyGen ( const BSONObj &keyDef ) ;
      // destructor
      ~_ixmIndexKeyGen() ;

      OSS_INLINE BOOLEAN isInit() const
      {
         return 0 != _nFields ;
      }

      // set reusable key builder
      OSS_INLINE void setKeyBuilder( _ixmKeyBuilder *pKeyBuilder )
      {
         _pKeyBuilder = pKeyBuilder ;
      }

      // this function overwrite _keyPattern.
      // This will make the ixmIndexKeyGen generate different key than
      // it supposed to
      INT32 setKeyPattern( const BSONObj &keyPattern ) ;

      OSS_INLINE const BSONObj &getKeyPattern() const
      {
         return _keyPattern ;
      }

      OSS_INLINE UINT32 getNFields() const
      {
         return _nFields ;
      }

      // get only one key object from object
      // for array key
      // - if order > 0, get smallest value of array
      // - if order < 0, get largest value of array
      INT32 getKeys ( const BSONObj &obj,
                      BSONObj &keys,
                      BSONElement *pArrEle = NULL,
                      BOOLEAN keepKeyName = FALSE,
                      BOOLEAN ignoreUndefined = FALSE ) ;
      // get key set from object
      // for array key, generate all possible values from array
      INT32 getKeys ( const BSONObj &obj,
                      BSONObjSet &keySet,
                      BSONElement *pArrEle = NULL,
                      BOOLEAN keepKeyName = FALSE,
                      BOOLEAN ignoreUndefined = FALSE ) ;

      static BOOLEAN validateKeyDef ( const BSONObj &keyDef ) ;

      void  setNotArray ( const BOOLEAN notArray )
      {
         _notArray = notArray ;
      }

      void  setIsIDIndex ( const BOOLEAN isIDIndex )
      {
         _isIDIndex = isIDIndex ;
      }

   protected:
      // disable copy
      _ixmIndexKeyGen( const _ixmIndexKeyGen & ) {}
      _ixmIndexKeyGen &operator =( const _ixmIndexKeyGen & ) { return *this ; }

      INT32 _init() ;
      void _release() ;

      // implement of get keys
      INT32 _getKeys( _ixmKeyGenBase *keyGen ) ;

      // extract keys from object
      INT32 _extractKeys( const BSONObj &obj,
                          IXM_KEY_ELEMENT_ARRAY &keyCache,
                          BOOLEAN &allUndefined,
                          BSONElement &arrEle,
                          const CHAR *&arrEleName,
                          INT32 &arrElePos ) ;

      // build keys
      INT32 _buildKeys( _ixmKeyBuilder *pBuilder,
                        BOOLEAN keepKeyName,
                        BOOLEAN ignoreUndefined,
                        BSONObj &keys ) ;

      // extract key from array
      INT32 _extractArrayKey( const BSONElement &arrEle,
                              const CHAR *arrEleName,
                              _ixmKeyGenBase *keyGen ) ;
   } ;
   typedef class _ixmIndexKeyGen ixmIndexKeyGen ;
}

#endif //IXMINDEXKEY_HPP_

