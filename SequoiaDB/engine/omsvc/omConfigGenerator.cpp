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

   Source File Name = omConfigGenerator.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          10/15/2014  LYB Initial Draft

   Last Changed =

*******************************************************************************/
#include "omConfigGenerator.hpp"
#include "omDef.hpp"
#include "pd.hpp"
#include "ossUtil.hpp"
#include "pmdOptions.hpp"
#include "pmdEDU.hpp"
#include "pmd.hpp"
#include "pmdOptionsMgr.hpp"

using namespace bson ;

namespace engine
{
   #define OM_GENERATOR_DOT               ","
   #define OM_GENERATOR_LINE              "-"


   #define OM_DG_NAME_PATTERN             "group"

   #define OM_DEPLOY_MOD_STANDALONE       "standalone"
   #define OM_DEPLOY_MOD_DISTRIBUTION     "distribution"

   #define OM_NODE_ROLE_STANDALONE        SDB_ROLE_STANDALONE_STR
   #define OM_NODE_ROLE_COORD             SDB_ROLE_COORD_STR
   #define OM_NODE_ROLE_CATALOG           SDB_ROLE_CATALOG_STR
   #define OM_NODE_ROLE_DATA              SDB_ROLE_DATA_STR

   #define OM_SVCNAME_STEP                (5)
   #define OM_PATH_LENGTH                 (256)
   #define OM_INT32_MAXVALUE_STR          "2147483647"

   #define OM_CONF_VALUE_INT_TYPE         "int"

   static string strPlus( const string &addend, INT32 augend ) ;
   static string strPlus( INT32 addend, INT32 augend ) ;
   static string strConnect( const string &left, INT32 right ) ;
   static string trimLeft( string &str, const string &trimer ) ;
   static string trimRight( string &str, const string &trimer ) ;
   static string trim( string &str ) ;
   static INT32 getValueAsString( const BSONObj &bsonTemplate, 
                                  const string &fieldName, string &value ) ;

