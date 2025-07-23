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

   Source File Name = coordCacheAssist.hpp

   Descriptive Name = Coord cache assistent

   When/how to use: this program may be used on binary and text-formatted
   versions of runtime component. This file contains code logic for
   common functions for coordinator node.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          06/04/2021  YSD Init
   Last Changed =

*******************************************************************************/
#ifndef COORD_CACHE_ASSIST_HPP__
#define COORD_CACHE_ASSIST_HPP__

#include "oss.hpp"
#include "../bson/bsonobj.h"
#include "ossMemPool.hpp"

using bson::BSONObj ;

namespace engine
{
   class _pmdEDUCB ;
   class _coordResource ;
   class _clsCatalogSet ;

   enum _coordCacheType
   {
      COORD_CACHE_INVALID = 0,
      COORD_CACHE_CATALOGUE = 1,
      COORD_CACHE_GROUP,
      COORD_CACHE_DATASOURCE,
      COORD_CACHE_STRATEGY
   } ;
   typedef _coordCacheType coordCacheType ;

   const CHAR *coordCacheTypeStr( coordCacheType type ) ;
   coordCacheType coordStr2CacheType( const CHAR *typeStr ) ;

   class _coordCacheInvalidator : public SDBObject
   {
   public:
      _coordCacheInvalidator( _coordResource *resource ) ;
      ~_coordCacheInvalidator() ;

      /**
       * @brief Invalidate local cache specified by type and name.
       * @param type Cache type
       * @name  name Name of the object to be cleaned.
       */
      void invalidate( coordCacheType type, const CHAR *name = NULL ) ;

      /**
       * @brief Invalidate local cache specified by option.
       * @param option Clean option. If empty, all kinds of caches will be
       *               cleaned, just like 'invalidateAll'. The options include:
       *               Type: Cache type.
       *               Name: one or more object names of type specified by
       *                     'Type'.
       */
      INT32 invalidate( const BSONObj &option ) ;

      void invalidateAll() ;

      /**
       * Notify coordinators to invalidate cache, specified by type and name.
       * If name is NULL, all cache of the specified type will be invalidated.
       */
      INT32 notify( coordCacheType type, const CHAR *name, _pmdEDUCB *cb ) ;

      /**
       * Notify cache of one or more object of type specified by 'type'.
       */
      INT32 notify( coordCacheType type, ossPoolVector<string> &names,
                    _pmdEDUCB *cb ) ;

   private:
      void _invalidateCatalogue( const CHAR *name ) ;

      INT32 _notify( const BSONObj &arguments, _pmdEDUCB *cb ) ;

   private:
      _coordResource *_resource ;
   } ;
   typedef _coordCacheInvalidator coordCacheInvalidator ;

}

#endif /* COORD_CACHE_ASSIST_HPP__ */
