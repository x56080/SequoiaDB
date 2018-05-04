/*******************************************************************************

   Copyright (C) 2011-2018 SequoiaDB Ltd.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

   http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

   Source File Name = rtnLobFetcher.hpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          07/31/2014  YW  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef RTN_LOBFETCHER_HPP_
#define RTN_LOBFETCHER_HPP_

#include "dmsLobDef.hpp"
#include "rtn.hpp"

namespace engine
{
   class _rtnLobFetcher : public SDBObject
   {
   public:
      _rtnLobFetcher() ;
      ~_rtnLobFetcher() ;

   public:
      INT32 init( const CHAR *fullName,
                  BOOLEAN onlyMetaPage ) ;

      INT32 fetch( _pmdEDUCB *cb,
                   _dmsLobInfoOnPage &piece,
                   _dpsMessageBlock *mb = NULL ) ;

      void  close( INT32 cause = SDB_DMS_EOC ) ;

      BOOLEAN hitEnd() const
      {
         return SDB_DMS_EOC == _lastErr ||
                SDB_DMS_NOTEXIST == _lastErr ;
      }

      DMS_LOB_PAGEID toBeFetched() const
      {
         return _pos ;
      }

      _dmsStorageUnit *getSu()
      {
         return _su ;
      }

      _dmsMBContext *getMBContext()
      {
         return _mbContext ;
      }

      const CHAR* collectionName() const
      {
         return _fullName ;
      }

   private:
      void _fini() ;
   private:
      dmsStorageUnitID     _suID ;
      _dmsStorageUnit      *_su ;
      _dmsMBContext        *_mbContext ;
      DMS_LOB_PAGEID       _pos ;
      BOOLEAN              _onlyMetaPage ;
      INT32                _lastErr ;
      CHAR                 _fullName[ DMS_COLLECTION_FULL_NAME_SZ + 1 ] ;

   } ;
   typedef class _rtnLobFetcher rtnLobFetcher ;
}

#endif

