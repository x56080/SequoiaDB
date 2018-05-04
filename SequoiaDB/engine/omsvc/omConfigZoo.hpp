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

   Source File Name = omConfigZoo.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          22/4/2016  David Li Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef OM_CONFIG_ZOO_HPP_
#define OM_CONFIG_ZOO_HPP_ 

#include "omConfigBuilder.hpp"

namespace engine
{
   class OmZooConfigBuilder ;

   class OmZooNode: public OmNode
   {
   public:
      OmZooNode() ;
      virtual ~OmZooNode() ;

   public:
      const string& getInstallPath() const { return _installPath ; }
      const string& getZooId() const { return _zooId ; }
      const string& getDataPath() const { return _dataPath ; }
      const string& getDataPort() const { return _dataPort ; }
      const string& getElectPort() const { return _electPort ; }
      const string& getClientPort() const { return _clientPort ; }

   private:
      INT32 _init( const BSONObj& bsonNode, OmHost& host, OmCluster& cluster ) ;
      INT32 _setInstallPath( const string& path, OmHost& host ) ;
      void  _setZooId( const string& zooId ) ;
      INT32 _setDataPath( const string& path, OmHost& host ) ;
      INT32 _setDataPort( const string& port ) ;
      INT32 _setElectPort( const string& port ) ;
      INT32 _setClientPort( const string& port ) ;

   private:
      string _installPath ;
      string _zooId ;
      string _dataPath ;
      string _dataPort ;
      string _electPort ;
      string _clientPort ;

      friend class OmZooConfigBuilder ;
   } ;

   class OmZooConfTemplate : public OmConfTemplate
   {
      public:
         OmZooConfTemplate() ;
         virtual ~OmZooConfTemplate() ;

      public:
         INT32             getZooNum() const { return _zooNum ; }

      private:
         void              _reset() ;
         bool              _isAllProperySet() ;
         INT32             _setPropery( const string& name, const string& value ) ;

      private:
         INT32             _zooNum ;
   } ;

   class OmZooConfProperties : public OmConfProperties
   {
      public:
         OmZooConfProperties() ;
         virtual ~OmZooConfProperties() ;

      public:
         bool           isPrivateProperty( const string& name ) const ;
         bool           isAllPropertySet() ;
   } ;

   class OmZooConfigBuilder: public OmConfigBuilder
   {
   public:
      OmZooConfigBuilder( const OmBusinessInfo& businessInfo ) ;
      virtual ~OmZooConfigBuilder() ;

   private:
      OmConfTemplate& _getConfTemplate() { return _template ; }
      OmConfProperties& _getConfProperties() { return _properties ; }
      INT32 _build( BSONArray& nodeConfig ) ;
      INT32 _createNode() ;
      INT32 _getDataPort( const OmHost& host, string& dataPort ) ;
      bool  _isDataPortUsed( const OmHost& host, string& dataPort ) ;
      INT32 _getInstallPath( OmHost& host, const string& businessType,
                               const string& businessName, const string& dataPort,
                               string& installPath ) ;
      INT32 _getDataPath( OmHost& host, const string& diskPath,
                           const string& businessType, const string& businessName,
                           const string& dataPort, string& dataPath ) ;
      INT32 _check( BSONObj& bsonConfig ) ;
      INT32 _checkAndAddNode( const BSONObj& bsonNode ) ;

   private:
      OmZooConfTemplate    _template ;
      OmZooConfProperties  _properties ;
      INT32                _zooId ;
   } ;
}

#endif /* OM_CONFIG_ZOO_HPP_ */