   INT32 getValueAsString( const BSONObj &bsonTemplate, 
                           const string &fieldName, string &value )
   {
      INT32 rc = SDB_OK ;
      BSONElement element = bsonTemplate.getField( fieldName ) ;
      if ( element.eoo() )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      if ( String == element.type() )
      {
         value = element.String() ;
      }
      else if ( NumberInt == element.type() )
      {
         CHAR tmp[20] ;
         ossSnprintf( tmp, sizeof(tmp), "%d", element.Int() ) ;
         value = string( tmp ) ;
      }
      else if ( NumberLong == element.type() )
      {
         CHAR tmp[40] ;
         ossSnprintf( tmp, sizeof(tmp), OSS_LL_PRINT_FORMAT, element.Long() ) ;
         value = string( tmp ) ;
      }
      else
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   string strPlus( const string &addend, INT32 augend )
   {
      INT32 total = ossAtoi( addend.c_str() ) + augend ;
      CHAR result[ OM_INT32_LENGTH + 1 ] ;
      ossItoa( total, result, OM_INT32_LENGTH ) ;

      return string( result ) ;
   }

   string strPlus( INT32 addend, INT32 augend )
   {
      INT32 total = addend + augend ;
      CHAR result[ OM_INT32_LENGTH + 1 ] ;
      ossItoa( total, result, OM_INT32_LENGTH ) ;

      return string( result ) ;
   }

   string strConnect( const string &left, INT32 right )
   {
      CHAR result[ OM_INT32_LENGTH + 1 ] ;
      ossItoa( right, result, OM_INT32_LENGTH ) ;

      return ( left + result ) ;
   }

   string trimLeft( string &str, const string &trimer )
   {
      str.erase( 0, str.find_first_not_of( trimer ) ) ;
      return str ;
   }

   string trimRight( string &str, const string &trimer )
   {
      string::size_type pos = str.find_last_not_of( trimer ) ;
      if ( pos == string::npos )
      {
         str.erase( 0 ) ;
      }
      else
      {
         str.erase( pos + 1 ) ;
      }

      return str ;
   }

   string trim( string &str )
   {
      trimLeft( str, " " ) ;
      trimRight( str, " " ) ;

      return str ;
   }

   nodeCounter::nodeCounter()
   {
   }

   nodeCounter::~nodeCounter()
   {
   }

   void nodeCounter::increaseNode( const string &role )
   {
      map<string, UINT32>::iterator iter = _mapCounter.find( role ) ;
      if ( iter == _mapCounter.end() )
      {
         _mapCounter[ role ] = 0 ;
      }

      _mapCounter[ role ] += 1 ;
   }

   INT32 nodeCounter::getNodeCount( const string &role )
   {
      map<string, UINT32>::iterator iter = _mapCounter.find( role ) ;
      if ( iter != _mapCounter.end() )
      {
         return iter->second ;
      }

      return 0 ;
   }

   INT32 nodeCounter::getNodeCount()
   {
      map<string, UINT32>::iterator iter = _mapCounter.begin() ;
      INT32 total = 0 ;
      while ( iter != _mapCounter.end() )
      {
         total += iter->second ;
         iter++ ;
      }

      return total ;
   }

   businessNodeCounter::businessNodeCounter( const string &businessName )
                       :_businessName( businessName )
   {
   }

   businessNodeCounter::~businessNodeCounter()
   {
   }

   void businessNodeCounter::addNode( const string &role )
   {
      _counter.increaseNode( role ) ;
   }


   diskNodeCounter::diskNodeCounter( const string &diskName )
                   :_diskName(diskName)
   {

   }

   diskNodeCounter::~diskNodeCounter()
   {
      map<string, businessNodeCounter*>::iterator iter ;
      iter = _mapBusinessCounter.begin() ;
      while ( iter != _mapBusinessCounter.end() )
      {
         SDB_OSS_DEL iter->second ;
         _mapBusinessCounter.erase( iter++ ) ;
      }

      _mapBusinessCounter.clear() ;
   }

   INT32 diskNodeCounter::addNode( const string &businessName, 
                                   const string &role )
   {
      INT32 rc = SDB_OK ;

      map<string, businessNodeCounter*>::iterator iter ;
      iter = _mapBusinessCounter.find( businessName ) ;
      if ( iter == _mapBusinessCounter.end() )
      {
         businessNodeCounter *bnc = SDB_OSS_NEW businessNodeCounter( 
                                                                businessName ) ;
         if ( NULL == bnc )
         {
            rc = SDB_OOM ;
            PD_LOG_MSG( PDERROR, "out of memory" ) ;      
            goto error ;
         }

         _mapBusinessCounter.insert( 
               map<string, businessNodeCounter*>::value_type( businessName, 
                                                              bnc ) ) ;
      }

      _mapBusinessCounter[businessName]->addNode( role ) ;
      _counter.increaseNode( role ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 diskNodeCounter::getNodeCount()
   {
      return _counter.getNodeCount() ;
   }

   INT32 diskNodeCounter::getNodeCount( const string &role )
   {
      return _counter.getNodeCount( role ) ;
   }

   hostNodeCounter::hostNodeCounter( const string &hostName )
                   :_hostName( hostName )
   {
   }

   hostNodeCounter::~hostNodeCounter()
   {
      map<string, diskNodeCounter*>::iterator iter ;
      iter = _mapDiskCounter.begin() ;
      while ( iter != _mapDiskCounter.end() )
      {
         SDB_OSS_DEL iter->second ;
         _mapDiskCounter.erase( iter++ ) ;
      }

      _mapDiskCounter.clear() ;
   }

   INT32 hostNodeCounter::addNode( const string &diskName,
                                   const string &businessName, 
                                   const string &role )
   {
      INT32 rc = SDB_OK ;
      map<string, diskNodeCounter *>::iterator iter ;
      iter = _mapDiskCounter.find( diskName ) ;
      if ( iter == _mapDiskCounter.end() )
      {
         diskNodeCounter *dnc = SDB_OSS_NEW diskNodeCounter( diskName ) ;
         if ( NULL == dnc )
         {
            rc = SDB_OOM ;
            PD_LOG_MSG( PDERROR, "out of memory" ) ;      
            goto error ;
         }

         _mapDiskCounter.insert( 
               map<string, diskNodeCounter*>::value_type( diskName, dnc ) ) ;
      }

      _mapDiskCounter[diskName]->addNode( businessName, role ) ;
      _counter.increaseNode( role ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 hostNodeCounter::getNodeCount( const string &role )
   {
      return _counter.getNodeCount( role ) ;
   }

   INT32 hostNodeCounter::getNodeCount()
   {
      return _counter.getNodeCount() ;
   }

   INT32 hostNodeCounter::getNodeCountInDisk( const string &diskName )
   {  
      INT32 count = 0 ;
      map<string, diskNodeCounter *>::iterator iter ;
      iter = _mapDiskCounter.find( diskName ) ;
      if ( iter != _mapDiskCounter.end() )
      {
         diskNodeCounter *p = iter->second ;
         count = p->getNodeCount() ;
      }

      return count ;
   }

   INT32 hostNodeCounter::getNodeCountInDisk( const string &diskName, 
                                              const string &role )
   {
      INT32 count = 0 ;
      map<string, diskNodeCounter *>::iterator iter ;
      iter = _mapDiskCounter.find( diskName ) ;
      if ( iter != _mapDiskCounter.end() )
      {
         diskNodeCounter *p = iter->second ;
         count = p->getNodeCount( role ) ;
      }

      return count ;
   }

   clusterNodeCounter::clusterNodeCounter()
   {
   }

   clusterNodeCounter::~clusterNodeCounter()
   {
      clear() ;
   }

   void clusterNodeCounter::clear()
   {
      map<string, hostNodeCounter*>::iterator iter ;
      iter = _mapHostNodeCounter.begin() ;
      while ( iter != _mapHostNodeCounter.end() )
      {
         SDB_OSS_DEL iter->second ;
         _mapHostNodeCounter.erase( iter++ ) ;
      }

      _availableGroupIDMap.clear() ;
   }

   INT32 clusterNodeCounter::increaseGroupID( const string &businessName )
   {
      INT32 id = 1 ;
      map<string, INT32>::iterator iter ;
      iter = _availableGroupIDMap.find( businessName) ;
      if ( iter == _availableGroupIDMap.end() )
      {
         _availableGroupIDMap[ businessName ] = 1 ;
      }

      id = _availableGroupIDMap[ businessName ] ;
      _availableGroupIDMap[ businessName ]++ ;

      return id ;
   }

   INT32 clusterNodeCounter::getCountInHost( const string &hostName, 
                                             const string &role )
   {
      INT32 count = 0 ;
      map<string, hostNodeCounter *>::iterator iter ;
      iter = _mapHostNodeCounter.find( hostName ) ;
      if ( iter != _mapHostNodeCounter.end() )
      {
         hostNodeCounter *p = iter->second ;
         count = p->getNodeCount( role ) ;
      }

      return count ;
   }

   INT32 clusterNodeCounter::getCountInHost( const string &hostName )
   {
      INT32 count = 0 ;
      map<string, hostNodeCounter *>::iterator iter ;
      iter = _mapHostNodeCounter.find( hostName ) ;
      if ( iter != _mapHostNodeCounter.end() )
      {
         hostNodeCounter *p = iter->second ;
         count = p->getNodeCount() ;
      }

      return count ;
   }

   INT32 clusterNodeCounter::getCountInDisk( const string &hostName, 
                                             const string &diskName,
                                             const string &role )
   {
      INT32 count = 0 ;
      map<string, hostNodeCounter *>::iterator iter ;
      iter = _mapHostNodeCounter.find( hostName ) ;
      if ( iter != _mapHostNodeCounter.end() )
      {
         hostNodeCounter *p = iter->second ;
         count = p->getNodeCountInDisk( diskName, role ) ;
      }

      return count ;
   }

   INT32 clusterNodeCounter::getCountInDisk( const string &hostName, 
                                             const string &diskName )
   {
      INT32 count = 0 ;
      map<string, hostNodeCounter *>::iterator iter ;
      iter = _mapHostNodeCounter.find( hostName ) ;
      if ( iter != _mapHostNodeCounter.end() )
      {
         hostNodeCounter *p = iter->second ;
         count = p->getNodeCountInDisk( diskName ) ;
      }

      return count ;
   }

   INT32 clusterNodeCounter::addNode( const string &hostName,
                                      const string &diskName,
                                      const string &businessName, 
                                      const string &role, 
                                      const string &groupName )
   {
      INT32 rc = SDB_OK ;
      map<string, hostNodeCounter *>::iterator iter ;
      iter = _mapHostNodeCounter.find( hostName ) ;
      if ( iter == _mapHostNodeCounter.end() )
      {
         hostNodeCounter *hnc = SDB_OSS_NEW hostNodeCounter( hostName ) ;
         if ( NULL == hnc )
         {
            rc = SDB_OOM ;
            PD_LOG_MSG( PDERROR, "out of memory" ) ;      
            goto error ;
         }

         _mapHostNodeCounter.insert( 
               map<string, hostNodeCounter*>::value_type( hostName, hnc ) ) ;
      }

      _mapHostNodeCounter[hostName]->addNode( diskName, businessName, role ) ;
      _counter.increaseNode( role ) ;

      if ( role == OM_NODE_ROLE_DATA )
      {
         string::size_type pos = 0 ;
         pos = groupName.find( OM_DG_NAME_PATTERN ) ;
         if ( pos != string::npos )
         {
            string groupID ;
            INT32 id ;
            string::size_type start = pos + ossStrlen( OM_DG_NAME_PATTERN ) ;
            groupID = groupName.substr( start ) ;
            
            {
               map<string, INT32>::iterator iter ;
               iter = _availableGroupIDMap.find( businessName) ;
               if ( iter == _availableGroupIDMap.end() )
               {
                  _availableGroupIDMap[ businessName ] = 1 ;
               }
            }

            id = ossAtoi( groupID.c_str() ) ;
            if ( id  >= _availableGroupIDMap[ businessName ] )
            {
               _availableGroupIDMap[ businessName ] = id  + 1 ;
            }
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }


   hostHardWare::hostHardWare( const string &hostName, string startSvcName )
                :_hostName( hostName ), _availableSvcName( startSvcName )
   {
      CHAR local[ OSS_MAX_HOSTNAME + 1 ] = "" ;
      ossGetHostName( local, OSS_MAX_HOSTNAME ) ;
      if ( _hostName.compare( local ) == 0 )
      {
         pmdKRCB *pKrcb       = pmdGetKRCB() ;
         _pmdOptionsMgr *pOpt = pKrcb->getOptionCB() ;
         INT32 svcPort        = pOpt->getServicePort() ;

         INT32 iTmpSvcName = ossAtoi( _availableSvcName.c_str() ) ;
         if ( iTmpSvcName <= svcPort )
         {
            _availableSvcName = strPlus( svcPort, OM_SVCNAME_STEP ) ;
         }
         else if ( iTmpSvcName < svcPort + OM_SVCNAME_STEP )
         {
            _availableSvcName = strPlus( iTmpSvcName, OM_SVCNAME_STEP ) ;
         }
      }
   }

   hostHardWare::~hostHardWare()
   {
      _mapDisk.clear() ;
      _occupayPathSet.clear() ;
      _occupayPortSet.clear() ;
      _inUseDisk.clear() ;
   }

   INT32 hostHardWare::addDisk( const string &diskName, 
                                const string &mountPath, UINT64 totalSize, 
                                UINT64 freeSize )
   {
      simpleDiskInfo disk ;
      disk.diskName  = diskName ;
      disk.mountPath = mountPath ;
      trimLeft( disk.mountPath, " " ) ;
      trimRight( disk.mountPath, " " ) ;
      trimRight( disk.mountPath, OSS_FILE_SEP ) ;
      if ( disk.mountPath == "" )
      {
         disk.mountPath = OSS_FILE_SEP ;
      }
      disk.totalSize = totalSize ;
      disk.freeSize  = freeSize ;

      _mapDisk[ diskName ] = disk ;

      return SDB_OK ;
   }

   string hostHardWare::getName()
   {
      return _hostName ;
   }

   INT32 hostHardWare::getDiskCount()
   {
      return _mapDisk.size() ;
   }

   simpleDiskInfo *hostHardWare::_getDiskInfo( const string &dbPath )
   {
      INT32 maxFitSize          = 0 ;
      simpleDiskInfo *pDiskInfo = NULL ;
      map<string, simpleDiskInfo>::iterator iter = _mapDisk.begin() ;
      while ( iter != _mapDisk.end() )
      {
         simpleDiskInfo *pTmpDisk = &iter->second ;
         string::size_type pathPos = dbPath.find( pTmpDisk->mountPath ) ;
         if ( pathPos != string::npos )
         {
            INT32 tmpFitSize = pTmpDisk->mountPath.length() ;
            if ( NULL == pDiskInfo )
            {
               pDiskInfo  = pTmpDisk ;
               maxFitSize = tmpFitSize ;
            }
            else
            {
               if ( maxFitSize < tmpFitSize )
               {
                  pDiskInfo  = pTmpDisk ;
                  maxFitSize = tmpFitSize ;
               }
            }
         }

         iter++ ;
      }

      return pDiskInfo ;
   }

   string hostHardWare::getDiskName( const string &dbPath )
   {
      simpleDiskInfo *pDiskInfo = _getDiskInfo( dbPath ) ;
      if ( NULL != pDiskInfo )
      {
         return pDiskInfo->diskName ;
      }

      return "" ;
   }

   string hostHardWare::getMountPath( const string &dbPath )
   {
      simpleDiskInfo *pDiskInfo = _getDiskInfo( dbPath ) ;
      if ( NULL != pDiskInfo )
      {
         return pDiskInfo->mountPath ;
      }

      return "" ;
   }

   // get the disk count that no used before
   INT32 hostHardWare::getFreeDiskCount()
   {
      return _mapDisk.size() - _inUseDisk.size() ;
   }

   INT32 hostHardWare::occupayResource( const string &path, 
                                        list<string> &svcNameList,
                                        BOOLEAN checkPath )
   {
      INT32 rc = SDB_OK ;
      if ( checkPath )
      {
         string diskName = getDiskName( path ) ;
         if ( "" == diskName )
         {
            rc = SDB_DMS_RECORD_NOTEXIST ;
            PD_LOG_MSG( PDERROR, "path's disk is not exist:path=%s", 
                        path.c_str() ) ;
            goto error ;
         }

         _inUseDisk.insert( diskName ) ;
      }      

      {
         INT32 iMaxPort = 0 ;
         _occupayPathSet.insert( path ) ;
         list<string>::iterator iterList = svcNameList.begin() ;
         while ( iterList != svcNameList.end() )
         {
            string tmpPort = *iterList ;
            INT32 iTmpPort = ossAtoi( tmpPort.c_str() ) ;
            if ( iTmpPort > iMaxPort )
            {
               iMaxPort = iTmpPort ;
            }

            _occupayPortSet.insert( tmpPort ) ;
            iterList++ ;
         }

         INT32 iAvailable = ossAtoi( _availableSvcName.c_str() ) ;
         if ( iAvailable <= iMaxPort )
         {
            _availableSvcName = strPlus( iMaxPort, OM_SVCNAME_STEP ) ;
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   BOOLEAN hostHardWare::isDiskExist( const string &dbPath )
   {
      simpleDiskInfo *p = _getDiskInfo( dbPath ) ;
      return ( NULL != p ) ;
   }

   BOOLEAN hostHardWare::isPathOccupayed( const string &dbPath ) 
   {
      string tmpPath = dbPath ;
      trimLeft( tmpPath, " " ) ;
      trimRight( tmpPath, " " ) ;
      trimRight( tmpPath, OSS_FILE_SEP ) ;
      set<string>::iterator iter = _occupayPathSet.begin() ;
      while ( iter != _occupayPathSet.end() )
      {
         if ( tmpPath == *iter )
         {
            return TRUE ;
         }

         iter++ ;
      }

      return FALSE ;
   }

   BOOLEAN hostHardWare::isSvcNameOccupayed( const string &svcName ) 
   {
      string tmpName = svcName ;
      trimLeft( tmpName, " " ) ;
      trimRight( tmpName, " " ) ;
      set<string>::iterator iter = _occupayPortSet.begin() ;
      while ( iter != _occupayPortSet.end() )
      {
         if ( tmpName == *iter )
         {
            return TRUE ;
         }

         iter++ ;
      }

      return FALSE ;
   }

   string hostHardWare::getAvailableSvcName() 
   {
      return _availableSvcName ;
   }

   map<string, simpleDiskInfo> *hostHardWare::getDiskMap()
   {
      return &_mapDisk ;
   }

   omCluster::omCluster()
   {
   }

   omCluster::~omCluster()
   {
      clear() ;
   }

   void omCluster::setPropertyContainer( propertyContainer *pc )
   {
      _propertyContainer = pc ;
   }

   /*
   host:
   { 
      "HostName":"host1", "ClusterName":"c1", 
      "Disk":
      [
         {
            "Name":"/dev/sdb", Size:"", Mount:"/test", Used:""
         }, ...
      ]
   }
   config:
   {
      [
         { "BusinessName":"b2","dbpath":"", svcname:"", "role":"", ... }
         , ...
      ]
   }
   */
   INT32 omCluster::addHost( const BSONObj &host, const BSONObj &config )
   {
      INT32 rc = SDB_OK ;
      string hostName = host.getStringField( OM_BSON_FIELD_HOST_NAME ) ;
      map<string, hostHardWare*>::iterator iter = _mapHost.find( hostName ) ;
      SDB_ASSERT( iter == _mapHost.end(), "" ) ;
      string defaultSvcName = _propertyContainer->getDefaultValue(
                                                     OM_CONF_DETAIL_SVCNAME ) ;
      hostHardWare *pHostHW = SDB_OSS_NEW hostHardWare( hostName, 
                                                        defaultSvcName ) ;
      if ( NULL == pHostHW )
      {
         rc = SDB_OOM ;
         PD_LOG( PDERROR, "new hostHardWare failed:rc=%d", rc ) ;
         goto error ;
      }

      _mapHost.insert( map<string, hostHardWare*>::value_type( hostName, 
                                                               pHostHW ) ) ;

      {
         BSONObj disks = host.getObjectField( OM_BSON_FIELD_DISK ) ;
         BSONObjIterator i( disks ) ;
         while ( i.more() )
         {
            string tmp ;
            BSONElement ele = i.next() ;
            if ( Object == ele.type() )
            {
               BSONObj oneDisk = ele.embeddedObject() ;
               string diskName ;
               string mountPath ;
               diskName  = oneDisk.getStringField( OM_BSON_FIELD_DISK_NAME ) ;
               mountPath = oneDisk.getStringField( OM_BSON_FIELD_DISK_MOUNT ) ;
               pHostHW->addDisk( diskName, mountPath, 0, 0 ) ;
            }
         }
      }

      {
         BSONObj nodes = config.getObjectField( OM_BSON_FIELD_CONFIG ) ;
         BSONObjIterator i( nodes ) ;
         while ( i.more() )
         {
            string tmp ;
            BSONElement ele = i.next() ;
            if ( Object == ele.type() )
            {
               list<string> portList ;
               BSONObj oneNode = ele.embeddedObject() ;
               string businessType ;
               businessType = oneNode.getStringField( OM_BUSINESS_FIELD_TYPE ) ;
               if ( businessType == OM_BUSINESS_SEQUOIADB )
               {
                  string businessName ;
                  string dbPath ;
                  string role ;
                  string svcName ;
                  string groupName ;
                  string diskName ;
                  businessName = oneNode.getStringField( OM_BSON_BUSINESS_NAME ) ;
                  dbPath       = oneNode.getStringField( OM_CONF_DETAIL_DBPATH ) ;
                  role         = oneNode.getStringField( OM_CONF_DETAIL_ROLE ) ;
                  svcName      = oneNode.getStringField( OM_CONF_DETAIL_SVCNAME ) ;
                  groupName    = oneNode.getStringField( 
                                                   OM_CONF_DETAIL_DATAGROUPNAME ) ;
                  diskName     = pHostHW->getDiskName( dbPath ) ;
                  SDB_ASSERT( diskName != "" ,"" ) ;

                  portList.push_back( svcName ) ;
                  portList.push_back( strPlus( svcName, 1 ) ) ;
                  portList.push_back( strPlus( svcName, 2 ) ) ;
                  portList.push_back( strPlus( svcName, 3 ) ) ;
                  portList.push_back( strPlus( svcName, 4 ) ) ;
                  portList.push_back( strPlus( svcName, 5 ) ) ;
                  pHostHW->occupayResource( dbPath, portList ) ;
                  rc = _nodeCounter.addNode( hostName, diskName, businessName, 
                                             role, groupName ) ;
                  if ( SDB_OK != rc )
                  {
                     PD_LOG( PDERROR, "add node failed:rc=%d", rc ) ;
                     goto error ;
                  }
               }
               else if ( businessType == OM_BUSINESS_ZOOKEEPER )
               {
                  string businessName = oneNode.getStringField( 
                                             OM_BSON_BUSINESS_NAME ) ;
                  string installPath  = oneNode.getStringField( 
                                             OM_ZOO_CONF_DETAIL_INSTALLPATH ) ;
                  string zooID        = oneNode.getStringField( 
                                             OM_ZOO_CONF_DETAIL_ZOOID ) ;
                  string dataPath     = oneNode.getStringField( 
                                             OM_ZOO_CONF_DETAIL_DATAPATH ) ;
                  string dataPort     = oneNode.getStringField( 
                                             OM_ZOO_CONF_DETAIL_DATAPORT ) ;
                  string electPort    = oneNode.getStringField( 
                                             OM_ZOO_CONF_DETAIL_ELECTPORT) ;
                  string clientPort   = oneNode.getStringField( 
                                             OM_ZOO_CONF_DETAIL_CLIENTPORT ) ;
                  string diskName     = pHostHW->getDiskName( dataPath ) ;
                  SDB_ASSERT( diskName != "" ,"" ) ;

                  //empty list
                  pHostHW->occupayResource( installPath, portList ) ;

                  portList.push_back( dataPort ) ;
                  portList.push_back( electPort ) ;
                  portList.push_back( clientPort ) ;
                  pHostHW->occupayResource( dataPath, portList ) ;
               }
               else
               {
                  SDB_ASSERT( FALSE, businessType.c_str() ) ;
               }
            }
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 omCluster::_getBestResourceFromHost( hostHardWare *host, 
                                              const string &role, 
                                              simpleDiskInfo **diskInfo, 
                                              string &svcName )
   {
      INT32 rc                 = SDB_OK ;
      simpleDiskInfo *bestDisk = NULL ;
      string hostName          = host->getName() ;

      map<string, simpleDiskInfo> *mapDisk       = host->getDiskMap() ;
      map<string, simpleDiskInfo>::iterator iter = mapDisk->begin() ;
      while ( iter != mapDisk->end() )
      {
         if ( NULL == bestDisk )
         {
            //get the first disk
            bestDisk = &( iter->second ) ;
            iter++ ;
            continue ;
         }

         simpleDiskInfo *pTmp = &( iter->second ) ;
         INT32 bestRoleCount = _nodeCounter.getCountInDisk( hostName, 
                                                             bestDisk->diskName, 
                                                             role ) ;
         INT32 tmpRoleCount  = _nodeCounter.getCountInDisk( hostName,
                                                             pTmp->diskName ,
                                                             role ) ;
         if ( tmpRoleCount != bestRoleCount  )
         {
            //role count less, the better
            if ( tmpRoleCount < bestRoleCount )
            {
               bestDisk = &( iter->second ) ;
            }

            iter++;
            continue ;
         }

         INT32 bestCount = _nodeCounter.getCountInDisk( hostName, 
                                                         bestDisk->diskName ) ;
         INT32 tmpCount  = _nodeCounter.getCountInDisk( hostName,
                                                         pTmp->diskName ) ;
         if ( tmpCount != bestCount  )
         {
            //total count less, the better
            if ( tmpCount < bestCount )
            {
               bestDisk = &( iter->second ) ;
            }

            iter++;
            continue ;
         }

         iter++ ;
      }

      *diskInfo = bestDisk ;
      if ( NULL == *diskInfo )
      {
         rc = SDB_DMS_RECORD_NOTEXIST ;
         PD_LOG( PDERROR, "get disk failed:role=%s", role.c_str() ) ;
         goto error ;
      }

      svcName = host->getAvailableSvcName() ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 omCluster::createNode( const string &businessType, 
                                const string &businessName,
                                const string &role, 
                                const string &groupName, omNodeConf &node )
   {
      INT32 rc = SDB_OK ;
      list <string> portList ;
      INT32 pathAdjustIndex    = 0 ;
      simpleDiskInfo *diskInfo = NULL ;
      string hostName ;
      string diskName ;
      string svcName ;
      string dbPath ;
      _propertyContainer->createSample( node ) ;
      hostHardWare *host = _getBestHost( role ) ;
      if ( NULL == host )
      {
         rc = SDB_DMS_RECORD_NOTEXIST ;
         PD_LOG_MSG( PDERROR, 
                     "create node failed:host is zero or disk is zero" ) ;
         goto error ;
      }

      rc = _getBestResourceFromHost( host, role, &diskInfo, svcName ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "_getBestResourceFromHost failed:host=%s,role=%s", 
                 host->getName().c_str(), role.c_str() ) ;
         goto error ;
      }

      do 
      {
         CHAR tmpDbPath[OM_PATH_LENGTH + 1] = "";
         utilBuildFullPath( diskInfo->mountPath.c_str(), businessType.c_str(), 
                            OM_PATH_LENGTH, tmpDbPath ) ;
         utilCatPath( tmpDbPath, OM_PATH_LENGTH, OM_DBPATH_PREFIX_DATABASE ) ;
         utilCatPath( tmpDbPath, OM_PATH_LENGTH, role.c_str() ) ;
         if ( 0 == pathAdjustIndex )
         {
            utilCatPath( tmpDbPath, OM_PATH_LENGTH, svcName.c_str() ) ;
         }
         else
         {
            CHAR tmpSvcName[OM_PATH_LENGTH + 1] = "";
            ossSnprintf( tmpSvcName, OM_PATH_LENGTH, "%s_%d", svcName.c_str(), 
                         pathAdjustIndex ) ;
            utilCatPath( tmpDbPath, OM_PATH_LENGTH, tmpSvcName ) ;
         }
         pathAdjustIndex++ ;
         dbPath = tmpDbPath ;
      }while ( host->isPathOccupayed( dbPath ) ) ;

      hostName = host->getName() ;
      diskName = diskInfo->diskName ;

      portList.push_back( svcName ) ;
      portList.push_back( strPlus( svcName, 1 ) ) ;
      portList.push_back( strPlus( svcName, 2 ) ) ;
      portList.push_back( strPlus( svcName, 3 ) ) ;
      portList.push_back( strPlus( svcName, 4 ) ) ;
      portList.push_back( strPlus( svcName, 5 ) ) ;
      host->occupayResource( dbPath, portList ) ;
      rc = _nodeCounter.addNode( hostName, diskName, businessName, role, 
                                 groupName ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "addNode failed:rc=%d", rc ) ;
         goto error ;
      }

      node.setDataGroupName( groupName ) ;
      node.setDbPath( dbPath ) ;
      node.setRole( role ) ;
      node.setSvcName( svcName ) ;
      node.setDiskName( diskName ) ;
      node.setHostName( hostName ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 omCluster::getHostNum()
   {
      return _mapHost.size() ;
   }

   INT32 omCluster::increaseGroupID( const string &businessName )
   {
      return _nodeCounter.increaseGroupID( businessName ) ;
   }

   /*
      get best host rule:
          rule1: the less the better which host contains specify role's count
          rule2: the more the better which host contains unused disk's count
          rule3: the less the better which host contains node's count
                 ( all the roles )
   */
   hostHardWare* omCluster::_getBestHost( const string &role )
   {
      map<string, hostHardWare*>::iterator iter ;
      hostHardWare *bestHost = NULL ;
      if ( _mapHost.size() == 0 )
      {
         PD_LOG( PDERROR, "host count is zero" ) ;
         goto error ;
      }

      iter = _mapHost.begin() ;
      while ( iter != _mapHost.end() )
      {
         hostHardWare *pTmpHost = iter->second ;

         //ignore the host without disk
         if ( 0 == pTmpHost->getDiskCount() )
         {
            iter++ ;
            continue ;
         }

         //this is the first one. continue
         if ( NULL == bestHost )
         {
            bestHost = pTmpHost ;
            iter++ ;
            continue ;
         }

         INT32 tmpRoleCount ;
         tmpRoleCount = _nodeCounter.getCountInHost( pTmpHost->getName(), 
                                                     role ) ;

         INT32 bestRoleCount ;
         bestRoleCount = _nodeCounter.getCountInHost( bestHost->getName(), 
                                                      role ) ;
         if ( tmpRoleCount != bestRoleCount )
         {
            if ( tmpRoleCount < bestRoleCount )
            {
               // rule1
               bestHost = pTmpHost ;
            }

            iter++ ;
            continue ;
         }

         INT32 tmpFreeDiskCount  = pTmpHost->getFreeDiskCount();
         INT32 bestFreeDiskCount = bestHost->getFreeDiskCount() ;
         if ( tmpFreeDiskCount != bestFreeDiskCount )
         {
            if ( tmpFreeDiskCount > bestFreeDiskCount )
            {
               // rule2
               bestHost = pTmpHost ;
            }

            iter++ ;
            continue ;
         }

         INT32 tmpNodeCount  = _nodeCounter.getCountInHost( 
                                                         pTmpHost->getName() ) ;
         INT32 bestNodeCount = _nodeCounter.getCountInHost( 
                                                         bestHost->getName() ) ;
         if ( tmpNodeCount != bestNodeCount )
         {
            if ( tmpNodeCount < bestNodeCount )
            {
               // rule3
               bestHost = pTmpHost ;
            }

            iter++ ;
            continue ;
         }

         iter++ ;
      }

   done:
      return bestHost ;
   error:
      goto done ;
   }

   INT32 omCluster::checkAndAddNode( const string &businessName,
                                     omNodeConf *node )
   {
      INT32 rc         = SDB_OK ;
      string role      = node->getRole() ;
      string svcName   = node->getSvcName() ;
      string dbPath    = node->getDbPath() ;
      string hostName  = node->getHostName() ;
      string groupName = node->getDataGroupName() ;

      map<string, hostHardWare*>::iterator iter = _mapHost.find( hostName ) ;
      if ( iter == _mapHost.end() )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "host is not exist:hostName=%s", 
                     hostName.c_str() ) ;
         goto error ;
      }

      {
         string diskName ;
         list <string> portList ;
         hostHardWare *hw = iter->second ;
         if ( !hw->isDiskExist( dbPath ) )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG_MSG( PDERROR, "dbPath's disk is not exist:hostName=%s,"
                        "dbPath=%s", hostName.c_str(), dbPath.c_str() ) ;
            goto error ;
         }

         if ( hw->isPathOccupayed( dbPath ) )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG_MSG( PDERROR, "dbpath is exist:dbpath=%s", dbPath.c_str() ) ;
            goto error ;
         }
         /*TODO: we must check this also
                 strPlus( svcName, 1 ) ; strPlus( svcName, 2 ) ;
                 strPlus( svcName, 3 ) ; strPlus( svcName, 4 ) ;
                 strPlus( svcName, 5 ) ;
         */
         if ( hw->isSvcNameOccupayed( svcName ) )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG_MSG( PDERROR, "svcname is exist:svcname=%s", 
                        svcName.c_str() ) ;
            goto error ;
         }

         portList.push_back( svcName ) ;
         portList.push_back( strPlus( svcName, 1 ) ) ;
         portList.push_back( strPlus( svcName, 2 ) ) ;
         portList.push_back( strPlus( svcName, 3 ) ) ;
         portList.push_back( strPlus( svcName, 4 ) ) ;
         portList.push_back( strPlus( svcName, 5 ) ) ;
         hw->occupayResource( dbPath, portList ) ;
         diskName = hw->getDiskName( dbPath ) ;

         _nodeCounter.addNode( hostName, diskName, businessName, role, 
                               groupName ) ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   void omCluster::clear()
   {
      map<string, hostHardWare*>::iterator iter = _mapHost.begin() ;
      while ( iter != _mapHost.end() )
      {
         hostHardWare *pHost = iter->second ;
         SDB_OSS_DEL pHost ;
         _mapHost.erase( iter++ ) ;
      }

      _propertyContainer = NULL ;
      _nodeCounter.clear() ;
   }

   INT32 omConfTemplate::init( const BSONObj &confTemplate )
   {
      INT32 rc = SDB_OK ;
      rc = getValueAsString( confTemplate, OM_BSON_BUSINESS_TYPE, 
                             _businessType ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG_MSG( PDERROR, "Template miss bson field[%s]", 
                     OM_BSON_BUSINESS_TYPE ) ;
         goto error ;
      }

      rc = getValueAsString( confTemplate, OM_BSON_BUSINESS_NAME, 
                             _businessName ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG_MSG( PDERROR, "Template miss bson field[%s]", 
                     OM_BSON_BUSINESS_NAME ) ;
         goto error ;
      }

      rc = getValueAsString( confTemplate, OM_BSON_FIELD_CLUSTER_NAME, 
                             _clusterName ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG_MSG( PDERROR, "Template miss bson field[%s]", 
                     OM_BSON_FIELD_CLUSTER_NAME ) ;
         goto error ;
      }

      rc = getValueAsString( confTemplate, OM_BSON_DEPLOY_MOD, _deployMod ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG_MSG( PDERROR, "Template miss bson field[%s]", 
                     OM_BSON_DEPLOY_MOD ) ;
         goto error ;
      }

      {
         BSONElement propertyElement ;
         propertyElement = confTemplate.getField( OM_BSON_PROPERTY_ARRAY ) ;
         if ( propertyElement.eoo() || Array != propertyElement.type() )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG_MSG( PDERROR, "template's field is not Array:field=%s,"
                        "type=%d", OM_BSON_PROPERTY_ARRAY, 
                        propertyElement.type() ) ;
            goto error ;
         }

         BSONObjIterator i( propertyElement.embeddedObject() ) ;
         while ( i.more() )
         {
            BSONElement ele = i.next() ;
            if ( ele.type() == Object )
            {
               BSONObj oneProperty = ele.embeddedObject() ;
               rc = _setPropery( oneProperty ) ;
               if ( SDB_OK != rc )
               {
                  PD_LOG( PDERROR, "_setPropery failed:rc=%d", rc ) ;
                  goto error ;
               }
            }
         }
      }

      if ( !_isAllProperySet() )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "miss template configur item" ) ;
         goto error ;
      }

      rc = _init() ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "_init failed:rc=%d", rc ) ;
         goto error ;
      }

      done:
         return rc ;
      error:
         goto done ;
   }

   void omConfTemplate::reset()
   {
      _businessType = "" ;
      _businessName = "" ;
      _clusterName  = "" ;
      _deployMod    = "" ;
      _reset() ;
   }

   INT32 omConfTemplate::_init()
   {
      return SDB_OK ;
   }

   void omConfTemplate::_reset()
   {
      return ;
   }

   BOOLEAN omConfTemplate::_isAllProperySet()
   {
      return TRUE ;
   }

   INT32 omConfTemplate::_setPropery( BSONObj &property )
   {
      return SDB_OK ;
   }

   omSdbConfTemplate::omSdbConfTemplate()
                  :_replicaNum( -1 ), _dataNum( 0 ), 
                   _catalogNum( -1 ), _dataGroupNum( -1 ), _coordNum( -1 )
   {
   }

   omSdbConfTemplate::~omSdbConfTemplate()
   {
      reset() ;
   }

   INT32 omSdbConfTemplate::_init()
   {
      _dataNum = _dataGroupNum * _replicaNum ;
      return SDB_OK ;
   }

   /*
   bsonTemplate:
   {
      "ClusterName":"c1","BusinessType":"sequoiadb", "BusinessName":"b1",
      "DeployMod": "standalone", 
      "Property":[{"Name":"replicanum", "Type":"int", "Default":"1", 
                      "Valid":"1", "Display":"edit box", "Edit":"false", 
                      "Desc":"", "WebName":"" }
                      , ...
                 ] 
   }
   */
   INT32 omSdbConfTemplate::_setPropery( BSONObj &property )
   {
      INT32 rc = SDB_OK ;
      string itemName ;
      string itemValue ;
      rc = getValueAsString( property, OM_BSON_PROPERTY_NAME, itemName ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG_MSG( PDERROR, "property miss bson field=%s", 
                     OM_BSON_PROPERTY_NAME ) ;
         goto error ;
      }

      rc = getValueAsString( property, OM_BSON_PROPERTY_VALUE, 
                             itemValue ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG_MSG( PDERROR, "property miss bson field=%s", 
                     OM_BSON_PROPERTY_VALUE ) ;
         goto error ;
      }

      if ( itemName.compare( OM_TEMPLATE_REPLICA_NUM ) == 0 )
      {
         _replicaNum = ossAtoi( itemValue.c_str() ) ;
      }
      else if ( itemName.compare( OM_TEMPLATE_DATAGROUP_NUM ) == 0 )
      {
         _dataGroupNum = ossAtoi( itemValue.c_str() ) ;
      }
      else if ( itemName.compare( OM_TEMPLATE_CATALOG_NUM ) == 0 )
      {
         _catalogNum = ossAtoi( itemValue.c_str() ) ;
      }
      else if ( itemName.compare( OM_TEMPLATE_COORD_NUM ) == 0 )
      {
         _coordNum = ossAtoi( itemValue.c_str() ) ;
      }

      {
         confProperty oneProperty ;
         rc = oneProperty.init( property ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "init property failed:rc=%d", rc ) ;
            goto error ;
         }

         if ( !oneProperty.isValid( itemValue ) )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG_MSG( PDERROR, "Template value is invalid:item=%s,value=%s,"
                        "valid=%s", itemName.c_str(), itemValue.c_str(), 
                        oneProperty.getValidString().c_str() ) ;
            goto error ;
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   void omSdbConfTemplate::_reset()
   {
      _replicaNum   = -1 ;
      _dataNum      = -1 ;
      _dataGroupNum = -1 ;
      _catalogNum   = -1 ;
      _coordNum     = -1 ;
   }

   BOOLEAN omSdbConfTemplate::_isAllProperySet()
   {
      if ( _replicaNum == -1 )
      {
         PD_LOG_MSG( PDERROR, "%s have not been set", 
                     OM_TEMPLATE_REPLICA_NUM ) ;
         return FALSE ;
      }
      else if ( _dataGroupNum == -1 )
      {
         PD_LOG_MSG( PDERROR, "%s have not been set", 
                     OM_TEMPLATE_DATAGROUP_NUM ) ;
         return FALSE ;
      }
      else if ( _catalogNum == -1 )
      {
         PD_LOG_MSG( PDERROR, "%s have not been set", 
                     OM_TEMPLATE_CATALOG_NUM ) ;
         return FALSE ;
      }
      else if ( _coordNum == -1 )
      {
         PD_LOG_MSG( PDERROR, "%s have not been set", OM_TEMPLATE_COORD_NUM ) ;
         return FALSE ;
      }

      return TRUE ;
   }

   INT32 omSdbConfTemplate::getReplicaNum()
   {
      return _replicaNum ;
   }

   INT32 omSdbConfTemplate::getDataNum()
   {
      return _dataNum ;
   }

   INT32 omSdbConfTemplate::getDataGroupNum()
   {
      return _dataGroupNum ;
   }

   INT32 omSdbConfTemplate::getCatalogNum()
   {
      return _catalogNum ;
   }

   INT32 omSdbConfTemplate::getCoordNum()
   {
      return _coordNum ;
   }

   void omSdbConfTemplate::setCoordNum( INT32 coordNum )
   {
      _coordNum = coordNum ;
   }

   omNodeConf::omNodeConf()
   {
      _dbPath        = "" ;
      _svcName       = "" ;
      _role          = "" ;
      _dataGroupName = "" ;
      _hostName      = "" ;
      _diskName      = "" ;

      _additionalConfMap.clear() ;
   }

   omNodeConf::omNodeConf( const omNodeConf &right )
   {
      _dbPath        = right._dbPath ;
      _svcName       = right._svcName ;
      _role          = right._role ;
      _dataGroupName = right._dataGroupName ;
      _hostName      = right._hostName ;
      _diskName      = right._diskName ;

      _additionalConfMap = right._additionalConfMap ;
   }

   omNodeConf::~omNodeConf()
   {
      _additionalConfMap.clear() ;
   }

   void omNodeConf::setDbPath( const string &dbpath )
   {
      _dbPath = dbpath ;
   }

   void omNodeConf::setSvcName( const string &svcName )
   {
      _svcName = svcName ;
   }

   void omNodeConf::setRole( const string &role )
   {
      _role = role ;
   }

   void omNodeConf::setDataGroupName( const string &dataGroupName )
   {
      _dataGroupName = dataGroupName ;
   }

   void omNodeConf::setHostName( const string &hostName )
   {
      _hostName = hostName ;
   }

   void omNodeConf::setDiskName( const string &diskName )
   {
      _diskName = diskName ;
   }

   void omNodeConf::setAdditionalConf( const string &key, 
                                       const string &value )
   {
      _additionalConfMap[key] = value ;
   }

   string omNodeConf::getDbPath()
   {
      return _dbPath ;
   }

   string omNodeConf::getSvcName()
   {
      return _svcName ;
   }

   string omNodeConf::getRole()
   {
      return _role ;
   }

   string omNodeConf::getDataGroupName()
   {
      return _dataGroupName ;
   }

   string omNodeConf::getHostName()
   {
      return _hostName ;
   }

   string omNodeConf::getDiskName()
   {
      return _diskName ;
   }

   string omNodeConf::getAdditionlConf( const string &key )
   {
      string value = "" ;
      map<string, string>::iterator iter = _additionalConfMap.find( key ) ;
      if ( iter != _additionalConfMap.end() )
      {
         value = iter->second ;
      }

      return value ;
   }

   const map<string, string>* omNodeConf::getAdditionalMap()
   {
      return &_additionalConfMap ;
   }

   rangeValidator::rangeValidator( const string &type, const CHAR *value )
   {
      _isClosed   = TRUE ;
      _begin      = value ;
      _end        = value ;
      _isValidAll = FALSE ;
      _type       = type ;

      trim( _begin ) ;
      trim( _end ) ;

      if ( ( _begin.length() == 0 ) && ( _end.length() == 0 ) )
      {
         /* if range is empty, all the value is valid */
         _isValidAll = TRUE ;
      }
   }

   rangeValidator::rangeValidator( const string &type, const CHAR *begin, 
                                   const CHAR *end, BOOLEAN isClosed )
   {
      _isClosed   = isClosed ;
      _begin      = begin ;
      _end        = end ;
      _isValidAll = FALSE ;
      _type       = type ;

      trim( _begin ) ;
      trim( _end ) ;

      if ( ( _begin.length() == 0 ) && ( _end.length() == 0 ) )
      {
         /* if range is empty, all the value is valid */
         _isValidAll = TRUE ;
      }

      if ( _type.compare( OM_CONF_VALUE_INT_TYPE ) == 0 )
      {
         if ( _end.length() == 0 )
         {
            _end = OM_INT32_MAXVALUE_STR ;
         }
      }
   }

   rangeValidator::~rangeValidator()
   {
   }

   BOOLEAN rangeValidator::_isPureNumber( const char *value )
   {
      INT32 dotCount = 0 ;
      while ( NULL != value && *value != '\0' )
      {
         if ( *value >= '0' && *value <= '9' )
         {
            value++ ;
            continue ;
         }
         else if ( *value == '.' )
         {
            dotCount++ ;
            if ( dotCount <= 1 )
            {
               value++ ;
               continue ;
            }
         }

         return FALSE ;
      }

      return TRUE ;
   }

   BOOLEAN rangeValidator::_isNumber( const char *value )
   {
      if ( *value == '+' || *value == '-' ) 
      {
         value++ ;
      }

      return _isPureNumber( value ) ;
   }

   BOOLEAN rangeValidator::isValid( const string &value )
   {
      if ( _type == OM_CONF_VALUE_INT_TYPE )
      {
         if ( !_isNumber( value.c_str() ) )
         {
            return FALSE ;
         }
      }

      if ( _isValidAll )
      {
         return TRUE ;
      }

      INT32 compareEnd = _compare( value, _end ) ;
      if ( _isClosed )
      {
         if ( compareEnd == 0 )
         {
            return TRUE ;
         }
      }

      INT32 compareBegin = _compare( value, _begin ) ;
      if ( compareBegin >= 0 && compareEnd < 0 )
      {
         return TRUE ;
      }

      return FALSE ;
   }

   INT32 rangeValidator::_compare( string left, string right )
   {
      if ( _type == OM_CONF_VALUE_INT_TYPE )
      {
         INT32 leftInt  = ossAtoi( left.c_str() ) ;
         INT32 rightInt = ossAtoi( right.c_str() ) ;

         return ( leftInt - rightInt ) ;
      }

      return left.compare( right ) ;
   }

   string rangeValidator::getType() 
   {
      return _type ;
   }

   confValidator::confValidator()
   {
   }

   confValidator::~confValidator()
   {
      _clear() ;
   }

   rangeValidator *confValidator::_createrangeValidator( const string &value )
   {
      rangeValidator *rv       = NULL ;
      string::size_type posTmp = value.find( OM_GENERATOR_LINE ) ;
      if( string::npos != posTmp )
      {
         rv = SDB_OSS_NEW rangeValidator( _type,
                                          value.substr(0,posTmp).c_str(), 
                                          value.substr(posTmp+1).c_str() ) ;
      }
      else
      {
         rv = SDB_OSS_NEW rangeValidator( _type, value.c_str() ) ;
      }

      return rv ;
   }

   INT32 confValidator::init( const string &type, const string &validateStr )
   {
      _clear() ;

      string tmp ;
      INT32 rc = SDB_OK ;
      _type    = type ;

      rangeValidator *rv     = NULL ;
      string::size_type pos1 = 0 ;
      string::size_type pos2 = validateStr.find( OM_GENERATOR_DOT ) ;
      while( string::npos != pos2 )
      {
         rv = _createrangeValidator( validateStr.substr( pos1, pos2 - pos1 ) ) ;
         if ( NULL == rv )
         {
            rc = SDB_OOM ;
            goto error ;
         }

         _validatorList.push_back( rv ) ;

         pos1 = pos2 + 1 ;
         pos2 = validateStr.find( OM_GENERATOR_DOT, pos1 ) ;
      }

      tmp = validateStr.substr( pos1 ) ; 
      rv  = _createrangeValidator( tmp ) ;
      if ( NULL == rv )
      {
         rc = SDB_OOM ;
         goto error ;
      }

      _validatorList.push_back( rv ) ;

   done:
      return rc ;
   error:
      _clear() ;
      goto done ;
   }

   BOOLEAN confValidator::isValid( const string &value )
   {
      VALIDATORLIST_ITER iter = _validatorList.begin() ;
      while ( iter != _validatorList.end() )
      {
         if ( ( *iter )->isValid( value ) )
         {
            return TRUE ;
         }

         iter++ ;
      }

      return FALSE ;
   }

   string confValidator::getType()
   {
      return _type ;
   }

   void confValidator::_clear()
   {
      VALIDATORLIST_ITER iter = _validatorList.begin() ;
      while ( iter != _validatorList.end() )
      {
         rangeValidator *p = *iter ;
         SDB_OSS_DEL p ;
         iter++ ;
      }

      _validatorList.clear() ;
   }

   confProperty::confProperty()
   {
   }

   confProperty::~confProperty()
   {
   }

   /*
   propery:
      {
         "Name":"replicanum", "Type":"int", "Default":"1", "Valid":"1", 
         "Display":"edit box", "Edit":"false", "Desc":"", "WebName":"" 
      }
   */
   INT32 confProperty::init( const BSONObj &property )
   {
      INT32 rc = SDB_OK ;
      rc = getValueAsString( property, OM_BSON_PROPERTY_TYPE, _type ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG_MSG( PDERROR, "get field failed:field=%s,rc=%d", 
                     OM_BSON_PROPERTY_TYPE, rc ) ;
         goto error ;
      }

      rc = getValueAsString( property, OM_BSON_PROPERTY_NAME, _name ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG_MSG( PDERROR, "get field failed:field=%s,rc=%d", 
                     OM_BSON_PROPERTY_NAME, rc ) ;
         goto error ;
      }

      rc = getValueAsString( property, OM_BSON_PROPERTY_DEFAULT, 
                             _defaultValue ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG_MSG( PDERROR, "get field failed:field=%s,rc=%d", 
                     OM_BSON_PROPERTY_DEFAULT, rc ) ;
         goto error ;
      }

      rc = getValueAsString( property, OM_BSON_PROPERTY_VALID, _validateStr ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG_MSG( PDERROR, "get field failed:field=%s,rc=%d", 
                     OM_BSON_PROPERTY_VALID, rc ) ;
         goto error ;
      }

      rc = _confValidator.init( _type, _validateStr ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG_MSG( PDERROR, "init _confValidator failed:rc=%d", rc ) ;
         goto error ;
      }

      if ( !_confValidator.isValid( _defaultValue ) )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "%s's default value is invalid:value=%s,valid=%s", 
                     _name.c_str(), _defaultValue.c_str(), 
                     _validateStr.c_str() ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   string confProperty::getDefaultValue()
   {
      return _defaultValue ;
   }

   string confProperty::getItemName()
   {
      return _name ;
   }

   string confProperty::getType()
   {
      return _type ;
   }

   BOOLEAN confProperty::isValid( const string &value )
   {
      return _confValidator.isValid( value ) ;
   }

   string confProperty::getValidString()
   {
      return _validateStr ;
   }

   propertyContainer::propertyContainer()
                     :_dbPathProperty( NULL ), _roleProperty( NULL ), 
                     _svcNameProperty( NULL )
   {
   }

   propertyContainer::~propertyContainer()
   {
      clear() ;
   }

   void propertyContainer::clear()
   {
      map<string, confProperty*>::iterator iter ;
      iter = _additionalPropertyMap.begin() ;
      while ( iter != _additionalPropertyMap.end() )
      {
         confProperty *property = iter->second ;
         SDB_OSS_DEL property ;
         _additionalPropertyMap.erase( iter++ ) ;
      }

      if ( NULL != _dbPathProperty )
      {
         SDB_OSS_DEL _dbPathProperty ;
         _dbPathProperty = NULL ;
      }

      if ( NULL != _roleProperty )
      {
         SDB_OSS_DEL _roleProperty ;
         _roleProperty = NULL ;
      }

      if ( NULL != _svcNameProperty )
      {
         SDB_OSS_DEL _svcNameProperty ;
         _svcNameProperty = NULL ;
      }
   }

   INT32 propertyContainer::addProperty( const BSONObj &property )
   {
      INT32 rc = SDB_OK ;
      confProperty *pConfProperty = SDB_OSS_NEW confProperty() ;
      if ( NULL == pConfProperty )
      {
         rc = SDB_OOM ;
         PD_LOG_MSG( PDERROR, "new confProperty failed:rc=%d", rc ) ;
         goto error ;
      }

      rc = pConfProperty->init( property ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "init confProperty failed:property=%s,rc=%d", 
                 property.toString(false, true ).c_str(), rc ) ;
         goto error ;
      }

      if ( OM_CONF_DETAIL_DBPATH == pConfProperty->getItemName() )
      {
         SDB_ASSERT( NULL == _dbPathProperty, "" ) ;
         _dbPathProperty = pConfProperty ;
      }
      else if ( OM_CONF_DETAIL_ROLE == pConfProperty->getItemName() )
      {
         SDB_ASSERT( NULL == _roleProperty, "" ) ;
         _roleProperty = pConfProperty ;
      }
      else if ( OM_CONF_DETAIL_SVCNAME == pConfProperty->getItemName() )
      {
         SDB_ASSERT( NULL == _svcNameProperty, "" ) ;
         _svcNameProperty = pConfProperty ;
      }
      else
      {
         map<string, confProperty*>::iterator iter ;
         iter = _additionalPropertyMap.find( pConfProperty->getItemName() ) ;
         if ( iter != _additionalPropertyMap.end() )
         {
            confProperty *pTmp = iter->second ;
            SDB_OSS_DEL pTmp ;
            _additionalPropertyMap.erase( iter ) ;
         }

         _additionalPropertyMap.insert( map<string, confProperty*>::value_type( 
                                                   pConfProperty->getItemName(),
                                                   pConfProperty ) ) ;
      }

   done:
      return rc ;
   error:
      SAFE_OSS_DELETE( pConfProperty ) ;
      goto done ;
   }

   INT32 propertyContainer::checkValue( const string &name, 
                                        const string &value )
   {
      INT32 rc = SDB_OK ;
      confProperty *pProperty = _getConfProperty( name ) ;
      if ( NULL == pProperty )
      {
         rc = SDB_DMS_RECORD_NOTEXIST ;
         PD_LOG_MSG( PDERROR, "can't find the property:name=%s,rc=%d", 
                     name.c_str(), rc ) ;
         goto error ;
      }

      if ( !pProperty->isValid( value ) )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "property's value is invalid:name=%s,value=%s,"
                     "valid=%s", name.c_str(), value.c_str(), 
                     pProperty->getValidString().c_str() ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   confProperty *propertyContainer::_getConfProperty( const string &name )
   {
      confProperty *property = NULL ;
      if ( OM_CONF_DETAIL_DBPATH == name )
      {
         property = _dbPathProperty ;
      }
      else if ( OM_CONF_DETAIL_ROLE == name )
      {
         property = _roleProperty ;
      }
      else if ( OM_CONF_DETAIL_SVCNAME == name )
      {
         property = _svcNameProperty ;
      }
      else
      {
         map<string, confProperty*>::iterator iter ;
         iter = _additionalPropertyMap.find( name ) ;
         if ( iter != _additionalPropertyMap.end() )
         {
            property = iter->second ;
         }
      }

      return property ;
   }

   INT32 propertyContainer::createSample( omNodeConf &sample )
   {
      confProperty *property = NULL ;
      map<string, confProperty*>::iterator iter ;
      iter = _additionalPropertyMap.begin() ;
      while ( iter != _additionalPropertyMap.end() )
      {
         property = iter->second ;
         sample.setAdditionalConf( property->getItemName(), 
                                   property->getDefaultValue() ) ;
         iter++ ;
      }

      return SDB_OK ;
   }

   string propertyContainer::getDefaultValue( const string &name )
   {
      confProperty *property = _getConfProperty( name ) ;
      if ( NULL != property )
      {
         return property->getDefaultValue() ;
      }

      return "" ;
   }

   BOOLEAN propertyContainer::isAllPropertySet()
   {
      if ( NULL == _dbPathProperty )
      {
         PD_LOG_MSG( PDERROR, "property [%s] have not been set", 
                     OM_CONF_DETAIL_DBPATH ) ;
         return FALSE ;
      }

      if ( NULL == _roleProperty )
      {
         PD_LOG_MSG( PDERROR, "property [%s] have not been set", 
                     OM_CONF_DETAIL_ROLE ) ;
         return FALSE ;
      }

      if ( NULL == _svcNameProperty )
      {
         PD_LOG_MSG( PDERROR, "property [%s] have not been set", 
                     OM_CONF_DETAIL_SVCNAME ) ;
         return FALSE ;
      }

      return TRUE ;
   }

   omBusinessConfigure::omBusinessConfigure()
   {
   }

   omBusinessConfigure::~omBusinessConfigure()
   {
      clear() ;
   }

   INT32 omBusinessConfigure::_setNodeConf( BSONObj &oneNode, 
                                            omNodeConf &nodeConf )
   {
      INT32 rc = SDB_OK ;
      BSONObjIterator itemIter( oneNode ) ;
      while ( itemIter.more() )
      {
         BSONElement itemEle = itemIter.next() ;
         string fieldName    = itemEle.fieldName() ;
         string value        = itemEle.String() ;
         if ( OM_BSON_FIELD_HOST_NAME == fieldName )
         {
            nodeConf.setHostName( value ) ;
         }
         else if ( OM_CONF_DETAIL_DATAGROUPNAME == fieldName )
         {
            nodeConf.setDataGroupName( value ) ;
         }
         else 
         {
            rc = _propertyContainer->checkValue( fieldName, value ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "check value failed:name=%s,value=%s", 
                       fieldName.c_str(), value.c_str() ) ;
               goto error ;
            }

            if ( OM_CONF_DETAIL_DBPATH == fieldName )
            {
               nodeConf.setDbPath( value ) ;
            }
            else if ( OM_CONF_DETAIL_SVCNAME == fieldName )
            {
               nodeConf.setSvcName( value ) ;
            }
            else if ( OM_CONF_DETAIL_ROLE == fieldName )
            {
               nodeConf.setRole( value ) ;
            }
            else 
            {
               nodeConf.setAdditionalConf( fieldName, value ) ;
            }
         }
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   string omBusinessConfigure::getBusinessName()
   {
      return _businessName ;
   }

   /*
   business:
   {
      "BusinessType":"sequoiadb", "BusinessName":"b1", "DeployMod":"xx", 
      "ClusterName":"c1", 
      "Config":
      [
         {"HostName": "host1", "datagroupname": "", 
          "dbpath": "/home/db2/standalone/11830", "svcname": "11830", ...}
         ,...
      ]
   }
   */
   INT32 omBusinessConfigure::init( propertyContainer *pc, 
                                    const BSONObj &business )
   {
      _propertyContainer = pc ;
      _businessType = business.getStringField( OM_BSON_BUSINESS_TYPE ) ;
      _businessName = business.getStringField( OM_BSON_BUSINESS_NAME ) ;
      _deployMod    = business.getStringField( OM_BSON_DEPLOY_MOD ) ;
      _clusterName  = business.getStringField( OM_BSON_FIELD_CLUSTER_NAME ) ;

      INT32 rc = SDB_OK ;
      BSONElement configEle ;
      configEle = business.getField( OM_BSON_FIELD_CONFIG ) ;
      if ( configEle.eoo() || Array != configEle.type() )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "business's field is not Array:field=%s,"
                     "type=%d", OM_BSON_FIELD_CONFIG, configEle.type() ) ;
         goto error ;
      }
      {
         BSONObjIterator i( configEle.embeddedObject() ) ;
         while ( i.more() )
         {
            BSONElement ele = i.next() ;
            if ( Object == ele.type() )
            {
               BSONObj oneNode = ele.embeddedObject() ;
               omNodeConf nodeConf ;
               rc = _setNodeConf( oneNode, nodeConf ) ;
               if ( SDB_OK != rc )
               {
                  PD_LOG( PDERROR, "set node conf failed:rc=%d", rc ) ;
                  goto error ;
               }
               _nodeList.push_back( nodeConf ) ;
            }
         }
      }

      rc = _innerCheck() ;
      if (SDB_OK != rc )
      {
         PD_LOG( PDERROR, "check business failed:rc=%d", rc ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 omBusinessConfigure::_innerCheck()
   {
      INT32 rc = SDB_OK ;
      if ( _nodeList.size() == 0 )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "node is zero!" ) ;
         goto error ;
      }

      if ( _deployMod != OM_DEPLOY_MOD_STANDALONE 
               && _deployMod != OM_DEPLOY_MOD_DISTRIBUTION )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "unreconigzed deploy mode:%s", 
                     _deployMod.c_str() ) ;
         goto error ;
      }

      if ( _deployMod == OM_DEPLOY_MOD_STANDALONE )
      {
         if ( _nodeList.size() != 1 )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG_MSG( PDERROR, "can't install more than one node in mode:%s",
                        OM_DEPLOY_MOD_STANDALONE ) ;
            goto error ;
         }

         list<omNodeConf>::iterator iter = _nodeList.begin() ;
         string role = iter->getRole() ;
         if ( role != OM_NODE_ROLE_STANDALONE )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG_MSG( PDERROR, "can't install node with role=%s in mode:%s", 
                        role.c_str(), _deployMod.c_str() ) ;
            goto error ;
         }
      }
      else
      {
         // OM_DEPLOY_MOD_DISTRIBUTION
         INT32 coordNum   = 0 ;
         INT32 catalogNum = 0 ;
         INT32 dataNum    = 0 ;
         list<omNodeConf>::iterator iter = _nodeList.begin() ;
         while( iter != _nodeList.end() )
         {
            string role = iter->getRole() ;
            if ( role == OM_NODE_ROLE_STANDALONE )
            {
               rc = SDB_INVALIDARG ;
               PD_LOG_MSG( PDERROR, "can't install node with role=%s in mode:"
                           "mode=%s", role.c_str(), _deployMod.c_str() ) ;
               goto error ;
            }

            if ( role == OM_NODE_ROLE_CATALOG )
            {
               catalogNum++ ;
            }
            else if ( role == OM_NODE_ROLE_COORD )
            {
               coordNum++ ;
            }
            else if ( role == OM_NODE_ROLE_DATA )
            {
               dataNum++ ;
            }

            iter++ ;
         }

         if ( catalogNum == 0 )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG_MSG( PDERROR, "catalog number can't be zero" ) ;
            goto error ;
         }

         if ( coordNum == 0 )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG_MSG( PDERROR, "coord number can't be zero" ) ;
            goto error ;
         }

         if ( dataNum == 0 )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG_MSG( PDERROR, "data number can't be zero" ) ;
            goto error ;
         }
      }

      {
         string transaction = "" ;
         string hostName    = "" ;
         string svcName     = "" ;
         list<omNodeConf>::iterator iter = _nodeList.begin() ;
         while( iter != _nodeList.end() )
         {
            string tmp = iter->getAdditionlConf( OM_CONF_DETAIL_TRANSACTION ) ;
            if ( iter == _nodeList.begin() )
            {
               transaction = tmp ;
               hostName    = iter->getHostName() ;
               svcName     = iter->getSvcName() ;
            }
            else
            {
               if ( transaction != tmp )
               {
                  rc = SDB_INVALIDARG ;
                  PD_LOG_MSG( PDERROR, "transaction exist conflict value:"
                              "host1=%s,svcname=%s,transaction=%s;host2=%s,"
                              "svcname=%s,transaction=%s", hostName.c_str(),
                              svcName.c_str(), transaction.c_str(), 
                              iter->getHostName().c_str(), 
                              iter->getSvcName().c_str(), tmp.c_str() ) ;
                  goto error ;
               }
            }
            iter++ ;
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   void omBusinessConfigure::getNodeList( list<omNodeConf> &nodeList )
   {
      nodeList = _nodeList ;
   }

   void omBusinessConfigure::clear()
   {
      _nodeList.clear() ;
      _businessType = "" ;
      _businessName = "" ;
      _deployMod    = "" ;
      _clusterName  = "" ;
      _propertyContainer = NULL ;
   }

   omConfigGenerator::omConfigGenerator()
   {
   }

   omConfigGenerator::~omConfigGenerator()
   {
   }

   /*
   bsonTemplate:
   {
      "ClusterName":"c1","BusinessType":"sequoiadb", "BusinessName":"b1",
      "DeployMod": "standalone", 
      "Property":[{"Name":"replicanum", "Type":"int", "Default":"1", 
                      "Valid":"1", "Display":"edit box", "Edit":"false", 
                      "Desc":"", "WebName":"" }
                      , ...
                 ] 
   }
   confProperties:
   {
      "Property":[{"Name":"dbpath", "Type":"path", "Default":"/opt/sequoiadb", 
                      "Valid":"1", "Display":"edit box", "Edit":"false", 
                      "Desc":"", "WebName":"" }
                      , ...
                 ] 
   }
   bsonHostInfo:
   { 
     "HostInfo":[
                   {
                      "HostName":"host1", "ClusterName":"c1", 
                      "Disk":{"Name":"/dev/sdb", Size:"", Mount:"", Used:""},
                      "Config":[{"BusinessName":"b2","dbpath":"", svcname:"", 
                                 "role":"", ... }, ...]
                   }
                    , ... 
                ]
   }
   */
   INT32 omConfigGenerator::generateSDBConfig( const BSONObj &bsonTemplate, 
                                               const BSONObj &confProperties, 
                                               const BSONObj &bsonHostInfo, 
                                               BSONObj &bsonConfig )
   {
      _cluster.clear() ;
      _propertyContainer.clear() ;
      _template.reset() ;

      INT32 rc = _template.init( bsonTemplate ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "init template failed:rc=%d", rc ) ;
         goto error ;
      }

      rc = _parseProperties( confProperties ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "parse confProperties failed:rc=%d", rc ) ;
         goto error ;
      }

      rc = _parseCluster( bsonHostInfo ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "parse hostInfo failed:rc=%d", rc ) ;
         goto error ;
      }

      rc = _generate( bsonConfig ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "parse hostInfo failed:rc=%d", rc ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      _errorDetail = omGetMyEDUInfoSafe( EDU_INFO_ERROR ) ;
      goto done ;
   }

   /*
   newBusinessConf:
   {
      "BusinessType":"sequoiadb", "BusinessName":"b1", "DeployMod":"xx", 
      "ClusterName":"c1", 
      "Config":
      [
         {"HostName": "host1", "datagroupname": "", 
          "dbpath": "/home/db2/standalone/11830", "svcname": "11830", ...}
         ,...
      ]
   }
   confProperties:
   {
      "Property":[{"Name":"dbpath", "Type":"path", "Default":"/opt/sequoiadb", 
                      "Valid":"1", "Display":"edit box", "Edit":"false", 
                      "Desc":"", "WebName":"" }
                      , ...
                 ] 
   }
   bsonHostInfo:
   { 
     "HostInfo":[
                   {
                      "HostName":"host1", "ClusterName":"c1", 
                      "Disk":{"Name":"/dev/sdb", Size:"", Mount:"", Used:""},
                      "Config":[{"BusinessName":"b2","dbpath":"", svcname:"", 
                                 "role":"", ... }, ...]
                   }
                    , ... 
                ]
   }
   */
   INT32 omConfigGenerator::checkSDBConfig( BSONObj &newBusinessConf,
                                            const BSONObj &confProperties, 
                                            const BSONObj &bsonHostInfo )
   {
      _cluster.clear() ;
      _propertyContainer.clear() ;
      _template.reset() ;

      INT32 rc = SDB_OK ;
      rc = _parseProperties( confProperties ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "parse confProperties failed:rc=%d", rc ) ;
         goto error ;
      }

      rc = _parseCluster( bsonHostInfo ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "_parseCluster failed:rc=%d", rc ) ;
         goto error ;
      }

      // parse business
      rc = _parseNewBusiness( newBusinessConf ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "_parseNewBusiness failed:rc=%d", rc ) ;
         goto error ;
      }

