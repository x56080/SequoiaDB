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

   Source File Name = omConfigMySQL.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          06/13/2018  HJW Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef OM_CONFIG_MYSQL_HPP_
#define OM_CONFIG_MYSQL_HPP_ 

#include "omConfigBuilder.hpp"

namespace engine
{
   class OmMySQLConfigBuilder ;

   class OmMySQLNode: public OmNode
   {
   public:
      OmMySQLNode() ;
      virtual ~OmMySQLNode() ;

   public:
      const string& getDBPath() const { return _dbPath ; }
      const string& getServiceName() const { return _serviceName ; }

   private:
      INT32 _init( const BSONObj& bsonNode, OmHost& host, OmCluster& cluster ) ;
      INT32 _setServiceName( const string& serviceName ) ;
      INT32 _setDBPath( const string& dbPath, OmHost& host ) ;

   private:
      string _dbPath ;
      string _serviceName ;

      friend class OmMySQLConfigBuilder ;
   } ;

   class OmMySQLConfTemplate : public OmConfTemplate
   {
   public:
      OmMySQLConfTemplate() ;
      virtual ~OmMySQLConfTemplate() ;
   } ;

   class OmMySQLConfProperties : public OmConfProperties
   {
   public:
      OmMySQLConfProperties() ;
      virtual ~OmMySQLConfProperties() ;

   public:
      bool isPrivateProperty( const string& name ) const ;
      bool isAllPropertySet() ;
   } ;

   class OmMySQLConfigBuilder: public OmConfigBuilder
   {
   public:
      OmMySQLConfigBuilder( const OmBusinessInfo& businessInfo ) ;
      virtual ~OmMySQLConfigBuilder() ;

   private:
      OmConfTemplate& _getConfTemplate() { return _template ; }
      OmConfProperties& _getConfProperties() { return _properties ; }
      INT32 _build( BSONArray &nodeConfig ) ;
      INT32 _check( BSONObj& bsonConfig ) ;
      INT32 _checkAndAddNode( const BSONObj& bsonNode ) ;
      bool  _isServiceNameUsed( const OmHost& host, const string& serviceName ) ;
      void  _setLocal() ;
      INT32 _getServiceName( const OmHost& host, string& serviceName ) ;
      INT32 _getDBPath( OmHost& host, const string& diskPath,
                        const string& businessType, const string& serviceName,
                        string& dbPath ) ;
      INT32 _createNode() ;

   private:
      string                  _localHostName ;
      string                  _defaultServicePort ;
      OmMySQLConfTemplate     _template ;
      OmMySQLConfProperties   _properties ;

   } ;
}

#endif /* OM_CONFIG_SDB_HPP_ */
