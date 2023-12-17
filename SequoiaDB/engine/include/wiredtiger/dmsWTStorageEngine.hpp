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

   Source File Name = dmsWTStorageEngine.hpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          11/20/2023  HGM Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef DMS_WT_STORAGE_ENGINE_HPP_
#define DMS_WT_STORAGE_ENGINE_HPP_

#include "interface/IStorageEngine.hpp"
#include "ossTypes.h"
#include "sdbIPersistence.hpp"
#include "wiredtiger/dmsWTCursor.hpp"
#include "wiredtiger/dmsWTItem.hpp"
#include "wiredtiger/dmsWTSession.hpp"
#include "wiredtiger/dmsWTStore.hpp"
#include "wiredtiger/dmsWTUtil.hpp"
#include "wiredtiger/dmsWTHandler.hpp"

#include <boost/filesystem/path.hpp>
#include <wiredtiger.h>

namespace engine
{
namespace wiredtiger
{

   /*
      _dmsWTStorageEngine define
    */
   class _dmsWTStorageEngine : public IStorageEngine, public IDataSyncBase
   {
   public:
      _dmsWTStorageEngine( dmsWTEngineOptions &options ) ;
      virtual ~_dmsWTStorageEngine() ;
      _dmsWTStorageEngine( const _dmsWTStorageEngine &o ) = delete ;
      _dmsWTStorageEngine &operator =( const _dmsWTStorageEngine & ) = delete ;

   public:
      virtual DMS_STORAGE_ENGINE_TYPE getEngineType() const
      {
         return DMS_STORAGE_ENGINE_WIREDTIGER ;
      }

      const dmsWTEngineOptions &getOptions() const
      {
         return _options ;
      }

      INT32 open( const boost::filesystem::path &dbPath,
                  const CHAR *config ) ;
      INT32 close( const CHAR *config ) ;

      INT32 openSession( dmsWTSession &session ) ;

      INT32 createStore( const CHAR *uri,
                         const CHAR *config,
                         dmsWTStore &store ) ;

      INT32 dropStore( const CHAR *uri,
                       const CHAR *config ) ;
      INT32 dropStores( const ossPoolList<ossPoolString> &uris,
                        const CHAR *config ) ;

      INT32 truncateStore( const CHAR *uri,
                           const CHAR *config ) ;

      INT32 insertToStore( const dmsWTStore &store,
                           UINT64 key,
                           const dmsWTItem &value ) ;
      INT32 updateToStore( const dmsWTStore &store,
                           UINT64 key,
                           const dmsWTItem &value ) ;
      INT32 removeFromStore( const dmsWTStore &store,
                             UINT64 key ) ;
      INT32 extractFromStore( const dmsWTStore &store,
                              UINT64 key,
                              dmsWTItem &value ) ;
      INT32 insertToStore( dmsWTCursor &cursor,
                           const dmsWTItem &key,
                           const dmsWTItem &value ) ;
      INT32 updateToStore( const dmsWTStore &store,
                           const dmsWTItem &key,
                           const dmsWTItem &value ) ;
      INT32 removeFromStore( const dmsWTStore &store,
                             const dmsWTItem &key ) ;
      INT32 extractFromStore( const dmsWTStore &store,
                              const dmsWTItem &key,
                              dmsWTItem &value ) ;
      INT32 loadStore( const CHAR *uri,
                       dmsWTStore &store ) ;
      INT32 openStoreCursor( const CHAR *uri,
                             const CHAR *config,
                             dmsWTCursor &cursor ) ;

      INT32 dumpURIListByPrefix( const CHAR *prefix,
                                 ossPoolList< ossPoolString > &uriList ) ;
      INT32 dumpURIList( ossPoolList< ossPoolString > &uriList ) ;

      // for persistence
      virtual BOOLEAN isClosed() const
      {
         return nullptr == _conn ;
      }

      virtual BOOLEAN canSync( BOOLEAN &force ) const ;

      virtual INT32 sync( BOOLEAN force, BOOLEAN sync, IExecutor *executor ) ;

      virtual void lock()
      {
         _persistLatch.get() ;
      }

      virtual void unlock()
      {
         _persistLatch.release() ;
      }

   protected:
      INT32 _checkDBPath( const boost::filesystem::path &dbPath ) ;

   protected:
      dmsWTEngineOptions &_options ;
      dmsWTHandler _handler ;
      WT_CONNECTION *_conn = nullptr ;
      ossSpinXLatch _persistLatch ;
      UINT64 _lastPersistTick = 0 ;
   } ;

   typedef class _dmsWTStorageEngine dmsWTStorageEngine ;

}
}

#endif // DMS_WT_STORAGE_ENGINE_HPP_