      //check itself
      //check with exist cluster

   done:
      return rc ;
   error:
      _errorDetail = omGetMyEDUInfoSafe( EDU_INFO_ERROR ) ;
      goto done ;
   }

   /*
   newBusinessConf:
   {
      "BusinessType":"sequoiadb", "BusinessName":"b1", "DeployMod":"xx", 
      "ClusterName":"c1", 
      "Config":
      [
         {"HostName": "host1", "datagroupname": "", 
          "dbpath": "/home/db2/standalone/11830", "svcname": "11830", ...}
         ,...
      ]
   }
   */
   INT32 omConfigGenerator::_parseNewBusiness( const BSONObj &newBusinessConf )
   {
      INT32 rc = SDB_OK ;
      rc = _businessConf.init( &_propertyContainer, newBusinessConf ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "init business configure failed:rc=%d", rc ) ;
         goto error ;
      }

      {
         list<omNodeConf> nodeList ;
         _businessConf.getNodeList( nodeList ) ;
         list<omNodeConf>::iterator iter = nodeList.begin() ;
         while ( iter != nodeList.end() )
         {
            omNodeConf *pNodeConf = &( *iter ) ;
            rc = _cluster.checkAndAddNode( _businessConf.getBusinessName(), 
                                           pNodeConf ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "check node failed:rc=%d", rc ) ;
               goto error ;
            }
            iter++ ;
         }
      } 

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 omConfigGenerator::_generateStandAlone( list<omNodeConf> &nodeList )
   {
      INT32 rc = SDB_OK ;
      omNodeConf node ;
      string businessType = _template.getBusinessType() ;
      string businessName = _template.getBusinessName() ;
      rc = _cluster.createNode( businessType, businessName,
                                OM_NODE_ROLE_STANDALONE, "", node ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "createNode failed:businessName=%s,businessType=%s,"
                 "role=%s,rc=%d", businessType.c_str(), businessName.c_str(),
                 OM_NODE_ROLE_STANDALONE, rc ) ;
         goto error ;
      }

      nodeList.push_back( node ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 omConfigGenerator::_generateCluster( list<omNodeConf> &nodeList )
   {
      INT32 rc = SDB_OK ;
      string businessType = _template.getBusinessType() ;
      string businessName = _template.getBusinessName() ;
      INT32 coordNum = _template.getCoordNum() ;
      if ( coordNum == 0 )
      {
         coordNum = _cluster.getHostNum() ;
         _template.setCoordNum( coordNum );
      }

      INT32 iCoordCount = 0 ;
      while ( iCoordCount < coordNum )
      {
         omNodeConf node ;
         rc = _cluster.createNode( businessType, businessName,
                                   OM_NODE_ROLE_COORD, "", node ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "createNode failed:businessName=%s,businessType=%s,"
                    "role=%s,rc=%d", businessType.c_str(), businessName.c_str(),
                    OM_NODE_ROLE_COORD, rc ) ;
            goto error ;
         }

         nodeList.push_back( node ) ;

         iCoordCount++ ;
      }

      {
         INT32 catalogNum    = _template.getCatalogNum() ;
         INT32 iCatalogCount = 0 ;
         while ( iCatalogCount < catalogNum )
         {
            omNodeConf node ;
            rc = _cluster.createNode( businessType,  businessName,
                                       OM_NODE_ROLE_CATALOG, "", node ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "createNode failed:businessName=%s,businessType=%s,"
                       "role=%s,rc=%d", businessType.c_str(), businessName.c_str(),
                       OM_NODE_ROLE_CATALOG, rc ) ;
               goto error ;
            }

            nodeList.push_back( node ) ;
            iCatalogCount++ ;
         }
      }

      {
         INT32 groupID      = _cluster.increaseGroupID( businessName ) ;
         INT32 replicaNum   = _template.getReplicaNum() ;
         INT32 groupIDCycle = 0 ;
         INT32 dataCount    = 0 ;
         while ( dataCount < _template.getDataNum() )
         {
            omNodeConf node ;
            string groupName = strConnect( OM_DG_NAME_PATTERN, groupID ) ;
            rc = _cluster.createNode( businessType,  businessName,
                                      OM_NODE_ROLE_DATA, groupName, node ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "createNode failed:businessName=%s,businessType=%s,"
                       "role=%s,rc=%d", businessType.c_str(), businessName.c_str(),
                       OM_NODE_ROLE_DATA, rc ) ;
               goto error ;
            }

            nodeList.push_back( node ) ;

            dataCount++ ;
            groupIDCycle++ ;
            if ( groupIDCycle >= replicaNum )
            {
               groupID      = _cluster.increaseGroupID( businessName ) ;
               groupIDCycle = 0 ;
            }
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 omConfigGenerator::_generate( BSONObj &bsonConfig )
   {
      INT32 rc = SDB_OK ;
      list<omNodeConf> nodeList ;
      string deployMod = _template.getDeployMod() ;
      if ( ossStrcasecmp( deployMod.c_str(), OM_DEPLOY_MOD_STANDALONE ) == 0 )
      {
         rc = _generateStandAlone( nodeList ) ;
      }
      else if ( ossStrcasecmp( deployMod.c_str(), 
                               OM_DEPLOY_MOD_DISTRIBUTION ) == 0 )
      {
         rc = _generateCluster( nodeList ) ;
      }
      else
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "unrecognized deploy mod:type=%s", 
                     deployMod.c_str() ) ;
         goto error ;
      }

      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "generate config failed:rc=%d", rc ) ;
         goto error ;
      }

      {
         BSONArrayBuilder arrBuilder ;
         list<omNodeConf>::iterator iter = nodeList.begin() ;
         while( iter != nodeList.end() )
         {
            BSONObjBuilder builder ;
            builder.append( OM_BSON_FIELD_HOST_NAME, iter->getHostName() ) ;
            builder.append( OM_CONF_DETAIL_EX_DG_NAME, iter->getDataGroupName() ) ;
            builder.append( OM_CONF_DETAIL_DBPATH, iter->getDbPath() ) ;
            builder.append( OM_CONF_DETAIL_SVCNAME, iter->getSvcName() ) ;
            builder.append( OM_CONF_DETAIL_ROLE, iter->getRole() ) ;

            const map<string, string>* pAdditionalMap = iter->getAdditionalMap() ;
            map<string, string>::const_iterator additionalIter ;
            additionalIter = pAdditionalMap->begin() ;
            while( additionalIter != pAdditionalMap->end() )
            {
               builder.append( additionalIter->first, additionalIter->second ) ;
               additionalIter++ ;
            }

            arrBuilder.append( builder.obj() ) ;
            iter++ ;
         }

         BSONObjBuilder confBuilder ;
         confBuilder.append( OM_BSON_FIELD_CONFIG, arrBuilder.arr() ) ;
         confBuilder.append( OM_BSON_BUSINESS_NAME, 
                             _template.getBusinessName() ) ;
         confBuilder.append( OM_BSON_BUSINESS_TYPE, 
                             _template.getBusinessType() ) ;
         confBuilder.append( OM_BSON_DEPLOY_MOD, _template.getDeployMod() ) ;
         bsonConfig = confBuilder.obj() ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   /*
   bsonHostInfo:
   { 
     "HostInfo":[
                   {
                      "HostName":"host1", "ClusterName":"c1", 
                      "Disk":{"Name":"/dev/sdb", Size:"", Mount:"", Used:""},
                      "Config":[{"BusinessName":"b2","dbpath":"", svcname:"", 
                                 "role":"", ... }, ...]
                   }
                    , ... 
                ]
   }
   */
   INT32 omConfigGenerator::_parseCluster( const BSONObj &bsonHostInfo )
   {
      INT32 rc = SDB_OK ;
      _cluster.setPropertyContainer( &_propertyContainer ) ;

      BSONObj confFilter = BSON( OM_BSON_FIELD_CONFIG << "" ) ;
      BSONElement clusterEle = bsonHostInfo.getField( 
                                                     OM_BSON_FIELD_HOST_INFO ) ;
      if ( clusterEle.eoo() || Array != clusterEle.type() )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "hostInfo is not Array:field=%s,type=%d", 
                     OM_BSON_FIELD_HOST_INFO, clusterEle.type() ) ;
         goto error ;
      }
      {
         BSONObjIterator iter( clusterEle.embeddedObject() ) ;
         while ( iter.more() )
         {
            BSONObj oneHostConf ;
            BSONElement ele = iter.next() ;
            if ( Object == ele.type() )
            {
               BSONObj oneHostConf = ele.embeddedObject() ;
               BSONObj config  = oneHostConf.filterFieldsUndotted( confFilter, 
                                                                   true ) ;
               BSONObj oneHost = oneHostConf.filterFieldsUndotted( confFilter, 
                                                                   false ) ;
               rc = _cluster.addHost( oneHost, config ) ;
               if ( SDB_OK != rc )
               {
                  PD_LOG( PDERROR, "add host failed:rc=%d", rc ) ;
                  goto error ;
               }
            }
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   /*
   confProperties:
   {
      "Property":[{"Name":"dbpath", "Type":"path", "Default":"/opt/sequoiadb", 
                      "Valid":"1", "Display":"edit box", "Edit":"false", 
                      "Desc":"", "WebName":"" }
                      , ...
                 ] 
   }
   */
   INT32 omConfigGenerator::_parseProperties( const BSONObj &confProperties )
   {
      INT32 rc = SDB_OK ;
      BSONElement propertyEle ;
      propertyEle = confProperties.getField( OM_BSON_PROPERTY_ARRAY ) ;
      if ( propertyEle.eoo() || Array != propertyEle.type() )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "confProperties's field is not Array:field=%s,"
                     "type=%d", OM_BSON_PROPERTY_ARRAY, propertyEle.type() ) ;
         goto error ;
      }
      {
         BSONObjIterator i( propertyEle.embeddedObject() ) ;
         while ( i.more() )
         {
            BSONElement ele = i.next() ;
            if ( Object == ele.type() )
            {
               BSONObj oneProperty = ele.embeddedObject() ;
               rc = _propertyContainer.addProperty( oneProperty ) ;
               if ( SDB_OK != rc )
               {
                  PD_LOG( PDERROR, "addProperty failed:rc=%d", rc ) ;
                  goto error ;
               }
            }
         }

         if ( !_propertyContainer.isAllPropertySet() )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG( PDERROR, "miss property configure" ) ;
            goto error ;
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   string omConfigGenerator::getErrorDetail()
   {
      return _errorDetail ;
   }

   //****************Zookeeper begin*********************************
   omZooConfTemplate::omZooConfTemplate()
                     : _zooNum( -1 )
   {
   }

   omZooConfTemplate::~omZooConfTemplate()
   {
      reset() ;
   }

   INT32 omZooConfTemplate::getZooNum()
   {
      return _zooNum ;
   }

   void omZooConfTemplate::_reset()
   {
      _zooNum       = -1 ;
   }

   BOOLEAN omZooConfTemplate::_isAllProperySet()
   {
      if ( _zooNum == -1 )
      {
         PD_LOG_MSG( PDERROR, "%s have not been set", 
                     OM_TEMPLATE_ZOO_NUM ) ;
         return FALSE ;
      }

      return TRUE ;
   }

   /*
   bsonTemplate:
   {
      "ClusterName":"c1","BusinessType":"zookeeper", "BusinessName":"myzookeeper",
      "DeployMod": "zookeeper", 
      "Property":[{"Name":"zoonum", "Type":"int", "Default":"1", 
                      "Valid":"1", "Display":"edit box", "Edit":"false", 
                      "Desc":"", "WebName":"" }
                      , ...
                 ] 
   }
   */
   INT32 omZooConfTemplate::_setPropery( BSONObj &property )
   {
      INT32 rc = SDB_OK ;
      string itemName ;
      string itemValue ;
      rc = getValueAsString( property, OM_BSON_PROPERTY_NAME, itemName ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG_MSG( PDERROR, "property miss bson field=%s", 
                     OM_BSON_PROPERTY_NAME ) ;
         goto error ;
      }

      rc = getValueAsString( property, OM_BSON_PROPERTY_VALUE, 
                             itemValue ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG_MSG( PDERROR, "property miss bson field=%s", 
                     OM_BSON_PROPERTY_VALUE ) ;
         goto error ;
      }

      if ( itemName.compare( OM_TEMPLATE_ZOO_NUM ) == 0 )
      {
         _zooNum = ossAtoi( itemValue.c_str() ) ;
      }

      {
         confProperty oneProperty ;
         rc = oneProperty.init( property ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "init property failed:rc=%d", rc ) ;
            goto error ;
         }

         if ( !oneProperty.isValid( itemValue ) )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG_MSG( PDERROR, "Template value is invalid:item=%s,value=%s,"
                        "valid=%s", itemName.c_str(), itemValue.c_str(), 
                        oneProperty.getValidString().c_str() ) ;
            goto error ;
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   zooPropertyContainer::zooPropertyContainer()
                        :_installPathProperty( NULL ), 
                         _dataPathProperty( NULL ), _zooIDProperty( NULL ),
                         _dataPortProperty( NULL ), _electPortProperty( NULL ),
                         _clientPortProperty( NULL )                    
   {
   }

   zooPropertyContainer::~zooPropertyContainer()
   {
      clear() ;
   }

   INT32 zooPropertyContainer::addProperty( const BSONObj &property )
   {
      INT32 rc = SDB_OK ;
      confProperty *pConfProperty = SDB_OSS_NEW confProperty() ;
      if ( NULL == pConfProperty )
      {
         rc = SDB_OOM ;
         PD_LOG_MSG( PDERROR, "new confProperty failed:rc=%d", rc ) ;
         goto error ;
      }

      rc = pConfProperty->init( property ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "init confProperty failed:property=%s,rc=%d", 
                 property.toString(false, true ).c_str(), rc ) ;
         goto error ;
      }

      if ( OM_ZOO_CONF_DETAIL_ZOOID == pConfProperty->getItemName() )
      {
         SDB_ASSERT( NULL == _zooIDProperty, "" ) ;
         _zooIDProperty = pConfProperty ;
      }
      else if ( OM_ZOO_CONF_DETAIL_INSTALLPATH == pConfProperty->getItemName() )
      {
         SDB_ASSERT( NULL == _installPathProperty, "" ) ;
         _installPathProperty = pConfProperty ;
      }
      else if ( OM_ZOO_CONF_DETAIL_DATAPATH == pConfProperty->getItemName() )
      {
         SDB_ASSERT( NULL == _dataPathProperty, "" ) ;
         _dataPathProperty = pConfProperty ;
      }
      else if ( OM_ZOO_CONF_DETAIL_DATAPORT == pConfProperty->getItemName() )
      {
         SDB_ASSERT( NULL == _dataPortProperty, "" ) ;
         _dataPortProperty = pConfProperty ;
      }
      else if ( OM_ZOO_CONF_DETAIL_ELECTPORT == pConfProperty->getItemName() )
      {
         SDB_ASSERT( NULL == _electPortProperty, "" ) ;
         _electPortProperty = pConfProperty ;
      }
      else if ( OM_ZOO_CONF_DETAIL_CLIENTPORT == pConfProperty->getItemName() )
      {
         SDB_ASSERT( NULL == _clientPortProperty, "" ) ;
         _clientPortProperty = pConfProperty ;
      }
      else
      {
         map<string, confProperty*>::iterator iter ;
         iter = _additionalPropertyMap.find( pConfProperty->getItemName() ) ;
         if ( iter != _additionalPropertyMap.end() )
         {
            confProperty *pTmp = iter->second ;
            SDB_OSS_DEL pTmp ;
            _additionalPropertyMap.erase( iter ) ;
         }

         _additionalPropertyMap.insert( map<string, confProperty*>::value_type( 
                                                   pConfProperty->getItemName(),
                                                   pConfProperty ) ) ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 zooPropertyContainer::checkValue( const string &name, 
                                           const string &value )
   {
      INT32 rc = SDB_OK ;
      confProperty *pProperty = _getConfProperty( name ) ;
      if ( NULL == pProperty )
      {
         rc = SDB_DMS_RECORD_NOTEXIST ;
         PD_LOG_MSG( PDERROR, "can't find the property:name=%s,rc=%d", 
                     name.c_str(), rc ) ;
         goto error ;
      }

      if ( !pProperty->isValid( value ) )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "property's value is invalid:name=%s,value=%s,"
                     "valid=%s", name.c_str(), value.c_str(), 
                     pProperty->getValidString().c_str() ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   BOOLEAN zooPropertyContainer::isAllPropertySet()
   {
      if ( NULL == _installPathProperty )
      {
         PD_LOG_MSG( PDERROR, "property [%s] have not been set", 
                     OM_ZOO_CONF_DETAIL_INSTALLPATH ) ;
         return FALSE ;
      }

      if ( NULL == _dataPathProperty )
      {
         PD_LOG_MSG( PDERROR, "property [%s] have not been set", 
                     OM_ZOO_CONF_DETAIL_DATAPATH ) ;
         return FALSE ;
      }

      if ( NULL == _zooIDProperty )
      {
         PD_LOG_MSG( PDERROR, "property [%s] have not been set", 
                     OM_ZOO_CONF_DETAIL_ZOOID ) ;
         return FALSE ;
      }

      if ( NULL == _dataPortProperty )
      {
         PD_LOG_MSG( PDERROR, "property [%s] have not been set", 
                     OM_ZOO_CONF_DETAIL_DATAPORT ) ;
         return FALSE ;
      }

      if ( NULL == _electPortProperty )
      {
         PD_LOG_MSG( PDERROR, "property [%s] have not been set", 
                     OM_ZOO_CONF_DETAIL_ELECTPORT ) ;
         return FALSE ;
      }

      if ( NULL == _clientPortProperty )
      {
         PD_LOG_MSG( PDERROR, "property [%s] have not been set", 
                     OM_ZOO_CONF_DETAIL_CLIENTPORT ) ;
         return FALSE ;
      }

      return TRUE ;
   }

   void zooPropertyContainer::clear()
   {
      map<string, confProperty*>::iterator iter ;
      iter = _additionalPropertyMap.begin() ;
      while ( iter != _additionalPropertyMap.end() )
      {
         confProperty *property = iter->second ;
         SDB_OSS_DEL property ;
         _additionalPropertyMap.erase( iter++ ) ;
      }

      if ( NULL != _installPathProperty )
      {
         SDB_OSS_DEL _installPathProperty ;
         _installPathProperty = NULL ;
      }

      if ( NULL != _dataPathProperty )
      {
         SDB_OSS_DEL _dataPathProperty ;
         _dataPathProperty = NULL ;
      }

      if ( NULL != _zooIDProperty )
      {
         SDB_OSS_DEL _zooIDProperty ;
         _zooIDProperty = NULL ;
      }

      if ( NULL != _dataPortProperty )
      {
         SDB_OSS_DEL _dataPortProperty ;
         _dataPortProperty = NULL ;
      }

      if ( NULL != _clientPortProperty )
      {
         SDB_OSS_DEL _clientPortProperty ;
         _clientPortProperty = NULL ;
      }

      if ( NULL != _electPortProperty )
      {
         SDB_OSS_DEL _electPortProperty ;
         _electPortProperty = NULL ;
      }
   }

   INT32 zooPropertyContainer::createSample( omZooNodeConf &sample )
   {
      confProperty *property = NULL ;
      map<string, confProperty*>::iterator iter ;
      iter = _additionalPropertyMap.begin() ;
      while ( iter != _additionalPropertyMap.end() )
      {
         property = iter->second ;
         sample.setAdditionalConf( property->getItemName(), 
                                   property->getDefaultValue() ) ;
         iter++ ;
      }

      return SDB_OK ;
   }

   string zooPropertyContainer::getDefaultValue( const string &name )
   {
      confProperty *property = _getConfProperty( name ) ;
      if ( NULL != property )
      {
         return property->getDefaultValue() ;
      }

      return "" ;
   }

   confProperty* zooPropertyContainer::_getConfProperty( const string &name )
   {
      confProperty *property = NULL ;
      if ( OM_ZOO_CONF_DETAIL_INSTALLPATH == name )
      {
         property = _installPathProperty ;
      }
      else if ( OM_ZOO_CONF_DETAIL_DATAPATH == name )
      {
         property = _dataPathProperty ;
      }
      else if ( OM_ZOO_CONF_DETAIL_ZOOID == name )
      {
         property = _zooIDProperty ;
      }
      else if ( OM_ZOO_CONF_DETAIL_DATAPORT == name )
      {
         property = _dataPortProperty ;
      }
      else if ( OM_ZOO_CONF_DETAIL_ELECTPORT == name )
      {
         property = _electPortProperty ;
      }
      else if ( OM_ZOO_CONF_DETAIL_CLIENTPORT == name )
      {
         property = _clientPortProperty ;
      }
      else
      {
         map<string, confProperty*>::iterator iter ;
         iter = _additionalPropertyMap.find( name ) ;
         if ( iter != _additionalPropertyMap.end() )
         {
            property = iter->second ;
         }
      }

      return property ;
   }

   omZooNodeConf::omZooNodeConf()
   {
      _installPath   = "" ;
      _dataPath      = "" ;
      _zooID         = "" ;
      _dataPort      = "" ;
      _electPort     = "" ;
      _clientPort    = "" ;
      _installPath   = "" ;
      _additionalConfMap.clear() ;
   }

   omZooNodeConf::omZooNodeConf( const omZooNodeConf &right )
   {
      _installPath   = right._installPath ;
      _dataPath      = right._dataPath ;
      _zooID         = right._zooID ;
      _dataPort      = right._dataPort ;
      _electPort     = right._electPort ;
      _clientPort    = right._clientPort ;
      _hostName      = right._hostName ;
      _diskName      = right._diskName ;

      _additionalConfMap = right._additionalConfMap ;
   }

   omZooNodeConf::~omZooNodeConf()
   {
      _additionalConfMap.clear() ;
   }

   void omZooNodeConf::setInstallPath( const string &installPath )
   {
      _installPath = installPath ;
   }

   void omZooNodeConf::setDataPath( const string &dataPath )
   {
      _dataPath = dataPath ;
   }

   void omZooNodeConf::setZooID( const string &zooID )
   {
      _zooID = zooID ;
   }

   void omZooNodeConf::setDataPort( const string &dataPort )
   {
      _dataPort = dataPort ;
   }

   void omZooNodeConf::setElectPort( const string &electPort )
   {
      _electPort = electPort ;
   }

   void omZooNodeConf::setClientPort( const string &clientPort )
   {
      _clientPort = clientPort ;
   }

   void omZooNodeConf::setHostName( const string &hostName )
   {
      _hostName = hostName ;
   }

   void omZooNodeConf::setDiskName( const string &diskName )
   {
      _diskName = diskName ;
   }

   void omZooNodeConf::setAdditionalConf( const string &key, 
                                          const string &value )
   {
      _additionalConfMap[key] = value ;
   }

   string omZooNodeConf::getInstallPath()
   {
      return _installPath ;
   }

   string omZooNodeConf::getDataPath()
   {
      return _dataPath ;
   }

   string omZooNodeConf::getZooID()
   {
      return _zooID ;
   }

   string omZooNodeConf::getDataPort()
   {
      return _dataPort ;
   }

   string omZooNodeConf::getElectPort()
   {
      return _electPort ;
   }

   string omZooNodeConf::getClientPort()
   {
      return _clientPort ;
   }

   string omZooNodeConf::getHostName()
   {
      return _hostName ;
   }

   string omZooNodeConf::getDiskName()
   {
      return _diskName ;
   }

   string omZooNodeConf::getAdditionlConf( const string &key )
   {
      string value = "" ;
      map<string, string>::iterator iter = _additionalConfMap.find( key ) ;
      if ( iter != _additionalConfMap.end() )
      {
         value = iter->second ;
      }

      return value ;
   }

   const map<string, string>* omZooNodeConf::getAdditionalMap()
   {
      return &_additionalConfMap ;
   }

   zooNodeCounter::zooNodeCounter()
   {
   }

   zooNodeCounter::~zooNodeCounter()
   {
      clear() ;
   }

   INT32 zooNodeCounter::addNode( const string &hostName,
                                  const string &diskName,
                                  const string &businessName, 
                                  const string &zooID )
   {
      INT32 rc       = SDB_OK ;
      INT32 tmpZooID = 0 ;
      map<string, hostNodeCounter *>::iterator iter ;
      iter = _mapHostNodeCounter.find( hostName ) ;
      if ( iter == _mapHostNodeCounter.end() )
      {
         hostNodeCounter *hnc = SDB_OSS_NEW hostNodeCounter( hostName ) ;
         if ( NULL == hnc )
         {
            rc = SDB_OOM ;
            PD_LOG_MSG( PDERROR, "out of memory" ) ;      
            goto error ;
         }

         _mapHostNodeCounter.insert( 
               map<string, hostNodeCounter*>::value_type( hostName, hnc ) ) ;
      }

      _mapHostNodeCounter[hostName]->addNode( diskName, businessName, 
                                              OM_BUSINESS_ZOOKEEPER ) ;
      _counter.increaseNode( OM_BUSINESS_ZOOKEEPER ) ;

      {
         map<string, INT32>::iterator iter ;
         iter = _availableZooIDMap.find( businessName) ;
         if ( iter == _availableZooIDMap.end() )
         {
            _availableZooIDMap[ businessName ] = 1 ;
         }
      }

      tmpZooID = ossAtoi( zooID.c_str() ) ;
      if ( tmpZooID  >= _availableZooIDMap[ businessName ] )
      {
         _availableZooIDMap[ businessName ] = tmpZooID  + 1 ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 zooNodeCounter::getCountInHost( const string &hostName )
   {
      INT32 count = 0 ;
      map<string, hostNodeCounter *>::iterator iter ;
      iter = _mapHostNodeCounter.find( hostName ) ;
      if ( iter != _mapHostNodeCounter.end() )
      {
         hostNodeCounter *p = iter->second ;
         count = p->getNodeCount() ;
      }

      return count ;
   }

   INT32 zooNodeCounter::getCountInDisk( const string &hostName,
                                         const string &diskName )
   {
      INT32 count = 0 ;
      map<string, hostNodeCounter *>::iterator iter ;
      iter = _mapHostNodeCounter.find( hostName ) ;
      if ( iter != _mapHostNodeCounter.end() )
      {
         hostNodeCounter *p = iter->second ;
         count = p->getNodeCountInDisk( diskName ) ;
      }

      return count ;
   }

   INT32 zooNodeCounter::increaseZooID( const string &businessName )
   {
      INT32 id = 1 ;
      map<string, INT32>::iterator iter ;
      iter = _availableZooIDMap.find( businessName) ;
      if ( iter == _availableZooIDMap.end() )
      {
         _availableZooIDMap[ businessName ] = 1 ;
      }

      id = _availableZooIDMap[ businessName ] ;
      _availableZooIDMap[ businessName ]++ ;

      return id ;
   }

   void zooNodeCounter::clear()
   {
      map<string, hostNodeCounter*>::iterator iter ;
      iter = _mapHostNodeCounter.begin() ;
      while ( iter != _mapHostNodeCounter.end() )
      {
         SDB_OSS_DEL iter->second ;
         _mapHostNodeCounter.erase( iter++ ) ;
      }

      _availableZooIDMap.clear() ;
   }

   omZooCluster::omZooCluster()
   {
   }

   omZooCluster::~omZooCluster()
   {
      clear() ;
   }

   void omZooCluster::setPropertyContainer( zooPropertyContainer *pc )
   {
      _propertyContainer = pc ;
   }

   INT32 omZooCluster::addHost( const BSONObj &host, const BSONObj &config )
   {
      INT32 rc = SDB_OK ;
      string hostName = host.getStringField( OM_BSON_FIELD_HOST_NAME ) ;
      map<string, hostHardWare*>::iterator iter = _mapHost.find( hostName ) ;
      SDB_ASSERT( iter == _mapHost.end(), "" ) ;
      string defaultSvcName = _propertyContainer->getDefaultValue(
                                                 OM_ZOO_CONF_DETAIL_DATAPORT ) ;
      hostHardWare *pHostHW = SDB_OSS_NEW hostHardWare( hostName, 
                                                        defaultSvcName ) ;
      if ( NULL == pHostHW )
      {
         rc = SDB_OOM ;
         PD_LOG( PDERROR, "new hostHardWare failed:rc=%d", rc ) ;
         goto error ;
      }

      _mapHost.insert( map<string, hostHardWare*>::value_type( hostName, 
                                                               pHostHW ) ) ;

      {
         BSONObj disks = host.getObjectField( OM_BSON_FIELD_DISK ) ;
         BSONObjIterator i( disks ) ;
         while ( i.more() )
         {
            string tmp ;
            BSONElement ele = i.next() ;
            if ( Object == ele.type() )
            {
               BSONObj oneDisk = ele.embeddedObject() ;
               string diskName ;
               string mountPath ;
               diskName  = oneDisk.getStringField( OM_BSON_FIELD_DISK_NAME ) ;
               mountPath = oneDisk.getStringField( OM_BSON_FIELD_DISK_MOUNT ) ;
               pHostHW->addDisk( diskName, mountPath, 0, 0 ) ;
            }
         }
      }

      {
         BSONObj nodes = config.getObjectField( OM_BSON_FIELD_CONFIG ) ;
         BSONObjIterator i( nodes ) ;
         while ( i.more() )
         {
            string tmp ;
            BSONElement ele = i.next() ;
            if ( Object == ele.type() )
            {
               list<string> portList ;
               BSONObj oneNode = ele.embeddedObject() ;
               string businessType = oneNode.getStringField(
                                             OM_BSON_BUSINESS_TYPE ) ;
               if ( businessType == OM_BUSINESS_SEQUOIADB )
               {
                  string businessName ;
                  string dbPath ;
                  string role ;
                  string svcName ;
                  string groupName ;
                  string diskName ;
                  businessName = oneNode.getStringField( 
                                                     OM_BSON_BUSINESS_NAME ) ;
                  dbPath       = oneNode.getStringField( 
                                                     OM_CONF_DETAIL_DBPATH ) ;
                  role         = oneNode.getStringField( OM_CONF_DETAIL_ROLE ) ;
                  svcName      = oneNode.getStringField( 
                                                     OM_CONF_DETAIL_SVCNAME ) ;
                  groupName    = oneNode.getStringField( 
                                                OM_CONF_DETAIL_DATAGROUPNAME ) ;
                  diskName     = pHostHW->getDiskName( dbPath ) ;
                  SDB_ASSERT( diskName != "" ,"" ) ;

                  portList.push_back( svcName ) ;
                  portList.push_back( strPlus( svcName, 1 ) ) ;
                  portList.push_back( strPlus( svcName, 2 ) ) ;
                  portList.push_back( strPlus( svcName, 3 ) ) ;
                  portList.push_back( strPlus( svcName, 4 ) ) ;
                  portList.push_back( strPlus( svcName, 5 ) ) ;
                  pHostHW->occupayResource( dbPath, portList ) ;
               }
               else if ( businessType == OM_BUSINESS_ZOOKEEPER )
               {
                  string businessName = oneNode.getStringField( 
                                             OM_BSON_BUSINESS_NAME ) ;
                  string installPath  = oneNode.getStringField( 
                                             OM_ZOO_CONF_DETAIL_INSTALLPATH ) ;
                  string zooID        = oneNode.getStringField( 
                                             OM_ZOO_CONF_DETAIL_ZOOID ) ;
                  string dataPath     = oneNode.getStringField( 
                                             OM_ZOO_CONF_DETAIL_DATAPATH ) ;
                  string dataPort     = oneNode.getStringField( 
                                             OM_ZOO_CONF_DETAIL_DATAPORT ) ;
                  string electPort    = oneNode.getStringField( 
                                             OM_ZOO_CONF_DETAIL_ELECTPORT) ;
                  string clientPort   = oneNode.getStringField( 
                                             OM_ZOO_CONF_DETAIL_CLIENTPORT ) ;
                  string diskName     = pHostHW->getDiskName( dataPath ) ;
                  SDB_ASSERT( diskName != "" ,"" ) ;

                  //empty list
                  pHostHW->occupayResource( installPath, portList, FALSE ) ;

                  portList.push_back( dataPort ) ;
                  portList.push_back( electPort ) ;
                  portList.push_back( clientPort ) ;
                  pHostHW->occupayResource( dataPath, portList ) ;
                  rc = _nodeCounter.addNode( hostName, diskName, businessName, 
                                             zooID ) ;
                  if ( SDB_OK != rc )
                  {
                     PD_LOG( PDERROR, "add node failed:rc=%d", rc ) ;
                     goto error ;
                  }
               }
               else
               {
                  SDB_ASSERT( FALSE, businessType.c_str() ) ;
               }
            }
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 omZooCluster::_getBestResourceFromHost( hostHardWare *host, 
                                                 simpleDiskInfo **diskInfo, 
                                                 string &svcName )
   {
      INT32 rc                 = SDB_OK ;
      simpleDiskInfo *bestDisk = NULL ;
      string hostName          = host->getName() ;

      map<string, simpleDiskInfo> *mapDisk       = host->getDiskMap() ;
      map<string, simpleDiskInfo>::iterator iter = mapDisk->begin() ;
      while ( iter != mapDisk->end() )
      {
         if ( NULL == bestDisk )
         {
            //get the first disk
            bestDisk = &( iter->second ) ;
            iter++ ;
            continue ;
         }

         simpleDiskInfo *pTmp = &( iter->second ) ;
         INT32 bestCount = _nodeCounter.getCountInDisk( hostName, 
                                                        bestDisk->diskName ) ;
         INT32 tmpCount  = _nodeCounter.getCountInDisk( hostName,
                                                        pTmp->diskName ) ;
         //total count less, the better
         if ( tmpCount < bestCount )
         {
            bestDisk = &( iter->second ) ;
         }

         iter++ ;
      }

      *diskInfo = bestDisk ;
      if ( NULL == *diskInfo )
      {
         rc = SDB_DMS_RECORD_NOTEXIST ;
         PD_LOG( PDERROR, "get disk failed:host=%s", hostName.c_str() ) ;
         goto error ;
      }

      svcName = host->getAvailableSvcName() ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 omZooCluster::createNode( const string &businessType,
                                   const string &businessName,
                                   omZooNodeConf &node )
   {
      INT32 rc = SDB_OK ;
      list <string> portList ;
      INT32 pathAdjustIndex    = 0 ;
      simpleDiskInfo *diskInfo = NULL ;
      string hostName ;
      string diskName ;
      string zooID ;
      string dataPort ;
      string electPort ;
      string clientPort ;
      string installPath ;
      string dataPath ;
      _propertyContainer->createSample( node ) ;
      hostHardWare *host = _getBestHost() ;
      if ( NULL == host )
      {
         rc = SDB_DMS_RECORD_NOTEXIST ;
         PD_LOG_MSG( PDERROR, 
                     "create node failed:host is zero or disk is zero" ) ;
         goto error ;
      }

      rc = _getBestResourceFromHost( host, &diskInfo, dataPort ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "_getBestResourceFromHost failed:host=%s", 
                 host->getName().c_str() ) ;
         goto error ;
      }

      // install path
      do 
      {
         CHAR tmpInstallPath[OM_PATH_LENGTH + 1] = "";
         utilBuildFullPath( OM_DEFAULT_INSTALL_ROOT_PATH, businessType.c_str(), 
                            OM_PATH_LENGTH, tmpInstallPath ) ;
         utilCatPath( tmpInstallPath, OM_PATH_LENGTH, businessName.c_str() ) ;
         if ( 0 == pathAdjustIndex )
         {
            utilCatPath( tmpInstallPath, OM_PATH_LENGTH, dataPort.c_str() ) ;
         }
         else
         {
            CHAR tmpSvcName[OM_PATH_LENGTH + 1] = "";
            ossSnprintf( tmpSvcName, OM_PATH_LENGTH, "%s_%d", dataPort.c_str(), 
                         pathAdjustIndex ) ;
            utilCatPath( tmpInstallPath, OM_PATH_LENGTH, tmpSvcName ) ;
         }
         pathAdjustIndex++ ;
         installPath = tmpInstallPath ;
      }while ( host->isPathOccupayed( installPath ) ) ;

      // data path
      pathAdjustIndex = 0 ;
      do 
      {
         CHAR tmpDataPath[OM_PATH_LENGTH + 1] = "";
         utilBuildFullPath( diskInfo->mountPath.c_str(), businessType.c_str(), 
                            OM_PATH_LENGTH, tmpDataPath ) ;
         utilCatPath( tmpDataPath, OM_PATH_LENGTH, businessName.c_str() ) ;
         utilCatPath( tmpDataPath, OM_PATH_LENGTH, OM_DBPATH_PREFIX_DATABASE ) ;
         if ( 0 == pathAdjustIndex )
         {
            utilCatPath( tmpDataPath, OM_PATH_LENGTH, dataPort.c_str() ) ;
         }
         else
         {
            CHAR tmpSvcName[OM_PATH_LENGTH + 1] = "";
            ossSnprintf( tmpSvcName, OM_PATH_LENGTH, "%s_%d", dataPort.c_str(), 
                         pathAdjustIndex ) ;
            utilCatPath( tmpDataPath, OM_PATH_LENGTH, tmpSvcName ) ;
         }
         pathAdjustIndex++ ;
         dataPath = tmpDataPath ;
      }while ( host->isPathOccupayed( dataPath ) ) ;

      hostName = host->getName() ;
      diskName = diskInfo->diskName ;

      //empty list
      portList.clear() ;
      host->occupayResource( installPath, portList, FALSE ) ;

      portList.push_back( dataPort ) ;
      electPort  = strPlus( dataPort, 1 ) ;
      clientPort = strPlus( dataPort, 2 ) ;
      portList.push_back( electPort ) ;
      portList.push_back( clientPort ) ;
      host->occupayResource( dataPath, portList ) ;
      {
         CHAR cZooID[ OM_INT32_LENGTH + 1 ] = "" ;
         INT32 iZooID = increaseZooID( businessName ) ;
         ossItoa( iZooID, cZooID, OM_INT32_LENGTH ) ;
         zooID = cZooID ;
      }
      rc = _nodeCounter.addNode( hostName, diskName, businessName, zooID ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "addNode failed:rc=%d", rc ) ;
         goto error ;
      }

      node.setDataPath( dataPath ) ;
      node.setInstallPath( installPath ) ;
      node.setClientPort( clientPort ) ;
      node.setDataPort( dataPort ) ;
      node.setDiskName( diskName ) ;
      node.setHostName( hostName ) ;
      node.setElectPort( electPort ) ;
      node.setZooID( zooID ) ;
      _addBizZooID( businessName, zooID ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   void omZooCluster::_addBizZooID( const string &businessName, 
                                    const string &zooID )
   {
      map< string, set<string> >::iterator iter ;
      iter = _bizZooIDMap.find( businessName ) ;
      if ( iter == _bizZooIDMap.end() )
      {
         set<string> tmpSet ;
         tmpSet.insert( zooID ) ;
         _bizZooIDMap[ businessName ] = tmpSet ;
      }
      else
      {
         _bizZooIDMap[ businessName ].insert( zooID ) ;
      }
   }

   BOOLEAN omZooCluster::_isBizZooIDExist( const string &businessName, 
                                           const string &zooID )
   {
      map< string, set<string> >::iterator iter ;
      iter = _bizZooIDMap.find( businessName ) ;
      if ( iter != _bizZooIDMap.end() )
      {
         set<string>::iterator iterSet ;
         iterSet = _bizZooIDMap[ businessName ].find( zooID ) ;
         if ( iterSet != _bizZooIDMap[ businessName ].end() )
         {
            return TRUE ;
         }
      }

      return FALSE ;
   }

   INT32 omZooCluster::getHostNum()
   {
      return _mapHost.size() ;
   }

   INT32 omZooCluster::increaseZooID( const string &businessName )
   {
      return _nodeCounter.increaseZooID( businessName ) ;
   }

   void omZooCluster::clear()
   {
      map<string, hostHardWare*>::iterator iter = _mapHost.begin() ;
      while ( iter != _mapHost.end() )
      {
         hostHardWare *pHost = iter->second ;
         SDB_OSS_DEL pHost ;
         _mapHost.erase( iter++ ) ;
      }

      _propertyContainer = NULL ;
      _nodeCounter.clear() ;
      _bizZooIDMap.clear() ;
   }

   INT32 omZooCluster::checkAndAddNode( const string &businessName,
                                        omZooNodeConf *node )
   {
      INT32 rc           = SDB_OK ;
      string zooID       = node->getZooID() ;
      string installPath = node->getInstallPath() ;
      string dataPath    = node->getDataPath() ;
      string dataPort    = node->getDataPort() ;
      string electPort   = node->getElectPort() ;
      string clientPort  = node->getClientPort() ;
      string hostName    = node->getHostName() ;

      map<string, hostHardWare*>::iterator iter = _mapHost.find( hostName ) ;
      if ( iter == _mapHost.end() )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "host is not exist:hostName=%s", 
                     hostName.c_str() ) ;
         goto error ;
      }

      {
         string diskName ;
         list <string> portList ;
         hostHardWare *hw = iter->second ;
         if ( !hw->isDiskExist( dataPath ) )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG_MSG( PDERROR, "dataPath's disk is not exist:hostName=%s,"
                        "dataPath=%s", hostName.c_str(), dataPath.c_str() ) ;
            goto error ;
         }

         if ( hw->isPathOccupayed( dataPath ) )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG_MSG( PDERROR, "dataPath is exist:dataPath=%s", 
                        dataPath.c_str() ) ;
            goto error ;
         }

         if ( hw->isPathOccupayed( installPath ) )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG_MSG( PDERROR, "installpath is exist:installPath=%s", 
                        installPath.c_str() ) ;
            goto error ;
         }

         if ( hw->isSvcNameOccupayed( dataPort ) )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG_MSG( PDERROR, "dataPort is exist:dataPort=%s", 
                        dataPort.c_str() ) ;
            goto error ;
         }

         if ( hw->isSvcNameOccupayed( electPort ) )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG_MSG( PDERROR, "electPort is exist:electPort=%s", 
                        electPort.c_str() ) ;
            goto error ;
         }

         if ( hw->isSvcNameOccupayed( clientPort ) )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG_MSG( PDERROR, "clientPort is exist:clientPort=%s", 
                        clientPort.c_str() ) ;
            goto error ;
         }

         if ( _isBizZooIDExist( businessName, zooID ) )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG_MSG( PDERROR, "zooid is exist:businessName=%s,zooid=%s",
                        businessName.c_str(), zooID.c_str() ) ;
            goto error ;
         }

         _addBizZooID( businessName, zooID ) ;

         //add empty list
         portList.clear() ;
         hw->occupayResource( installPath, portList, FALSE ) ;

         portList.push_back( dataPort ) ;
         portList.push_back( electPort ) ;
         portList.push_back( clientPort ) ;
         hw->occupayResource( dataPath, portList ) ;
         diskName = hw->getDiskName( dataPath ) ;
         _nodeCounter.addNode( hostName, diskName, businessName, zooID ) ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   /*
      get best host rule:
          rule1: the more the better which host contains unused disk's count
          rule2: the less the better which host contains node's count
   */
   hostHardWare* omZooCluster::_getBestHost()
   {
      map<string, hostHardWare*>::iterator iter ;
      hostHardWare *bestHost = NULL ;
      if ( _mapHost.size() == 0 )
      {
         PD_LOG( PDERROR, "host count is zero" ) ;
         goto error ;
      }

      iter = _mapHost.begin() ;
      while ( iter != _mapHost.end() )
      {
         hostHardWare *pTmpHost = iter->second ;

         //ignore the host without disk
         if ( 0 == pTmpHost->getDiskCount() )
         {
            iter++ ;
            continue ;
         }

         //this is the first one. continue
         if ( NULL == bestHost )
         {
            bestHost = pTmpHost ;
            iter++ ;
            continue ;
         }

         INT32 tmpFreeDiskCount  = pTmpHost->getFreeDiskCount();
         INT32 bestFreeDiskCount = bestHost->getFreeDiskCount() ;
         if ( tmpFreeDiskCount != bestFreeDiskCount )
         {
            if ( tmpFreeDiskCount > bestFreeDiskCount )
            {
               // rule1
               bestHost = pTmpHost ;
            }

            iter++ ;
            continue ;
         }

         INT32 tmpNodeCount  = _nodeCounter.getCountInHost( 
                                                         pTmpHost->getName() ) ;
         INT32 bestNodeCount = _nodeCounter.getCountInHost( 
                                                         bestHost->getName() ) ;
         if ( tmpNodeCount != bestNodeCount )
         {
            if ( tmpNodeCount < bestNodeCount )
            {
               // rule2
               bestHost = pTmpHost ;
            }

            iter++ ;
            continue ;
         }

         iter++ ;
      }

   done:
      return bestHost ;
   error:
      goto done ;
   }

   omZooBizConfigure::omZooBizConfigure()
   {
   }
   
   omZooBizConfigure::~omZooBizConfigure()
   {
      clear() ;
   }

   /*
   business:
   {
      "BusinessType":"zookeeper", "BusinessName":"z1", "DeployMod":"xx", 
      "ClusterName":"c1", 
      "Config":
      [
         {"HostName": "host1", "zooid": "1", 
          "datapath": "/home/zookeeper/z1/2888", "dataport": "2888", ...}
         ,...
      ]
   }
   */
   INT32 omZooBizConfigure::init( zooPropertyContainer *pc, 
                                  const BSONObj &business )
   {
      _propertyContainer = pc ;
      _businessType = business.getStringField( OM_BSON_BUSINESS_TYPE ) ;
      _businessName = business.getStringField( OM_BSON_BUSINESS_NAME ) ;
      _deployMod    = business.getStringField( OM_BSON_DEPLOY_MOD ) ;
      _clusterName  = business.getStringField( OM_BSON_FIELD_CLUSTER_NAME ) ;

      INT32 rc = SDB_OK ;
      BSONElement configEle ;
      configEle = business.getField( OM_BSON_FIELD_CONFIG ) ;
      if ( configEle.eoo() || Array != configEle.type() )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "business's field is not Array:field=%s,"
                     "type=%d", OM_BSON_FIELD_CONFIG, configEle.type() ) ;
         goto error ;
      }
      {
         BSONObjIterator i( configEle.embeddedObject() ) ;
         while ( i.more() )
         {
            BSONElement ele = i.next() ;
            if ( Object == ele.type() )
            {
               BSONObj oneNode = ele.embeddedObject() ;
               omZooNodeConf nodeConf ;
               rc = _setNodeConf( oneNode, nodeConf ) ;
               if ( SDB_OK != rc )
               {
                  PD_LOG( PDERROR, "set node conf failed:rc=%d", rc ) ;
                  goto error ;
               }
               _nodeList.push_back( nodeConf ) ;
            }
         }
      }

      rc = _innerCheck() ;
      if (SDB_OK != rc )
      {
         PD_LOG( PDERROR, "check business failed:rc=%d", rc ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }
   
   void omZooBizConfigure::clear()
   {
      _nodeList.clear() ;
      _businessType = "" ;
      _businessName = "" ;
      _deployMod    = "" ;
      _clusterName  = "" ;
      _propertyContainer = NULL ;
   }

   void omZooBizConfigure::getNodeList( list<omZooNodeConf> &nodeList )
   {
      nodeList = _nodeList ;
   }

   string omZooBizConfigure::getBusinessName()
   {
      return _businessName ;
   }
   
   INT32 omZooBizConfigure::_innerCheck()
   {
      INT32 rc = SDB_OK ;
      if ( _nodeList.size() == 0 )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "node is zero!" ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }
   
   INT32 omZooBizConfigure::_setNodeConf( BSONObj &oneNode, 
                                          omZooNodeConf &nodeConf )
   {
      INT32 rc = SDB_OK ;
      BSONObjIterator itemIter( oneNode ) ;
      while ( itemIter.more() )
      {
         BSONElement itemEle = itemIter.next() ;
         string fieldName    = itemEle.fieldName() ;
         string value        = itemEle.String() ;
         if ( OM_BSON_FIELD_HOST_NAME == fieldName )
         {
            nodeConf.setHostName( value ) ;
         }
         else 
         {
            rc = _propertyContainer->checkValue( fieldName, value ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "check value failed:name=%s,value=%s", 
                       fieldName.c_str(), value.c_str() ) ;
               goto error ;
            }

            if ( OM_ZOO_CONF_DETAIL_CLIENTPORT == fieldName )
            {
               nodeConf.setClientPort( value ) ;
            }
            else if ( OM_ZOO_CONF_DETAIL_DATAPATH == fieldName )
            {
               nodeConf.setDataPath( value ) ;
            }
            else if ( OM_ZOO_CONF_DETAIL_DATAPORT == fieldName )
            {
               nodeConf.setDataPort( value ) ;
            }
            else if ( OM_ZOO_CONF_DETAIL_ELECTPORT == fieldName )
            {
               nodeConf.setElectPort( value ) ;
            }
            else if ( OM_ZOO_CONF_DETAIL_INSTALLPATH == fieldName )
            {
               nodeConf.setInstallPath( value ) ;
            }
            else if ( OM_ZOO_CONF_DETAIL_ZOOID == fieldName )
            {
               nodeConf.setZooID( value ) ;
            }
            else 
            {
               nodeConf.setAdditionalConf( fieldName, value ) ;
            }
         }
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   omZooConfigGenerator::omZooConfigGenerator()
   {
   }
   
   omZooConfigGenerator::~omZooConfigGenerator()
   {
   }

   /*
   bsonTemplate:
   {
      "ClusterName":"c1","BusinessType":"zookeeper", "BusinessName":"z1",
      "DeployMod": "distribution", 
      "Property":[{"Name":"zoonum", "Type":"int", "Default":"1", 
                      "Valid":"1", "Display":"edit box", "Edit":"false", 
                      "Desc":"", "WebName":"" }
                 ] 
   }
   confProperties:
   {
      "Property":[{"Name":"installpath", "Type":"path", "Default":"/opt/zookeeper", 
                      "Valid":"1", "Display":"edit box", "Edit":"false", 
                      "Desc":"", "WebName":"" }
                      , ...
                 ] 
   }
   bsonHostInfo:
   { 
     "HostInfo":[
                   {
                      "HostName":"host1", "ClusterName":"c1", 
                      "Disk":{"Name":"/dev/sdb", Size:"", Mount:"", Used:""},
                      "Config":[{"BusinessName":"b2","dbpath":"", svcname:"", 
                                 "role":"", ... }, ...]
                   }
                    , ... 
                ]
   }
   */
   INT32 omZooConfigGenerator::generateConfig( const BSONObj &bsonTemplate, 
                                               const BSONObj &confProperties, 
                                               const BSONObj &bsonHostInfo, 
                                               BSONObj &bsonConfig )
   {
      _cluster.clear() ;
      _propertyContainer.clear() ;
      _template.reset() ;

      INT32 rc = _template.init( bsonTemplate ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "init template failed:rc=%d", rc ) ;
         goto error ;
      }

      rc = _parseProperties( confProperties ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "parse confProperties failed:rc=%d", rc ) ;
         goto error ;
      }

      rc = _parseCluster( bsonHostInfo ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "parse hostInfo failed:rc=%d", rc ) ;
         goto error ;
      }

      _propertyContainer.createSample( _nodeSample ) ;
      rc = _generate( bsonConfig ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "parse hostInfo failed:rc=%d", rc ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      _errorDetail = omGetMyEDUInfoSafe( EDU_INFO_ERROR ) ;
      goto done ;
   }

   string omZooConfigGenerator::getErrorDetail()
   {
      return _errorDetail ;
   }

   /*
   confProperties:
   {
      "Property":[{"Name":"installpath", "Type":"path", "Default":"/opt/sequoiadb", 
                      "Valid":"1", "Display":"edit box", "Edit":"false", 
                      "Desc":"", "WebName":"" }
                      , ...
                 ] 
   }
   */
   INT32 omZooConfigGenerator::_parseProperties( const BSONObj &confProperties )
   {
      INT32 rc = SDB_OK ;
      BSONElement propertyEle ;
      propertyEle = confProperties.getField( OM_BSON_PROPERTY_ARRAY ) ;
      if ( propertyEle.eoo() || Array != propertyEle.type() )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "confProperties's field is not Array:field=%s,"
                     "type=%d", OM_BSON_PROPERTY_ARRAY, propertyEle.type() ) ;
         goto error ;
      }
      {
         BSONObjIterator i( propertyEle.embeddedObject() ) ;
         while ( i.more() )
         {
            BSONElement ele = i.next() ;
            if ( Object == ele.type() )
            {
               BSONObj oneProperty = ele.embeddedObject() ;
               rc = _propertyContainer.addProperty( oneProperty ) ;
               if ( SDB_OK != rc )
               {
                  PD_LOG( PDERROR, "addProperty failed:rc=%d", rc ) ;
                  goto error ;
               }
            }
         }

         if ( !_propertyContainer.isAllPropertySet() )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG( PDERROR, "miss property configure" ) ;
            goto error ;
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   /*
   bsonHostInfo:
   { 
     "HostInfo":[
                   {
                      "HostName":"host1", "ClusterName":"c1", 
                      "Disk":{"Name":"/dev/sdb", Size:"", Mount:"", Used:""},
                      "Config":[{"BusinessName":"b2","BusinessType":"sequoiadb",
                                 "dbpath":"", svcname:"", "role":"", ... }, ...
                               ]
                   }
                    , ... 
                ]
   }
   */
   INT32 omZooConfigGenerator::_parseCluster( const BSONObj &bsonHostInfo )
   {
      INT32 rc = SDB_OK ;
      _cluster.setPropertyContainer( &_propertyContainer ) ;

      BSONObj confFilter = BSON( OM_BSON_FIELD_CONFIG << "" ) ;
      BSONElement clusterEle = bsonHostInfo.getField( 
                                                     OM_BSON_FIELD_HOST_INFO ) ;
      if ( clusterEle.eoo() || Array != clusterEle.type() )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "hostInfo is not Array:field=%s,type=%d", 
                     OM_BSON_FIELD_HOST_INFO, clusterEle.type() ) ;
         goto error ;
      }
      {
         BSONObjIterator iter( clusterEle.embeddedObject() ) ;
         while ( iter.more() )
         {
            BSONObj oneHostConf ;
            BSONElement ele = iter.next() ;
            if ( Object == ele.type() )
            {
               BSONObj oneHostConf = ele.embeddedObject() ;
               BSONObj config  = oneHostConf.filterFieldsUndotted( confFilter, 
                                                                   true ) ;
               BSONObj oneHost = oneHostConf.filterFieldsUndotted( confFilter, 
                                                                   false ) ;
               rc = _cluster.addHost( oneHost, config ) ;
               if ( SDB_OK != rc )
               {
                  PD_LOG( PDERROR, "add host failed:rc=%d", rc ) ;
                  goto error ;
               }
            }
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 omZooConfigGenerator::_generate( BSONObj &bsonConfig )
   {
      INT32 rc = SDB_OK ;
      INT32 num = 0 ;
      list<omZooNodeConf> nodeList ;
      string businessName = _template.getBusinessName() ;
      string businessType = _template.getBusinessType() ;
      while ( num < _template.getZooNum() )
      {
         omZooNodeConf node ;
         rc = _cluster.createNode( businessType, businessName, node );
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "createNode failed:businessName=%s,businessType=%s,"
                    "role=%s,rc=%d", businessType.c_str(), businessName.c_str(),
                    OM_NODE_ROLE_DATA, rc ) ;
            goto error ;
         }

         nodeList.push_back( node ) ;

         num++ ;
      }

      {
         BSONArrayBuilder arrBuilder ;
         list<omZooNodeConf>::iterator iter = nodeList.begin() ;
         while( iter != nodeList.end() )
         {
            BSONObjBuilder builder ;
            builder.append( OM_BSON_FIELD_HOST_NAME, iter->getHostName() ) ;
            builder.append( OM_ZOO_CONF_DETAIL_ZOOID, iter->getZooID() ) ;
            builder.append( OM_ZOO_CONF_DETAIL_INSTALLPATH, 
                            iter->getInstallPath() ) ;
            builder.append( OM_ZOO_CONF_DETAIL_DATAPATH, iter->getDataPath() ) ;
            builder.append( OM_ZOO_CONF_DETAIL_DATAPORT, iter->getDataPort() ) ;
            builder.append( OM_ZOO_CONF_DETAIL_ELECTPORT, 
                            iter->getElectPort() ) ;
            builder.append( OM_ZOO_CONF_DETAIL_CLIENTPORT, 
                            iter->getClientPort() ) ;

            const map<string, string>* pAdditionalMap = iter->getAdditionalMap() ;
            map<string, string>::const_iterator additionalIter ;
            additionalIter = pAdditionalMap->begin() ;
            while( additionalIter != pAdditionalMap->end() )
            {
               builder.append( additionalIter->first, additionalIter->second ) ;
               additionalIter++ ;
            }

            arrBuilder.append( builder.obj() ) ;
            iter++ ;
         }

         BSONObjBuilder confBuilder ;
         confBuilder.append( OM_BSON_FIELD_CONFIG, arrBuilder.arr() ) ;
         confBuilder.append( OM_BSON_BUSINESS_NAME, 
                             _template.getBusinessName() ) ;
         confBuilder.append( OM_BSON_BUSINESS_TYPE, 
                             _template.getBusinessType() ) ;
         confBuilder.append( OM_BSON_DEPLOY_MOD, _template.getDeployMod() ) ;
         bsonConfig = confBuilder.obj() ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   /*
   newBusinessConf:
   {
      "BusinessType":"zookeeper", "BusinessName":"z1", "DeployMod":"xx", 
      "ClusterName":"c1", 
      "Config":
      [
         {"HostName": "host1", "zooid": "1", 
          "installpath": "/opt/zookeeper/z1/", "clientport": "2888", ...}
         ,...
      ]
   }
   confProperties:
   {
      "Property":[{"Name":"installpath", "Type":"path", "Default":"/opt/zookeeper", 
                      "Valid":"1", "Display":"edit box", "Edit":"false", 
                      "Desc":"", "WebName":"" }
                      , ...
                 ] 
   }
   bsonHostInfo:
   { 
     "HostInfo":[
                   {
                      "HostName":"host1", "ClusterName":"c1", 
                      "Disk":{"Name":"/dev/sdb", Size:"", Mount:"", Used:""},
                      "Config":[{"BusinessName":"z1","installpath":"", 
                                 "clientport":"", "role":"", ... }, ...]
                   }
                    , ... 
                ]
   }
   */
   INT32 omZooConfigGenerator::checkConfig( BSONObj &newBusinessConf,
                                            const BSONObj &confProperties, 
                                            const BSONObj &bsonHostInfo )
   {
      _cluster.clear() ;
      _propertyContainer.clear() ;
      _template.reset() ;

      INT32 rc = SDB_OK ;
      rc = _parseProperties( confProperties ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "parse confProperties failed:rc=%d", rc ) ;
         goto error ;
      }

      rc = _parseCluster( bsonHostInfo ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "_parseCluster failed:rc=%d", rc ) ;
         goto error ;
      }

      // parse business
      rc = _parseNewBusiness( newBusinessConf ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "_parseNewBusiness failed:rc=%d", rc ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      _errorDetail = omGetMyEDUInfoSafe( EDU_INFO_ERROR ) ;
      goto done ;
   }

   /*
   newBusinessConf:
   {
      "BusinessType":"zookeeper", "BusinessName":"z1", "DeployMod":"xx", 
      "ClusterName":"c1", 
      "Config":
      [
         {"HostName": "host1", "datagroupname": "", 
          "installpath": "/opt/zookeeper/z1", "clientport": "2888", ...}
         ,...
      ]
   }
   */
   INT32 omZooConfigGenerator::_parseNewBusiness( const BSONObj &newBusinessConf )
   {
      INT32 rc = SDB_OK ;
      rc = _businessConf.init( &_propertyContainer, newBusinessConf ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "init business configure failed:rc=%d", rc ) ;
         goto error ;
      }

      {
         list<omZooNodeConf> nodeList ;
         _businessConf.getNodeList( nodeList ) ;
         list<omZooNodeConf>::iterator iter = nodeList.begin() ;
         while ( iter != nodeList.end() )
         {
            omZooNodeConf *pNodeConf = &( *iter ) ;
            rc = _cluster.checkAndAddNode( _businessConf.getBusinessName(), 
                                           pNodeConf ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "check node failed:rc=%d", rc ) ;
               goto error ;
            }
            iter++ ;
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   //*********************Zookeeper end**************************************
}


