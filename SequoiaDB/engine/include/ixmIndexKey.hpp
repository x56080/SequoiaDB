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
        _name( field._name ),
        _keyElement( field._keyElement )
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
         _keyElement = field._keyElement ;

         return ( *this ) ;
      }

      INT32 init( const BSONElement &element, BOOLEAN needOrder ) ;
      INT32 getOrder( const BSONObj &keyPattern, INT32 &order ) ;

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

      OSS_INLINE void setKeyEle( const BSONElement &element )
      {
         _keyElement = element ;
      }

      OSS_INLINE const BSONElement &getKeyEle() const
      {
         return _keyElement ;
      }

   protected:
      // order of key
      INT32          _order ;
      // name length of key
      UINT32         _nameLen ;
      // field name of key
      const CHAR *   _name ;
      // cache of key element during generation
      BSONElement    _keyElement ;
   } ;

   typedef class _ixmKeyField ixmKeyField ;
   typedef _utilArray< ixmKeyField > IXM_KEY_FIELD_ARRAY ;

   // Index KeyGen is the operator to extract keys from given object
   // It depends on its underlying ixmIndexDetails control block
   // ixmIndexKeyGen is local to each thread
   class _ixmIndexKeyGen : public utilPooledObject
   {
   protected:
      // index key pattern
      BSONObj              _keyPattern ;

      // number of fields
      UINT32               _nFields ;
      // list of parsed key fields
      IXM_KEY_FIELD_ARRAY  _keyFields ;
      // key builder contains buffer which could be reused
      // WARNING: should not take memory owned from this builder
      BSONObjBuilder       _keyBuilder ;
      // undefined key contains specified number of elements
      // used to shortcut for generate undefined keys if the given
      // object matches no key pattern
      BSONObj              _undefinedKey ;

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

   protected:
      // disable copy
      _ixmIndexKeyGen( const _ixmIndexKeyGen & ) {}
      _ixmIndexKeyGen &operator =( const _ixmIndexKeyGen & ) { return *this ; }

      BOOLEAN _isInit() const
      {
         return 0 != _nFields ;
      }

      INT32 _init( BOOLEAN needOrder ) ;
      void _release() ;

      // implement of get keys
      INT32 _getKeys( _ixmKeyGenBase *keyGen ) ;

      // extract keys from object
      INT32 _extractKeys( const BSONObj &obj,
                          BOOLEAN &allUndefined,
                          BSONElement &arrEle,
                          const CHAR *&arrEleName,
                          INT32 &arrElePos ) ;

      // build keys
      INT32 _buildKeys( BOOLEAN keepKeyName,
                        BOOLEAN ignoreUndefined,
                        BSONObj &keys ) ;

      // save array key into output
      INT32 _saveArrayKey( const BSONElement &arrEle,
                           ixmKeyField &arrKeyField,
                           BOOLEAN keepKeyName,
                           BOOLEAN ignoreUndefined,
                           BSONObjSet *keySet,
                           BSONElement *foundArrEle ) ;

      // extract key from array
      INT32 _extractArrayKey( const BSONElement &arrEle,
                              const CHAR *arrEleName,
                              _ixmKeyGenBase *keyGen ) ;

      // build a key into key set
      INT32 _buildKeySet( BOOLEAN keepKeyName,
                          BOOLEAN ignoreUndefined,
                          BSONObjSet &keySet ) ;
   } ;
   typedef class _ixmIndexKeyGen ixmIndexKeyGen ;
}

#endif //IXMINDEXKEY_HPP_

