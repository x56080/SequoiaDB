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

   Source File Name = authResource.hpp

   Descriptive Name = 

   When/how to use: this program may be used on backup or restore db data.
   You can specfiy some options from parameters.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          07/14/2023  ZHY Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef AUTH_RESOURCE_HPP__
#define AUTH_RESOURCE_HPP__

#include "oss.hpp"
#include "ossTypes.h"
#include "../bson/bson.hpp"
#include "authRBAC.hpp"
#include <boost/optional/optional.hpp>

namespace engine
{
#define AUTH_RESOURCE_CS_FIELD_NAME "cs"
#define AUTH_RESOURCE_CL_FIELD_NAME "cl"
#define AUTH_RESOURCE_CLUSTER_FIELD_NAME "Cluster"
#define AUTH_RESOURCE_ANY_FIELD_NAME "AnyResource"
#define AUTH_RESOURCE_INVALID_FIELD_NAME "Invalid"

   class _authResource : public SDBObject
   {
   public:
      static boost::shared_ptr<_authResource> fromBson( const bson::BSONObj &obj );
      static boost::shared_ptr<_authResource> forCluster();
      static boost::shared_ptr<_authResource> forCS( const ossPoolString &cs );
      static boost::shared_ptr<_authResource> forCL( const ossPoolString &cl );
      static boost::shared_ptr< _authResource > forExact( const ossPoolString &cs,
                                                          const ossPoolString &cl );
      static BOOLEAN isExactName( const CHAR *clFullName);
      // ensure that parameter clFullName is a full name by calling isExactName
      static boost::shared_ptr< _authResource > forExact( const CHAR *clFullName );
      static boost::shared_ptr<_authResource> forNonSystem();
      static boost::shared_ptr<_authResource> forAny();
      // Cluster, NonSystem, Any are simple types, which means they don't have cs and cl fields.
      static boost::shared_ptr<_authResource> forSimpleType( RESOURCE_TYPE t );

   private:
      _authResource( RESOURCE_TYPE t );
      _authResource( const _authResource & );
      void operator=(const _authResource &);

   public:
      _authResource();
      ~_authResource();
      BOOLEAN operator==( const _authResource &r ) const;
      bool operator<(const _authResource &o) const;

      RESOURCE_TYPE getType() const;

      // This is a function to check if the resource is included in another resource.
      // For example, the resource { cs:"cs1", "cl:cl1" } is included in
      // { cs:"cs1", "cl:cl1" }, { cs:"", cl:"cl1" }, { cs:"cs1", cl:"" }, { cs: "", cl: "" } and
      // { Any: true }
      BOOLEAN isIncluded(const _authResource &other) const;

      void toBSONObj( bson::BSONObjBuilder &builder) const;

   private:
      RESOURCE_TYPE _t;
      boost::optional< ossPoolString > _cs;
      boost::optional< ossPoolString > _cl;
   };
   typedef _authResource authResource;
} // namespace engine

#endif