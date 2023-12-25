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

   Source File Name = dmsWTStorageService.hpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          11/20/2023  HGM Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef DMS_WT_STORAGE_SERVICE_HPP_
#define DMS_WT_STORAGE_SERVICE_HPP_

#include "interface/IStorageService.hpp"
#include "interface/ICollection.hpp"
#include "ossUtil.hpp"
#include "ossRWMutex.hpp"
#include "wiredtiger/dmsWTEngineOptions.hpp"
#include "wiredtiger/dmsWTPersistUnit.hpp"
#include "wiredtiger/dmsWTStorageEngine.hpp"

namespace engine
{
namespace wiredtiger
{

   /*
      _dmsWTStorageService define
    */
   class _dmsWTStorageService : public IStorageService
   {
   public:
      _dmsWTStorageService() ;
      virtual ~_dmsWTStorageService() ;
      _dmsWTStorageService( const _dmsWTStorageService &o ) = delete ;
      _dmsWTStorageService &operator =( const _dmsWTStorageService & ) = delete ;

   public:
      virtual DMS_STORAGE_ENGINE_TYPE getEngineType() const
      {
         return DMS_STORAGE_ENGINE_WIREDTIGER ;
      }

      virtual INT32 openEngine( const dmsOpenEngineOptions &options ) ;
      virtual INT32 closeEngine( const dmsCloseEngineOptions &options ) ;

      virtual IStorageEngine *getEngine()
      {
         return &_engine ;
      }

      const dmsWTEngineOptions &getEngineOptions() const
      {
         return _engineOptions ;
      }

      virtual INT32 sync( BOOLEAN force, BOOLEAN sync, IExecutor *executor ) ;
      virtual INT32 getPersistUnit( IExecutor *executor, IPersistUnit *&persistUnit ) ;

      virtual INT32 createCS( const dmsCSMetadata &metadata,
                              const dmsCreateCSOptions &options,
                              IExecutor *executor ) ;
      virtual INT32 dropCS( const dmsCSMetadata &metadata,
                            const dmsDropCSOptions &options,
                            IExecutor *executor ) ;

      virtual INT32 createCL( const dmsCLMetadata &metadata,
                              const dmsCreateCLOptions &options,
                              IExecutor *executor ) ;
      virtual INT32 dropCL( const dmsCLMetadata &metadata,
                            const dmsDropCLOptions &options,
                            IExecutor *executor ) ;

      virtual INT32 getCollection( const dmsCLMetadataKey &metadataKey,
                                   IExecutor *executor,
                                   std::shared_ptr<ICollection> &collPtr ) ;
      virtual INT32 loadCollection( const dmsCLMetadata &metadata,
                                    IExecutor *executor,
                                    std::shared_ptr<ICollection> &collPtr ) ;

      virtual BOOLEAN isDurable() const
      {
         return TRUE ;
      }

      virtual BOOLEAN isCompressorAlterable() const
      {
         return FALSE ;
      }

   protected:
      INT32 _initEngineOptions( dmsWTEngineOptions &options ) ;
      INT32 _checkDBPath( const boost::filesystem::path &dbPath ) ;
      INT32 _buildConfigString( const dmsWTEngineOptions &options,
                                ossPoolString &configString ) ;

      INT32 _dumpURIListByCS( utilCSUniqueID csUID,
                              ossPoolList< ossPoolString > &uriList ) ;

      INT32 _addCollection( const dmsCLMetadata &metadata,
                            const dmsWTStore &store,
                            std::shared_ptr<ICollection> &collPtr ) ;
      void _removeCollection( const dmsCLMetadataKey &metadataKey ) ;
      void _removeCollections( utilCSUniqueID csUID ) ;
      std::shared_ptr<ICollection> _getCollection( const dmsCLMetadataKey &metadataKey ) ;

   protected:
      dmsWTEngineOptions _engineOptions ;
      dmsWTStorageEngine _engine ;

      typedef ossPoolMap<dmsCLMetadataKey,
                         std::shared_ptr<ICollection>> _dmsWTCollMap ;
      typedef _dmsWTCollMap::iterator _dmsWTCollMapIter ;
      _dmsWTCollMap _collMap ;
      ossRWMutex _collMapMutex ;
   } ;

   typedef class _dmsWTStorageService dmsWTStorageService ;

}
}

#endif // DMS_WT_STORAGE_SERVICE_HPP_
