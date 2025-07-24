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

   Source File Name = omConfigSdb.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          18/4/2016  David Li Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef OM_CONFIG_SDB_HPP_
#define OM_CONFIG_SDB_HPP_ 

#include "omConfigBuilder.hpp"

namespace engine
{
   class OmSdbConfigBuilder ;

   class OmSdbNode: public OmNode
   {
   public:
      OmSdbNode() ;
      virtual ~OmSdbNode() ;

   public:
      const string& getDBPath() const { return _dbPath ; }
      const string& getServiceName() const { return _serviceName ; }
      const string& getRole() const { return _role ; }
      const string& getGroupName() const { return _groupName ; }
      const string& getTransaction() const { return _transaction ; }

   private:
      INT32 _init( const BSONObj& bsonNode, OmHost& host, OmCluster& cluster ) ;
      INT32 _setServiceName( const string& serviceName ) ;
      INT32 _setDBPath( const string& dbPath, OmHost& host ) ;
      void  _setRole( const string& role ) ;
      void  _setGroupName( const string& groupName ) ;
      void  _setTransaction( const string& transaction ) ;

   private:
      string _dbPath ;
      string _serviceName ;
      string _role ;
      string _groupName ;
      string _transaction ;

      friend class OmSdbConfigBuilder ;
   } ;

   class OmSdbConfTemplate : public OmConfTemplate
   {
      public:
         OmSdbConfTemplate() ;
         virtual ~OmSdbConfTemplate() ;

      public:
         INT32 getReplicaNum() const { return _replicaNum ; }
         INT32 getDataNum() const { return _dataNum ; }
         INT32 getDataGroupNum() const { return _dataGroupNum ; }
         INT32 getCatalogNum() const { return _catalogNum ; }
         INT32 getCoordNum() const { return _coordNum ; }
         void  setCoordNum( INT32 coordNum ) ;

      private:
         INT32 _init() ;
         void  _reset() ;
         bool  _isAllProperySet() ;
         INT32 _setPropery( const string& name, const string& value ) ;

      private:
         INT32 _replicaNum ;
         INT32 _dataNum ;
         INT32 _catalogNum ;
         INT32 _dataGroupNum ;
         INT32 _coordNum ;
   } ;

   class OmSdbConfProperties : public OmConfProperties
   {
      public:
         OmSdbConfProperties() ;
         virtual ~OmSdbConfProperties() ;

      public:
         bool           isPrivateProperty( const string& name ) const ;
         bool           isAllPropertySet() ;
   } ;

   class OmSdbConfigBuilder: public OmConfigBuilder
   {
   public:
      OmSdbConfigBuilder( const OmBusinessInfo& businessInfo ) ;
      virtual ~OmSdbConfigBuilder() ;

   private:
      OmConfTemplate& _getConfTemplate() { return _template ; }
      INT32 _build( BSONArray &nodeConfig ) ;
      INT32 _buildStandalone() ;
      INT32 _buildCluster() ;         
      INT32 _check( BSONObj& bsonConfig ) ;
      INT32 _checkAndAddNode( const BSONObj& bsonNode ) ;
      bool  _isServiceNameUsed( const OmHost& host, const string& serviceName ) ;

   private:
      OmSdbConfTemplate    _template ;

   protected:
      string               _localHostName ;
      string               _localServicePort ;
      string               _defaultServicePort ;
      OmSdbConfProperties  _properties ;

   protected:
      void  _setLocal() ;
      OmConfProperties& _getConfProperties() { return _properties ; }
      INT32 _getServiceName( const OmHost& host, const string& role,
                             string& serviceName ) ;
      INT32 _getDBPath( OmHost& host, const string& diskPath,
                        const string& businessType, const string& role,
                        const string& serviceName, string& dbPath ) ;
      INT32 _createNode( const string& role, const string& groupName ) ;
   } ;

   class OmSdbConfExtendTemplate : public OmConfTemplate
   {
      public:
         OmSdbConfExtendTemplate() ;
         virtual ~OmSdbConfExtendTemplate() ;

      public:
         INT32 getReplicaNum() const { return _replicaNum ; }
         INT32 getDataNum() const { return _dataNum ; }
         INT32 getDataGroupNum() const { return _dataGroupNum ; }
         INT32 getCatalogNum() const { return _catalogNum ; }
         INT32 getCoordNum() const { return _coordNum ; }

      private:
         INT32 _init() ;
         void  _reset() ;
         bool  _isAllProperySet() ;
         INT32 _setPropery( const string& name, const string& value ) ;

      private:
         INT32 _replicaNum ;
         INT32 _dataNum ;
         INT32 _catalogNum ;
         INT32 _dataGroupNum ;
         INT32 _coordNum ;
   } ;

   class OmExtendSdbConfigBuilder : public OmSdbConfigBuilder
   {
   public:
      OmExtendSdbConfigBuilder( const OmBusinessInfo& businessInfo ) ;
      virtual ~OmExtendSdbConfigBuilder() ;

   private:
         OmConfTemplate& _getConfTemplate() { return _template ; }
         INT32 _build( BSONArray& nodeConfig ) ;
         INT32 _buildHorizontal( map<string, INT32>& groupMap ) ;
         INT32 _buildVertical( INT32 existCoord, INT32 existCatalog,
                               map<string, INT32>& dataGroup ) ;
         INT32 _getGroupInfo( map<string, INT32>& groupMap,
                              INT32& coordNum,
                              INT32& catalogNum ) ;

   private:
      OmSdbConfExtendTemplate _template ;
   } ;

}

#endif /* OM_CONFIG_SDB_HPP_ */
