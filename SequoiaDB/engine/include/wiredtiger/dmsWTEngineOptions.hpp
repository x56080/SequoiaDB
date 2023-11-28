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
#include "utilPooledObject.hpp"
#include <boost/filesystem/path.hpp>

namespace engine
{
namespace wiredtiger
{

   #define DMS_WT_CACHE_SIZE_DEF ( 2048 )

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

   protected:
      boost::filesystem::path _dbPath ;
      UINT32 _cacheSizeMB = DMS_WT_CACHE_SIZE_DEF ;
   } ;

   typedef class _dmsWTEngineOptions dmsWTEngineOptions ;

}
}

#endif // DMS_WT_ENGINE_OPTIONS_HPP_
