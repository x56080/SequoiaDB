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

   Source File Name = dmsWTEngineOptions.hpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          11/20/2023  HGM Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef DMS_WT_ENGINE_OPTIONS_HPP_
#define DMS_WT_ENGINE_OPTIONS_HPP_

#include "dmsDef.hpp"
#include "wiredtiger/dmsWTDef.hpp"
#include "utilPooledObject.hpp"
#include <boost/filesystem/path.hpp>

namespace engine
{
namespace wiredtiger
{

   /*
      _dmsWTEngineOptions define
    */
   class _dmsWTEngineOptions : public _utilPooledObject
   {
   public:
      _dmsWTEngineOptions() = default ;
      virtual ~_dmsWTEngineOptions() = default ;
      _dmsWTEngineOptions( const _dmsWTEngineOptions &o ) = default ;
      _dmsWTEngineOptions &operator =( const _dmsWTEngineOptions & ) = default ;

   public:
      const boost::filesystem::path &getDBPath() const
      {
         return _dbPath ;
      }

      void setDBPath( const boost::filesystem::path &dbPath )
      {
         _dbPath = dbPath ;
      }

      UINT32 getCacheSizeMB() const
      {
         return _cacheSizeMB ;
      }

      void setCacheSizeMB( UINT32 cacheSizeMB )
      {
         _cacheSizeMB = cacheSizeMB ;
      }

      UINT32 getEvictTarget() const
      {
         return _evictTarget ;
      }

      void setEvictTarget( UINT32 evictTarget )
      {
         _evictTarget = evictTarget ;
      }

      UINT32 getEvictTrigger() const
      {
         return _evictTrigger ;
      }

      void setEvictTrigger( UINT32 evictTrigger )
      {
         _evictTrigger = evictTrigger ;
      }

      UINT32 getEvictDirtyTarget() const
      {
         return _evictDirtyTarget ;
      }

      void setEvictDirtyTarget( UINT32 evictDirtyTarget )
      {
         _evictDirtyTarget = evictDirtyTarget ;
      }

      UINT32 getEvictDirtyTrigger() const
      {
         return _evictDirtyTrigger ;
      }

      void setEvictDirtyTrigger( UINT32 evictDirtyTrigger )
      {
         _evictDirtyTrigger = evictDirtyTrigger ;
      }

      UINT32 getEvictUpdatesTarget() const
      {
         return _evictUpdatesTarget ;
      }

      void setEvictUpdatesTarget( UINT32 evictUpdatesTarget )
      {
         _evictUpdatesTarget = evictUpdatesTarget ;
      }

      UINT32 getEvictUpdatesTrigger() const
      {
         return _evictUpdatesTrigger ;
      }

      void setEvictUpdatesTrigger( UINT32 evictUpdatesTrigger )
      {
         _evictUpdatesTrigger = evictUpdatesTrigger ;
      }

      UINT32 getEvictThreadsMin() const
      {
         return _evictThreadsMin ;
      }

      void setEvictThreadsMin( UINT32 evictThreadsMin )
      {
         _evictThreadsMin = evictThreadsMin ;
      }

      UINT32 getEvictThreadsMax() const
      {
         return _evictThreadsMax ;
      }

      void setEvictThreadsMax( UINT32 evictThreadsMax )
      {
         _evictThreadsMax = evictThreadsMax ;
      }

      UINT32 getCheckPointInterval() const
      {
         return _checkPointInterval ;
      }

      void setCheckPointInterval( UINT32 checkPointInterval )
      {
         _checkPointInterval = checkPointInterval ;
      }

      UINT32 getCheckPointLogSize() const
      {
         return _checkPointLogSize ;
      }

      void setCheckPointLogSize( UINT32 checkPointLogSize )
      {
         _checkPointLogSize = checkPointLogSize ;
      }

      void fixOptions()
      {
         if ( _evictTarget > _evictTrigger )
         {
            _evictTarget = _evictTrigger ;
         }
         if ( _evictDirtyTarget > _evictDirtyTrigger )
         {
            _evictDirtyTarget = _evictDirtyTrigger ;
         }
         if ( _evictDirtyTarget > _evictTarget )
         {
            _evictDirtyTarget = _evictTarget ;
         }
         if ( _evictDirtyTrigger > _evictTrigger )
         {
            _evictDirtyTrigger = _evictTrigger ;
         }
         if ( _evictUpdatesTarget > _evictUpdatesTrigger )
         {
            _evictUpdatesTarget = _evictUpdatesTrigger ;
         }
         if ( _evictUpdatesTarget > _evictDirtyTarget )
         {
            _evictUpdatesTarget = _evictDirtyTarget ;
         }
         if ( _evictUpdatesTrigger > _evictDirtyTrigger )
         {
            _evictUpdatesTrigger = _evictDirtyTrigger ;
         }
      }

   protected:
      boost::filesystem::path _dbPath ;
      UINT32 _cacheSizeMB = DMS_DFT_WT_CACHE_SIZE ;
      UINT32 _evictTarget = DMS_DFT_WT_EVICT_TARGET ;
      UINT32 _evictTrigger = DMS_DFT_WT_EVICT_TRIGGER ;
      UINT32 _evictDirtyTarget = DMS_DFT_WT_EVICT_DIRTY_TARGET ;
      UINT32 _evictDirtyTrigger = DMS_DFT_WT_EVICT_DIRTY_TRIGGER ;
      UINT32 _evictUpdatesTarget = DMS_DFT_WT_EVICT_UPDATES_TARGET ;
      UINT32 _evictUpdatesTrigger = DMS_DFT_WT_EVICT_UPDATES_TRIGGER ;
      UINT32 _evictThreadsMin = DMS_DFT_WT_EVICT_THREADS_MIN ;
      UINT32 _evictThreadsMax = DMS_DFT_WT_EVICT_THREADS_MAX ;
      UINT32 _checkPointInterval = DMS_DFT_WT_CHECK_POINT_INTERVAL ;
      UINT32 _checkPointLogSize = DMS_DFT_WT_CHECK_POINT_LOG_SIZE ;
   } ;

   typedef class _dmsWTEngineOptions dmsWTEngineOptions ;

}
}

#endif // DMS_WT_ENGINE_OPTIONS_HPP_
