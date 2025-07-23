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

using namespace bson;

namespace engine
{
   /*
      IXM get undefine key object
   */
   BSONObj ixmGetUndefineKeyObj( INT32 fieldNum ) ;

   enum IndexSuitability { USELESS = 0 , HELPFUL = 1 , OPTIMAL = 2 };
   class _ixmIndexCB ;

   enum IXM_KEYGEN_TYPE
   {
      GEN_OBJ_NO_FIELD_NAME         = 1,
      GEN_OBJ_KEEP_FIELD_NAME       = 2,
      GEN_OBJ_ARRAY_FIELD_NAME      = 3
   } ;

   // Index KeyGen is the operator to extract keys from given object
   // It depends on its underlying ixmIndexDetails control block
   // ixmIndexKeyGen is local to each thread
   class _ixmIndexKeyGen : public utilPooledObject
   {
   protected:
      INT32 indexVersion() const ;
      IndexSuitability _suitability( const BSONObj& query ,
                                     const BSONObj& order ) const ;
      //BSONSizeTracker _sizeTracker ;
      ossPoolVector<const CHAR*>    _fieldNames ; // vector contains all fields
      ossPoolVector<BSONElement>    _fixedElements ; // dummy element for KeyGenerator
      BSONObj _undefinedKey ;

      INT32                _nFields ; // number of fields
      // index key pattern
      BSONObj              _keyPattern ;
      BSONObj              _info ;
      UINT16               _type ;

      IXM_KEYGEN_TYPE      _keyGenType ;

      //const _ixmIndexCB *_indexCB ;
      void _init() ;
      friend class _ixmKeyGenerator ;
   public:
      // create key generator from index control block
      _ixmIndexKeyGen ( const _ixmIndexCB *indexCB,
                        IXM_KEYGEN_TYPE genType = GEN_OBJ_NO_FIELD_NAME ) ;
      // create key generator from key def
      _ixmIndexKeyGen ( const BSONObj &keyDef,
                        IXM_KEYGEN_TYPE genType = GEN_OBJ_NO_FIELD_NAME ) ;
      // this function overwrite _keyPattern and _info with a new index info
      // object. This will make the ixmIndexKeyGen generate different key than
      // it supposed to
      INT32 reset ( const BSONObj & info ) ;
      INT32 reset ( const _ixmIndexCB *indexCB ) ;
      INT32 getKeys ( const BSONObj &obj, BSONObjSet &keys,
                      BSONElement *pArrEle = NULL,
                      BOOLEAN isKeepKeyName = FALSE,
                      BOOLEAN ignoreUndefined = FALSE ) const ;
      BSONElement missingField() const ;
      IndexSuitability suitability( const BSONObj &query ,
                                    const BSONObj &order ) const ;
      static BOOLEAN validateKeyDef ( const BSONObj &keyDef ) ;
   } ;
   typedef class _ixmIndexKeyGen ixmIndexKeyGen ;
}

#endif //IXMINDEXKEY_HPP_

