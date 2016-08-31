(function(){
   var sacApp = window.SdbSacManagerModule ;
   //控制器
   sacApp.controllerProvider.register( 'Deploy.Index.Ctrl', function( $scope, $compile, $location, $rootScope, SdbFunction, SdbRest, Loading ){

      var defaultShow = $rootScope.tempData( 'Deploy', 'Index' ) ;

      //初始化
      $scope.EditHostGridOptions = { 'titleWidth': [ '200px', '200px', 100 ] } ;
      //集群列表
      $scope.clusterList = [] ;
      //默认选的cluster
      $scope.currentCluster = 0 ;
      //默认显示业务还是主机列表
      $scope.currentPage = 'module' ;
      //业务列表
      $scope.moduleList = [] ;
      //主机列表
      $scope.HostList = [] ;
      //业务类型列表
      $scope.moduleType = [] ;
      //业务列表数量
      $scope.ModuleNum = 0 ;
      //主机列表数量
      $scope.HostNum = 0 ;
      //选择主机的网格选项
      $scope.HostGridOptions = { 'titleWidth': [ '30px', '60px', 30, 30, 40 ] } ;
      //主机和业务的关联表(也就是有安装业务的主机列表)
      var host_module_table = [] ;
      //循环查询的业务
      var autoQueryModuleIndex = [] ;

      //清空Deploy域的数据
      $rootScope.tempData( 'Deploy' ) ;

      //计算每个业务的资源
      var countModule_Host = function(){
         $.each( $scope.moduleList, function( index, moduleInfo ){
            if( isArray( moduleInfo['Location'] ) )
            {
               var cpu = 0 ;
               var memory = 0 ;
               var disk = 0 ;
               var length = 0 ;
               $.each( moduleInfo['Location'], function( index2, hostInfo ){
                  var index3 = hostModuleTableIsExist( hostInfo['HostName'] ) ;
                  if( index3 >= 0 )
                  {
                     ++length ;
                     cpu += host_module_table[index3]['Info']['CPU'] ;
                     memory += host_module_table[index3]['Info']['Memory'] ;
                     disk += host_module_table[index3]['Info']['Disk'] ;
                     if( $scope.moduleList[index]['Error']['Type'] == 'Host' || $scope.moduleList[index]['Error']['Flag'] == 0 )
                     {
                        if( host_module_table[index3]['Error']['Flag'] == 0 )
                        {
                           $scope.moduleList[index]['Error']['Flag'] = 0 ;
                        }
                        else
                        {
                           $scope.moduleList[index]['Error']['Flag'] = host_module_table[index3]['Error']['Flag'] ;
                           $scope.moduleList[index]['Error']['Type'] = 'Host' ;
                           $scope.moduleList[index]['Error']['Message'] = sprintf( $scope.autoLanguage( '主机 ? 状态异常: ?。' ), 
                                                                                   host_module_table[index3]['HostName'],
                                                                                   host_module_table[index3]['Error']['Message'] ) ;
                        }
                     }
                  }
               } ) ;
               $scope.moduleList[index]['Chart']['Host']['CPU'] = { 'percent': fixedNumber( cpu / length, 2 ), 'style': { 'progress': { 'background': '#87CEFA' } } } ;
               $scope.moduleList[index]['Chart']['Host']['Memory'] = { 'percent': fixedNumber( memory / length, 2 ), 'style': { 'progress': { 'background': '#DDA0DD' } } } ;
               $scope.moduleList[index]['Chart']['Host']['Disk'] = { 'percent':  fixedNumber( disk / length, 2 ), 'style': { 'progress': { 'background': '#FFA07A' } } } ;
            }
         } ) ;
      }
      
      //host_module_table是否已经存在该主机
      var hostModuleTableIsExist = function( hostName ){
         var flag = -1 ;
         $.each( host_module_table, function( index, hostInfo ){
            if( hostInfo['HostName'] == hostName )
            {
               flag = index ;
               return false ;
            }
         } ) ;
         return flag ;
      }

      //$scope.HostList是否存在该主机
      var hostListIsExist = function( hostName ){
         var flag = -1 ;
         $.each( $scope.HostList, function( index, hostInfo ){
            if( hostInfo['HostName'] == hostName )
            {
               flag = index ;
               return false ;
            }
         } ) ;
         return flag ;
      }

      var isFirstQueryHostStatus = true ;
      //查询主机状态
      var queryHostStatus = function(){
         var isFirst = false ;
         var queryHostList = { 'HostInfo': [] } ;
         if( $scope.HostList.length == 0 )
         {
            SdbFunction.Timeout( queryHostStatus, 5000 ) ;
            return ;
         }
         $.each( $scope.HostList, function( index, hostInfo ){
            if( isFirstQueryHostStatus || hostInfo['ClusterName'] == $scope.clusterList[ $scope.currentCluster ]['ClusterName'] )
            {
               queryHostList['HostInfo'].push( { 'HostName': hostInfo['HostName'] } ) ;
            }
         } ) ;
         isFirstQueryHostStatus = false ;
         if( queryHostList['HostInfo'].length == 0 )
         {
            SdbFunction.Timeout( queryHostStatus, 5000 ) ;
            return ;
         }
         var data = { 'cmd': 'query host status', 'HostInfo': JSON.stringify( queryHostList ) } ;
         SdbRest.OmOperation( data, function( hostStatusList ){
            $.each( hostStatusList[0]['HostInfo'], function( index, statusInfo ){
               var index2 = hostModuleTableIsExist( statusInfo['HostName'] ) ;
               if( index2 >= 0 )
               {
                  if( statusInfo['errno'] == 0 || typeof( statusInfo['errno'] ) == 'undefined' )
                  {
                     if( typeof( host_module_table[index2]['CPU'] ) == 'object' )
                     {
                        var resource = host_module_table[index2] ;
                        var old_idle1   = resource['CPU']['Idle']['Megabit'] ;
                        var old_idle2   = resource['CPU']['Idle']['Unit'] ;
                        var old_cpuSum1 = resource['CPU']['Idle']['Megabit'] +
                                          resource['CPU']['Other']['Megabit'] +
                                          resource['CPU']['Sys']['Megabit'] +
                                          resource['CPU']['User']['Megabit'] ;
                        var old_cpuSum2 = resource['CPU']['Idle']['Unit'] +
                                          resource['CPU']['Other']['Unit'] +
                                          resource['CPU']['Sys']['Unit'] +
                                          resource['CPU']['User']['Unit'] ;
                        var idle1   = statusInfo['CPU']['Idle']['Megabit'] ;
                        var idle2   = statusInfo['CPU']['Idle']['Unit'] ;
                        var cpuSum1 = statusInfo['CPU']['Idle']['Megabit'] +
                                      statusInfo['CPU']['Other']['Megabit'] +
                                      statusInfo['CPU']['Sys']['Megabit'] +
                                      statusInfo['CPU']['User']['Megabit'] ;
                        var cpuSum2 = statusInfo['CPU']['Idle']['Unit'] +
                                      statusInfo['CPU']['Other']['Unit'] +
                                      statusInfo['CPU']['Sys']['Unit'] +
                                      statusInfo['CPU']['User']['Unit'] ;
                        host_module_table[index2]['Info']['CPU'] = ( ( 1 - ( ( idle1 - old_idle1 ) * 1024 + ( idle2 - old_idle2 ) / 1024 ) / ( ( cpuSum1 - old_cpuSum1 ) * 1024 + ( cpuSum2 - old_cpuSum2 ) / 1024 ) ) * 100 ) ;
                     }
                     else
                     {
                        isFirst = true ;
                        host_module_table[index2]['Info']['CPU'] = 0 ;
                     }
                     host_module_table[index2]['CPU'] = statusInfo['CPU'] ;
                     var diskFree = 0 ;
                     var diskSize = 0 ;
                     $.each( statusInfo['Disk'], function( index2, diskInfo ){
                        diskFree += diskInfo['Free'] ;
                        diskSize += diskInfo['Size'] ;
                     } ) ;
                     if( diskSize == 0 )
                     {
                        host_module_table[index2]['Info']['Disk'] = 0 ;
                     }
                     else
                     {
                        host_module_table[index2]['Info']['Disk'] = ( 1 - diskFree / diskSize ) * 100 ;
                     }
                     host_module_table[index2]['Info']['Memory'] = statusInfo['Memory']['Used'] / statusInfo['Memory']['Size'] * 100 ;
                     host_module_table[index2]['Error']['Flag'] = 0 ;
                     var index3 = hostListIsExist( statusInfo['HostName'] ) ;
                     if( index3 >= 0 )
                     {
                        $scope.HostList[index3]['Error']['Flag'] = 0 ;
                     }
                  }
                  else
                  {
                     host_module_table[index2]['Info']['CPU'] = 0 ;
                     host_module_table[index2]['Info']['Disk'] = 0 ;
                     host_module_table[index2]['Info']['Memory'] = 0 ;
                     host_module_table[index2]['Error']['Flag'] = statusInfo['errno'] ;
                     host_module_table[index2]['Error']['Message'] = statusInfo['detail'] ;
                     var index3 = hostListIsExist( statusInfo['HostName'] ) ;
                     if( index3 >= 0 )
                     {
                        $scope.HostList[index3]['Error']['Flag'] = statusInfo['errno'] ;
                        $scope.HostList[index3]['Error']['Message'] = statusInfo['detail'] ;
                     }
                  }
               }
               else
               {
                  if( statusInfo['errno'] == 0 || typeof( statusInfo['errno'] ) == 'undefined' )
                  {
                     var index3 = hostListIsExist( statusInfo['HostName'] ) ;
                     if( index3 >= 0 )
                     {
                        $scope.HostList[index3]['Error']['Flag'] = 0 ;
                     }
                  }
                  else
                  {
                     var index3 = hostListIsExist( statusInfo['HostName'] ) ;
                     if( index3 >= 0 )
                     {
                        $scope.HostList[index3]['Error']['Flag'] = statusInfo['errno'] ;
                        $scope.HostList[index3]['Error']['Message'] = statusInfo['detail'] ;
                     }
                  }
               }
            } ) ;
            countModule_Host() ;
            SdbFunction.Timeout( queryHostStatus, isFirst ? 2000 : 5000 ) ;
         }, function( errorInfo ){
            SdbFunction.Timeout( queryHostStatus, isFirst ? 2000 : 5000 ) ;
         }, function(){
            _IndexPublic.createErrorModel( $scope, $scope.autoLanguage( '网络连接错误，请尝试按F5刷新浏览器。' ) ) ;
         }, null, false ) ;
      }

      //获取sequoiadb的节点信息
      var getNodesList = function( moduleIndex ){
         if( $.inArray( moduleIndex, autoQueryModuleIndex ) == -1 )
         {
            return ;
         }
         $scope.moduleList[moduleIndex]['BusinessInfo'] = {} ;
         var moduleName = $scope.moduleList[moduleIndex]['BusinessName'] ;
         var data = { 'cmd': 'list nodes', 'BusinessName': moduleName } ;
         SdbRest.OmOperation( data, function( nodeList ){
           $scope.moduleList[moduleIndex]['BusinessInfo']['NodeList'] = nodeList ;
         }, function( errorInfo ){
            _IndexPublic.createRetryModel( $scope, errorInfo, function(){
               getNodesList( moduleIndex ) ;
               return true ;
            } ) ;
         }, function(){
            _IndexPublic.createErrorModel( $scope, $scope.autoLanguage( '网络连接错误，请尝试按F5刷新浏览器。' ) ) ;
         }, null, false ) ;
      } ;

      //获取sequoiadb业务信息
      var getCollectionInfo = function( moduleIndex ){
         if( $.inArray( moduleIndex, autoQueryModuleIndex ) == -1 )
         {
            return ;
         }
         var clusterName = $scope.moduleList[moduleIndex]['ClusterName'] ;
         var moduleName = $scope.moduleList[moduleIndex]['BusinessName'] ;
         var moduleMode = $scope.moduleList[moduleIndex]['DeployMod'] ;
         var sql ;
         if( moduleMode == 'standalone' )
         {
            sql = 'SELECT t1.Name, t1.Details.TotalIndexPages, t1.Details.PageSize, t1.Details.TotalDataPages, t1.Details.LobPageSize, t1.Details.TotalLobPages FROM (SELECT * FROM $SNAPSHOT_CL split BY Details) AS t1' ;
         }
         else
         {
            sql = 'SELECT t1.Name, t1.Details.TotalIndexPages, t1.Details.PageSize, t1.Details.TotalDataPages, t1.Details.LobPageSize, t1.Details.TotalLobPages FROM (SELECT * FROM $SNAPSHOT_CL WHERE NodeSelect="master" split BY Details) AS t1' ;
         }
         SdbRest.Exec2( clusterName, moduleName, sql, function( clList ){
            var index = 0 ;
            var data = 0 ;
            var lob = 0 ;
            var indexPercent = 0 ;
            var dataPercent = 0 ;
            var lobPercent = 0 ;
            $.each( clList, function( clIndex, clInfo ){
               index += clInfo['PageSize'] * clInfo['TotalIndexPages'] ;
               data += clInfo['PageSize'] * clInfo['TotalDataPages'] ;
               lob += clInfo['LobPageSize'] * clInfo['TotalLobPages'] ;
            } ) ;
            var sum = index + data + lob ;
            var indexPercent = fixedNumber( index / sum * 100, 2 ) ;
            var dataPercent  = fixedNumber( data / sum * 100, 2 ) ;
            var lobPercent   = 100 - indexPercent - dataPercent ;
            if( isNaN( indexPercent ) || index == 0 )
            {
               indexPercent = 0 ;
            }
            if( isNaN( dataPercent ) || data == 0 )
            {
               dataPercent = 0 ;
            }
            if( isNaN( lobPercent ) || lob == 0 )
            {
               lobPercent = 0 ;
            }
            $scope.moduleList[ moduleIndex ]['Chart']['Module']['value'] = [
               [ 0, indexPercent, true, false ],
               [ 1, dataPercent, true, false ],
               [ 2, lobPercent, true, false ]
            ] ;

            if( $scope.moduleList[moduleIndex]['Error']['Type'] == 'Module cl' )
            {
               $scope.moduleList[moduleIndex]['Error']['Flag'] = 0 ;
            }

            SdbFunction.Timeout( function(){
               getCollectionInfo( moduleIndex ) ;
            }, 5000 ) ;
         }, function( errorInfo ){
            if( moduleMode == 'standalone' )
            {
               if( $scope.moduleList[moduleIndex]['Error']['Type'] == 'Module cl' || $scope.moduleList[moduleIndex]['Error']['Flag'] == 0 )
               {
                  $scope.moduleList[moduleIndex]['Error']['Flag'] = errorInfo['errno'] ;
                  $scope.moduleList[moduleIndex]['Error']['Type'] = 'Module cl' ;
                  $scope.moduleList[moduleIndex]['Error']['Message'] = sprintf( $scope.autoLanguage( '节点错误: ?，错误码 ?。' ),
                                                                                errorInfo['description'],
                                                                                errorInfo['errno'] ) ;
               }
            }
            SdbFunction.Timeout( function(){
               getCollectionInfo( moduleIndex ) ;
            }, 5000 ) ;
         }, function(){
            _IndexPublic.createErrorModel( $scope, $scope.autoLanguage( '网络连接错误，请尝试按F5刷新浏览器。' ) ) ;
         }, null, false ) ;
      }

      //获取sequoiadb的错误节点信息
      var getErrNodes = function( moduleIndex ){
         if( $.inArray( moduleIndex, autoQueryModuleIndex ) == -1 )
         {
            return ;
         }
         var moduleMode = $scope.moduleList[moduleIndex]['DeployMod'] ;
         if( moduleMode == 'standalone' )
         {
            return ;
         }
         var clusterName = $scope.moduleList[moduleIndex]['ClusterName'] ;
         var moduleName = $scope.moduleList[moduleIndex]['BusinessName'] ;
         var data = { 'cmd': 'snapshot system', 'selector': JSON.stringify( { 'ErrNodes': 1 } ) } ;
         SdbRest.DataOperation2( clusterName, moduleName, data, function( errNodes ){
            errNodes = errNodes[0]['ErrNodes'] ;
            if( $scope.moduleList[moduleIndex]['Error']['Type'] == 'Module node' || $scope.moduleList[moduleIndex]['Error']['Flag'] == 0 )
            {
               if( errNodes.length > 0 )
               {
                  $scope.moduleList[moduleIndex]['Error']['Flag'] = errNodes[0]['Flag'] ;
                  $scope.moduleList[moduleIndex]['Error']['Type'] = 'Module node' ;
                  $scope.moduleList[moduleIndex]['Error']['Message'] = sprintf( $scope.autoLanguage( '节点错误: ?，错误码 ?。' ),
                                                                                errNodes[0]['NodeName'],
                                                                                errNodes[0]['Flag'] ) ;
               }
               else if( errNodes.length == 0 )
               {
                  $scope.moduleList[moduleIndex]['Error']['Flag'] = 0 ;
               }
            }
            SdbFunction.Timeout( function(){
               getErrNodes( moduleIndex ) ;
            }, 5000 ) ;
         }, function( errorInfo ){
            SdbFunction.Timeout( function(){
               getErrNodes( moduleIndex ) ;
            }, 5000 ) ;
         }, function(){
            _IndexPublic.createErrorModel( $scope, $scope.autoLanguage( '网络连接错误，请尝试按F5刷新浏览器。' ) ) ;
         }, null, null, false ) ;
      }

      //查询业务
      var queryModule = function(){
         var data = { 'cmd': 'query business' } ;
         var clusterList = [] ;
         var clusterIsExist = function( clusterName ){
            var flag = 0 ;
            var isFind = false ;
            $.each( clusterList, function( index, clusterInfo ){
               if( clusterInfo['ClusterName'] == clusterName )
               {
                  isFind = true ;
                  flag = clusterInfo['index'] ;
                  ++clusterList[index]['index'] ;
                  return false ;
               }
            } ) ;
            if( isFind == false )
            {
               clusterList.push( { 'ClusterName': clusterName, 'index': 1 } ) ;
            }
            return flag ;
         }
         SdbRest.OmOperation( data, function( moduleList ){
            $scope.moduleList = moduleList ;
            host_module_table = [] ;
            autoQueryModuleIndex = [] ;
            $.each( $scope.moduleList, function( index, moduleInfo ){

               var colorId = clusterIsExist( moduleInfo['ClusterName'] ) ;

               $scope.moduleList[index]['Color'] = colorId ;

               $scope.moduleList[index]['Error'] = {} ;
               $scope.moduleList[index]['Error']['Flag'] = 0 ;
               $scope.moduleList[index]['Error']['Type'] = '' ;
               $scope.moduleList[index]['Error']['Message'] = '' ;

               $scope.moduleList[index]['Chart'] = {} ;
               $scope.moduleList[index]['Chart']['Module'] = {} ;
               $scope.moduleList[index]['Chart']['Module']['options'] = $.extend( true, {}, window.SdbSacManagerConf.StorageScaleEchart ) ;
               $scope.moduleList[index]['Chart']['Module']['options']['title']['text'] = $scope.autoLanguage( '元数据比例' ) ;

               $scope.moduleList[index]['Chart']['Host'] = {} ;
               $scope.moduleList[index]['Chart']['Host']['CPU'] = { 'percent': 0 } ;
               $scope.moduleList[index]['Chart']['Memory'] = { 'percent': 0 } ;
               $scope.moduleList[index]['Chart']['Disk'] = { 'percent': 0 } ;
               if( isArray( moduleInfo['Location'] ) )
               {
                  $.each( moduleInfo['Location'], function( index2, hostInfo ){
                     if( hostModuleTableIsExist( hostInfo['HostName'] ) == -1 )
                     {
                        host_module_table.push( { 'HostName': hostInfo['HostName'], 'Info': {}, 'Error': {} } ) ;
                     }
                  } ) ;
               }
               if( moduleInfo['BusinessType'] == 'sequoiadb' && moduleInfo['ClusterName'] == $scope.clusterList[ $scope.currentCluster ]['ClusterName'] )
               {
                  autoQueryModuleIndex.push( index ) ;
                  getNodesList( index ) ;
                  getCollectionInfo( index ) ;
                  getErrNodes( index ) ;
               }
            } ) ;
            $scope.SwitchCluster( $scope.currentCluster ) ;
            queryHostStatus() ;
         }, function( errorInfo ){
            _IndexPublic.createRetryModel( $scope, errorInfo, function(){
               queryModule() ;
               return true ;
            } ) ;
         }, function(){
            _IndexPublic.createErrorModel( $scope, $scope.autoLanguage( '网络连接错误，请尝试按F5刷新浏览器。' ) ) ;
         } ) ;
      }

      //查询主机
      var queryHost = function(){
         var data = { 'cmd': 'query host' } ;
         SdbRest.OmOperation( data, function( hostList ){
            $scope.HostList = hostList ;
            $.each( $scope.HostList, function( index ){
               $scope.HostList[index]['Error'] = {} ;
               $scope.HostList[index]['Error']['Flag'] = 0 ;
            } ) ;
            $scope.SwitchCluster( $scope.currentCluster ) ;
            if( defaultShow == 'host' )
            {
               $scope.SwitchPage( defaultShow ) ;
            }
         }, function( errorInfo ){
            _IndexPublic.createRetryModel( $scope, errorInfo, function(){
               queryHost() ;
               return true ;
            } ) ;
         }, function(){
            _IndexPublic.createErrorModel( $scope, $scope.autoLanguage( '网络连接错误，请尝试按F5刷新浏览器。' ) ) ;
         } ) ;
      }

      //查询集群
      var queryCluster = function(){
         var data = { 'cmd': 'query cluster' } ;
         SdbRest.OmOperation( data, function( clusterList ){
            $scope.clusterList = clusterList ;
            if( $scope.clusterList.length > 0 )
            {
               queryHost() ;
               queryModule() ;
            }
         }, function( errorInfo ){
            _IndexPublic.createRetryModel( $scope, errorInfo, function(){
               queryCluster() ;
               return true ;
            } ) ;
         }, function(){
            _IndexPublic.createErrorModel( $scope, $scope.autoLanguage( '网络连接错误，请尝试按F5刷新浏览器。' ) ) ;
         } ) ;
      }

      //跳转到监控业务
      $scope.GotoMonitorModule = function( clusterName, moduleType, moduleMode, moduleName ){
         SdbFunction.LocalData( 'SdbClusterName', clusterName ) ;
         SdbFunction.LocalData( 'SdbModuleType', moduleType ) ;
         SdbFunction.LocalData( 'SdbModuleMode', moduleMode ) ;
         SdbFunction.LocalData( 'SdbModuleName', moduleName ) ;
         switch( moduleType )
         {
         case 'sequoiadb':
            $location.path( '/Monitor/Index' ) ; break ;
         default:
            break ;
         }
      }

      //跳转到监控主机
      $scope.GotoMonitorHost = function( clusterName, moduleType, moduleMode, moduleName ){
         SdbFunction.LocalData( 'SdbClusterName', clusterName ) ;
         SdbFunction.LocalData( 'SdbModuleType', moduleType ) ;
         SdbFunction.LocalData( 'SdbModuleMode', moduleMode ) ;
         SdbFunction.LocalData( 'SdbModuleName', moduleName ) ;
         switch( moduleType )
         {
         case 'sequoiadb':
            $location.path( '/Monitor/Host-List/Index' ) ; break ;
         default:
            break ;
         }
      }

      //跳转到业务数据
      $scope.GotoDataModule = function( clusterName, moduleType, moduleMode, moduleName ){
         SdbFunction.LocalData( 'SdbClusterName', clusterName ) ;
         SdbFunction.LocalData( 'SdbModuleType', moduleType ) ;
         SdbFunction.LocalData( 'SdbModuleMode', moduleMode ) ;
         SdbFunction.LocalData( 'SdbModuleName', moduleName ) ;
         switch( moduleType )
         {
         case 'sequoiadb':
            $location.path( '/Data/SDB-Database/Index' ) ; break ;
         case 'sequoiasql':
            $location.path( '/Data/SQL-Metadata/Index' ) ; break ;
         case 'hdfs':
            $location.path( '/Data/HDFS-web/Index' ) ; break ;
         case 'spark':
            $location.path( '/Data/SPARK-web/Index' ) ; break ;
         case 'yarn':
            $location.path( '/Data/YARN-web/Index' ) ; break ;
         default:
            break ;
         }
      }

      //切换业务和主机
      $scope.SwitchPage = function( page ){
         $scope.currentPage = page ;
         $scope.bindResize() ;
      }

      //切换集群
      $scope.SwitchCluster = function( index ){
         $scope.currentCluster = index ;
         if( $scope.clusterList.length > 0 )
         {
            var clusterName = $scope.clusterList[ $scope.currentCluster ]['ClusterName'] ;
            $scope.ModuleNum = 0 ;
            autoQueryModuleIndex = [] ;
            $.each( $scope.moduleList, function( index, moduleInfo ){
               if( moduleInfo['ClusterName'] == clusterName )
               {
                  ++$scope.ModuleNum ;
                  autoQueryModuleIndex.push( index ) ;
                  if( moduleInfo['BusinessType'] == 'sequoiadb' )
                  {
                     getNodesList( index ) ;
                     getCollectionInfo( index ) ;
                     getErrNodes( index ) ;
                  }
               }
            } ) ;
            $scope.HostNum = 0 ;
            $.each( $scope.HostList, function( index, hostInfo ){
               if( hostInfo['ClusterName'] == clusterName )
               {
                  ++$scope.HostNum ;
               }
            } ) ;
         }
         $scope.bindResize() ;
      }

      //获取业务类型列表
      var GetModuleType = function(){
         var data = { 'cmd': 'list business type' } ;
         SdbRest.OmOperation( data, function( moduleType ){
            $scope.moduleType = moduleType ;
         }, function( errorInfo ){
            _IndexPublic.createRetryModel( $scope, errorInfo, function(){
               GetModuleType() ;
               return true ;
            } ) ;
         }, function(){
            _IndexPublic.createErrorModel( $scope, $scope.autoLanguage( '网络连接错误，请尝试按F5刷新浏览器。' ) ) ;
         } ) ;
      }

      //全选
      $scope.SelectAll = function(){
         $.each( $scope.HostList, function( index ){
            if( $scope.clusterList[ $scope.currentCluster ]['ClusterName'] == $scope.HostList[index]['ClusterName'] )
            {
               $scope.HostList[index]['checked'] = true ;
            }
         } ) ;
      }

      //反选
      $scope.Unselect = function(){
         $.each( $scope.HostList, function( index ){
            if( $scope.clusterList[ $scope.currentCluster ]['ClusterName'] == $scope.HostList[index]['ClusterName'] )
            {
               $scope.HostList[index]['checked'] = !$scope.HostList[index]['checked'] ;
            }
         } ) ;
      }

      //添加主机
      $scope.AddHost = function(){
         if( $scope.clusterList.length > 0 )
         {
            $rootScope.tempData( 'Deploy', 'Model', 'Host' ) ;
            $rootScope.tempData( 'Deploy', 'Module', 'None' ) ;
            $rootScope.tempData( 'Deploy', 'ClusterName', $scope.clusterList[ $scope.currentCluster ]['ClusterName'] ) ;
            $rootScope.tempData( 'Deploy', 'InstallPath', $scope.clusterList[ $scope.currentCluster ]['InstallPath'] ) ;
            $location.path( '/Deploy/ScanHost' ) ;
         }
      }

      //删除主机
      $scope.RemoveHost = function(){
         if( $scope.clusterList.length > 0 )
         {
            var hostList = [] ;
            $.each( $scope.HostList, function( index ){
               if( $scope.HostList[index]['checked'] == true && $scope.clusterList[ $scope.currentCluster ]['ClusterName'] == $scope.HostList[index]['ClusterName'] )
               {
                  hostList.push( { 'HostName': $scope.HostList[index]['HostName'] } ) ;
               }
            } ) ;
            if( hostList.length > 0 )
            {
               var data = { 'cmd': 'remove host', 'HostInfo': JSON.stringify( { 'HostInfo': hostList } ) } ;
               SdbRest.OmOperation( data, function( taskInfo ){
                  $rootScope.tempData( 'Deploy', 'Model', 'Task' ) ;
                  $rootScope.tempData( 'Deploy', 'Module', 'None' ) ;
                  $rootScope.tempData( 'Deploy', 'HostTaskID', taskInfo[0]['TaskID'] ) ;
                  $location.path( '/Deploy/InstallHost' ) ;
               }, function( errorInfo ){
                  _IndexPublic.createRetryModel( $scope, errorInfo, function(){
                     $scope.RemoveHost() ;
                     return true ;
                  } ) ;
               }, function(){
                  _IndexPublic.createErrorModel( $scope, $scope.autoLanguage( '网络连接错误，请尝试按F5刷新浏览器。' ) ) ;
               } ) ;
            }
            else
            {
               $scope.Components.Confirm.type = 3 ;
               $scope.Components.Confirm.context = $scope.autoLanguage( '至少选择一台的主机。' ) ;
               $scope.Components.Confirm.isShow = true ;
               $scope.Components.Confirm.okText = $scope.autoLanguage( '好的' ) ;
            }
         }
      }

      //创建 添加业务 弹窗
      $scope.CreateInstallModuleModel = function(){
         if( $scope.clusterList.length > 0 )
         {
            if( $scope.HostNum == 0 )
            {
               $scope.Components.Confirm.type = 3 ;
               $scope.Components.Confirm.context = $scope.autoLanguage( '集群还没有安装主机。' ) ;
               $scope.Components.Confirm.isShow = true ;
               $scope.Components.Confirm.okText = $scope.autoLanguage( '好的' ) ;
               return ;
            }
            $scope.Components.Modal.icon = '' ;
            $scope.Components.Modal.title = $scope.autoLanguage( '添加业务' ) ;
            $scope.Components.Modal.isShow = true ;
            $scope.Components.Modal.form = {
               'inputList': [
                  {
                     "name": 'moduleName',
                     "webName": $scope.autoLanguage( '业务名' ),
                     "type": "string",
                     "required": true,
                     "value": "",
                     "valid": {
                        "min": 1,
                        "max": 127,
                        "regex": '^[0-9a-zA-Z]+$'
                     }
                  },
                  {
                     "name": 'moduleType',
                     "webName": $scope.autoLanguage( '业务类型' ),
                     "type": "select",
                     "value": 0,
                     "valid": []
                  }
               ]
            } ;
            var num = 1 ;
            var defaultName = '' ;
            while( true )
            {
               var isFind = false ;
               defaultName = sprintf( 'myModule?', num ) ;
               $.each( $scope.moduleList, function( index, moduleInfo ){
                  if( defaultName == moduleInfo['BusinessName'] )
                  {
                     isFind = true ;
                     return false ;
                  }
               } ) ;
               if( isFind == false )
               {
                  $.each( $rootScope.OmTaskList, function( index, taskInfo ){
                     if( taskInfo['Status'] != 4 && defaultName == taskInfo['Info']['BusinessName'] )
                     {
                        isFind = true ;
                        return false ;
                     }
                  } ) ;
                  if( isFind == false )
                  {
                     break ;
                  }
               }
               ++num ;
            }
            $scope.Components.Modal.form['inputList'][0]['value'] = defaultName ;
            $.each( $scope.moduleType, function( index, typeInfo ){
               $scope.Components.Modal.form['inputList'][1]['valid'].push( { 'key': typeInfo['BusinessDesc'], 'value': index } ) ;
            } ) ;
            $scope.Components.Modal.Context = '<div form-create para="data.form"></div>' ;
            $scope.Components.Modal.ok = function(){
               var isAllClear = $scope.Components.Modal.form.check( function( formVal ){
                  var isFind = false ;
                  $.each( $scope.moduleList, function( index, moduleInfo ){
                     if( formVal['moduleName'] == moduleInfo['BusinessName'] )
                     {
                        isFind = true ;
                        return false ;
                     }
                  } ) ;
                  if( isFind == true )
                  {
                     return [ { 'name': 'moduleName', 'error': $scope.autoLanguage( '业务名已经存在' ) } ]
                  }
                  else
                  {
                     return [] ;
                  }
               } ) ;
               if( isAllClear )
               {
                  var formVal = $scope.Components.Modal.form.getValue() ;
                  $rootScope.tempData( 'Deploy', 'Model', 'Module' ) ;
                  $rootScope.tempData( 'Deploy', 'Module', $scope.moduleType[ formVal['moduleType'] ]['BusinessType'] ) ;
                  $rootScope.tempData( 'Deploy', 'ModuleName', formVal['moduleName'] ) ;
                  $rootScope.tempData( 'Deploy', 'ClusterName', $scope.clusterList[ $scope.currentCluster ]['ClusterName'] ) ;
                  if( $scope.moduleType[ formVal['moduleType'] ]['BusinessType'] == 'sequoiadb' )
                  {
                     $location.path( '/Deploy/SDB-Conf' ) ;
                  }
                  else if( $scope.moduleType[ formVal['moduleType'] ]['BusinessType'] == 'sequoiasql' )
                  {
                     /*
                     var tempHostInfo = [] ;
			            $.each( $scope.HostList, function( index, value ){
                        if( $scope.clusterList[$scope.currentCluster]['ClusterName'] == value['ClusterName'] )
                        {
				               tempHostInfo.push( { 'HostName': value['HostName'] } ) ;
                        }
			            } ) ;
                     var businessConf = {} ;
                     businessConf['ClusterName']  = $scope.clusterList[ $scope.currentCluster ]['ClusterName'] ;
                     businessConf['BusinessName'] = formVal['moduleName'] ;
                     businessConf['BusinessType'] = $scope.moduleType[ formVal['moduleType'] ]['BusinessType'] ;
                     businessConf['DeployMod'] = 'olap' ;
                     businessConf['Property'] = [
                        { "Name": "deploy_standby", "Value": "false" },
                        { "Name": "segment_num", "Value": tempHostInfo.length + '' }
                     ] ;
                     businessConf['HostInfo'] = tempHostInfo ;
                     $rootScope.tempData( 'Deploy', 'ModuleConfig', businessConf ) ;
                     $location.path( '/Deploy/SSQL-Mod' ) ;
                     */
                     $location.path( '/Deploy/SSQL-Conf' ) ;
                  }
                  else if( $scope.moduleType[ formVal['moduleType'] ]['BusinessType'] == 'zookeeper' )
                  {
                     var tempHostInfo = [] ;
			            $.each( $scope.HostList, function( index, value ){
                        if( $scope.clusterList[$scope.currentCluster]['ClusterName'] == value['ClusterName'] )
                        {
				               tempHostInfo.push( { 'HostName': value['HostName'] } ) ;
                        }
			            } ) ;
                     var businessConf = {} ;
                     businessConf['ClusterName']  = $scope.clusterList[ $scope.currentCluster ]['ClusterName'] ;
                     businessConf['BusinessName'] = formVal['moduleName'] ;
                     businessConf['BusinessType'] = $scope.moduleType[ formVal['moduleType'] ]['BusinessType'] ;
                     businessConf['DeployMod'] = 'distribution' ;
                     businessConf['Property'] = [ { 'Name': 'zoonodenum', 'Value': '3' } ] ;
                     businessConf['HostInfo'] = tempHostInfo ;
                     $rootScope.tempData( 'Deploy', 'ModuleConfig', businessConf ) ;
                     $location.path( '/Deploy/ZKP-Mod' ) ;
                  }
               }
               return isAllClear ;
            }
         }
      }

      //发现业务
      var discoverModule = function( configure ){
         var data = { 'cmd': 'discover business', 'ConfigInfo': JSON.stringify( configure ) } ;
         SdbRest.OmOperation( data, function(){
            queryCluster() ;
         }, function( errorInfo ){
            _IndexPublic.createRetryModel( $scope, errorInfo, function(){
               discoverModule( configure ) ;
               return true ;
            } ) ;
         }, function(){
            _IndexPublic.createErrorModel( $scope, $scope.autoLanguage( '网络连接错误，请尝试按F5刷新浏览器。' ) ) ;
         } ) ;
      }

      //创建 发现sequoiasql 弹窗
      $scope.CreateAppendSSQLModel = function( moduleName ){
         $scope.Components.Modal.icon = '' ;
         $scope.Components.Modal.title = 'SequoiaSQL' ;
         $scope.Components.Modal.isShow = true ;
         $scope.Components.Modal.form = {
            'inputList': [
               {
                  "name": 'HostName',
                  "webName": $scope.autoLanguage( '主机名' ),
                  "type": "string",
                  "required": true,
                  "value": "",
                  "valid": {
                     "min": 1
                  }
               },
               {
                  "name": 'ServiceName',
                  "webName": $scope.autoLanguage( '服务名' ),
                  "type": "port",
                  "required": true,
                  "value": '',
                  "valid": {}
               },
               {
                  "name": 'InstallPath',
                  "webName": $scope.autoLanguage( '安装路径' ),
                  "type": "string",
                  "required": true,
                  "value": "",
                  "valid": {
                     "min": 1
                  }
               },
               {
                  "name": 'DbName',
                  "webName": $scope.autoLanguage( '数据库名' ),
                  "type": "string",
                  "required": true,
                  "value": "",
                  "valid": {
                     "min": 1
                  }
               },
               {
                  "name": 'User',
                  "webName": $scope.autoLanguage( '数据库用户名' ),
                  "type": "string",
                  "required": true,
                  "value": "",
                  "valid": {
                     "min": 1
                  }
               },
               {
                  "name": 'Passwd',
                  "webName": $scope.autoLanguage( '数据库密码' ),
                  "type": "string",
                  "required": true,
                  "value": "",
                  "valid": {
                     "min": 1
                  }
               }
            ]
         } ;
         $scope.Components.Modal.Context = '<div form-create para="data.form"></div>' ;
         $scope.Components.Modal.ok = function(){
            var isAllClear = $scope.Components.Modal.form.check() ;
            if( isAllClear )
            {
               var formVal = $scope.Components.Modal.form.getValue() ;
               var configure = {} ;
               configure['ClusterName']  = $scope.clusterList[ $scope.currentCluster ]['ClusterName'] ;
               configure['BusinessType'] = 'sequoiasql' ;
               configure['BusinessName'] = moduleName ;
               configure['BusinessInfo'] = formVal ;
               discoverModule( configure ) ;
            }
            return isAllClear ;
         }
      }

      //创建 发现其他业务 弹窗
      $scope.CreateAppendOtherModel = function( moduleName, moduleType ){
         $scope.Components.Modal.icon = '' ;
         $scope.Components.Modal.title = moduleType ;
         $scope.Components.Modal.isShow = true ;
         $scope.Components.Modal.form = {
            'inputList': [
               {
                  "name": 'HostName',
                  "webName": $scope.autoLanguage( '主机名' ),
                  "type": "string",
                  "required": true,
                  "value": "",
                  "valid": {
                     "min": 1
                  }
               },
               {
                  "name": 'WebServicePort',
                  "webName": $scope.autoLanguage( '服务名' ),
                  "type": "port",
                  "required": true,
                  "value": '',
                  "valid": {}
               }
            ]
         } ;
         $scope.Components.Modal.Context = '<div form-create para="data.form"></div>' ;
         $scope.Components.Modal.ok = function(){
            var isAllClear = $scope.Components.Modal.form.check() ;
            if( isAllClear )
            {
               var formVal = $scope.Components.Modal.form.getValue() ;
               var configure = {} ;
               configure['ClusterName']  = $scope.clusterList[ $scope.currentCluster ]['ClusterName'] ;
               configure['BusinessType'] = moduleType ;
               configure['BusinessName'] = moduleName ;
               configure['BusinessInfo'] = formVal ;
               discoverModule( configure ) ;
            }
            return isAllClear ;
         }
      }

      //创建 发现业务 弹窗
      $scope.CreateAppendModuleModel = function(){
         if( $scope.clusterList.length > 0 )
         {
            $scope.Components.Modal.icon = '' ;
            $scope.Components.Modal.title = $scope.autoLanguage( '发现业务' ) ;
            $scope.Components.Modal.isShow = true ;
            $scope.Components.Modal.form = {
               'inputList': [
                  {
                     "name": 'moduleName',
                     "webName": $scope.autoLanguage( '业务名' ),
                     "type": "string",
                     "required": true,
                     "value": "",
                     "valid": {
                        "min": 1,
                        "max": 127,
                        "regex": '^[0-9a-zA-Z]+$'
                     }
                  },
                  {
                     "name": 'moduleType',
                     "webName": $scope.autoLanguage( '业务类型' ),
                     "type": "select",
                     "value": 'sequoiasql',
                     "valid": [
                        { 'key': $scope.autoLanguage( 'SequoiaSQL引擎' ), 'value': 'sequoiasql' },
                        { 'key': 'Spark', 'value': 'spark' },
                        { 'key': 'Hdfs', 'value': 'hdfs' },
                        { 'key': 'Yarn', 'value': 'yarn' },
                     ]
                  }
               ]
            } ;
            var num = 1 ;
            var defaultName = '' ;
            while( true )
            {
               var isFind = false ;
               defaultName = sprintf( 'myModule?', num ) ;
               $.each( $scope.moduleList, function( index, moduleInfo ){
                  if( defaultName == moduleInfo['BusinessName'] )
                  {
                     isFind = true ;
                     return false ;
                  }
               } ) ;
               if( isFind == false )
               {
                  $.each( $rootScope.OmTaskList, function( index, taskInfo ){
                     if( defaultName == taskInfo['Info']['BusinessName'] )
                     {
                        isFind = true ;
                        return false ;
                     }
                  } ) ;
                  if( isFind == false )
                  {
                     break ;
                  }
               }
               ++num ;
            }
            $scope.Components.Modal.form['inputList'][0]['value'] = defaultName ;
            $scope.Components.Modal.Context = '<div form-create para="data.form"></div>' ;
            $scope.Components.Modal.ok = function(){
               var isAllClear = $scope.Components.Modal.form.check( function( formVal ){
                  var isFind = false ;
                  $.each( $scope.moduleList, function( index, moduleInfo ){
                     if( formVal['moduleName'] == moduleInfo['BusinessName'] )
                     {
                        isFind = true ;
                        return false ;
                     }
                  } ) ;
                  if( isFind == false )
                  {
                     $.each( $rootScope.OmTaskList, function( index, taskInfo ){
                        if( formVal['moduleName'] == taskInfo['Info']['BusinessName'] )
                        {
                           isFind = true ;
                           return false ;
                        }
                     } ) ;
                  }
                  if( isFind == true )
                  {
                     return [ { 'name': 'moduleName', 'error': $scope.autoLanguage( '业务名已经存在' ) } ]
                  }
                  else
                  {
                     return [] ;
                  }
               } ) ;
               if( isAllClear )
               {
                  $scope.Components.Modal.isShow = false ;
                  var formVal = $scope.Components.Modal.form.getValue() ;
                  if( formVal['moduleType'] == 'sequoiasql' )
                  {
                     setTimeout( function(){
                        $scope.CreateAppendSSQLModel( formVal['moduleName'] ) ;
                        $scope.$apply() ;
                     } ) ;
                  }
                  else
                  {
                     setTimeout( function(){
                        $scope.CreateAppendOtherModel( formVal['moduleName'], formVal['moduleType'] ) ;
                        $scope.$apply() ;
                     } ) ;
                  }
               }
               else
               {
                  return false ;
               }
            }
         }
      }

      //卸载业务
      var uninstallModule = function( index ){
         if( typeof( $scope.moduleList[index]['AddtionType'] ) == 'undefined' || $scope.moduleList[index]['AddtionType'] != 1 )
         {
            var data = { 'cmd': 'remove business', 'BusinessName': $scope.moduleList[index]['BusinessName'] } ;
            SdbRest.OmOperation( data, function( taskInfo ){
               $rootScope.tempData( 'Deploy', 'Model', 'Task' ) ;
               $rootScope.tempData( 'Deploy', 'Module', 'None' ) ;
               $rootScope.tempData( 'Deploy', 'ModuleTaskID', taskInfo[0]['TaskID'] ) ;
               $location.path( '/Deploy/InstallModule' ) ;
            }, function( errorInfo ){
               _IndexPublic.createRetryModel( $scope, errorInfo, function(){
                  uninstallModule( moduleName ) ;
                  return true ;
               } ) ;
            }, function(){
               _IndexPublic.createErrorModel( $scope, $scope.autoLanguage( '网络连接错误，请尝试按F5刷新浏览器。' ) ) ;
            } ) ;
         }
         else
         {
            var data = { 'cmd': 'undiscover business', 'ClusterName': $scope.moduleList[index]['ClusterName'], 'BusinessName': $scope.moduleList[index]['BusinessName'] } ;
            SdbRest.OmOperation( data, function(){
               queryCluster() ;
            }, function( errorInfo ){
               _IndexPublic.createRetryModel( $scope, errorInfo, function(){
                  uninstallModule( moduleName ) ;
                  return true ;
               } ) ;
            }, function(){
               _IndexPublic.createErrorModel( $scope, $scope.autoLanguage( '网络连接错误，请尝试按F5刷新浏览器。' ) ) ;
            } ) ;
         }
      }

      //创建 卸载业务 弹窗
      $scope.CreateUninstallModuleModel = function(){
         if( $scope.clusterList.length > 0 )
         {
            if( $scope.ModuleNum == 0 )
            {
               $scope.Components.Confirm.type = 3 ;
               $scope.Components.Confirm.context = $scope.autoLanguage( '已经没有业务了。' ) ;
               $scope.Components.Confirm.isShow = true ;
               $scope.Components.Confirm.okText = $scope.autoLanguage( '好的' ) ;
               return ;
            }
            var clusterName = $scope.clusterList[ $scope.currentCluster ]['ClusterName'] ;
            $scope.Components.Modal.icon = '' ;
            $scope.Components.Modal.title = $scope.autoLanguage( '卸载业务' ) ;
            $scope.Components.Modal.isShow = true ;
            $scope.Components.Modal.form = {
               'inputList': [
                  {
                     "name": 'moduleIndex',
                     "webName": $scope.autoLanguage( '业务名' ),
                     "type": "select",
                     "value": null,
                     "valid": []
                  }
               ]
            } ;
            $.each( $scope.moduleList, function( index, moduleInfo ){
               if( clusterName == moduleInfo['ClusterName'] )
               {
                  if( $scope.Components.Modal.form['inputList'][0]['value'] == null )
                  {
                     $scope.Components.Modal.form['inputList'][0]['value'] = index ;
                  }
                  $scope.Components.Modal.form['inputList'][0]['valid'].push( { 'key': moduleInfo['BusinessName'], 'value': index } )
               }
            } ) ;
            $scope.Components.Modal.Context = '<div form-create para="data.form"></div>' ;
            $scope.Components.Modal.ok = function(){
               var isAllClear = $scope.Components.Modal.form.check() ;
               if( isAllClear )
               {
                  var formVal = $scope.Components.Modal.form.getValue() ;
                  uninstallModule( formVal['moduleIndex'] ) ;
               }
               return isAllClear ;
            }
         }
      }

      //创建集群
      var createCluster = function( clusterInfo, success ){
         var data = { 'cmd': 'create cluster', 'ClusterInfo': JSON.stringify( clusterInfo ) } ;
         SdbRest.OmOperation( data, function(){
            success() ;
         }, function( errorInfo ){
            _IndexPublic.createRetryModel( $scope, errorInfo, function(){
               createCluster( clusterInfo ) ;
               return true ;
            } ) ;
         }, function(){
            _IndexPublic.createErrorModel( $scope, $scope.autoLanguage( '网络连接错误，请尝试按F5刷新浏览器。' ) ) ;
         } ) ;
      }

      //创建 创建集群 弹窗
      $scope.CreateAddClusterModel = function(){
         $scope.Components.Modal.icon = '' ;
         $scope.Components.Modal.title = $scope.autoLanguage( '创建集群' ) ;
         $scope.Components.Modal.isShow = true ;
         $scope.Components.Modal.form = {
            'inputList': [
               {
                  "name": 'ClusterName',
                  "webName": $scope.autoLanguage( '集群名' ),
                  "type": "string",
                  "required": true,
                  "value": "",
                  "valid": {
                     'min': 1,
                     'max': 127,
                     'regex': '^[0-9a-zA-Z]+$'
                  }
               },
               {
                  'name': 'Desc',
                  'webName': $scope.autoLanguage( '描述' ),
                  'type': 'string',
                  'value': '',
                  'valid': {
                     'min': 0,
                     'max': 1024
                  }
               },
               {
                  "name": 'SdbUser',
                  "webName": $scope.autoLanguage( '用户名' ),
                  "type": "string",
                  "required": true,
                  "value": 'sdbadmin',
                  "valid": {
                     'min': 1,
                     'max': 1024
                  }
               },
               {
                  "name": 'SdbPasswd',
                  "webName": $scope.autoLanguage( '密码' ),
                  "type": "string",
                  "required": true,
                  "value": 'sdbadmin',
                  "valid": {
                     'min': 1,
                     'max': 1024
                  }
               },
               {
                  "name": 'SdbUserGroup',
                  "webName": $scope.autoLanguage( '用户组' ),
                  "type": "string",
                  "required": true,
                  "value": 'sdbadmin_group',
                  "valid": {
                     'min': 1,
                     'max': 1024
                  }
               },
               {
                  "name": 'InstallPath',
                  "webName": $scope.autoLanguage( '安装路径' ),
                  "type": "string",
                  "required": true,
                  "value": '/opt/sequoiadb/',
                  "valid": {
                     'min': 1,
                     'max': 2048
                  }
               }
            ]
         } ;
         var num = 1 ;
         var defaultName = '' ;
         while( true )
         {
            var isFind = false ;
            defaultName = sprintf( 'myCluster?', num ) ;
            $.each( $scope.clusterList, function( index, clusterInfo ){
               if( defaultName == clusterInfo['ClusterName'] )
               {
                  isFind = true ;
                  return false ;
               }
            } ) ;
            if( isFind == false )
            {
               break ;
            }
            ++num ;
         }
         $scope.Components.Modal.form['inputList'][0]['value'] = defaultName ;
         $scope.Components.Modal.Context = '<div form-create para="data.form"></div>' ;
         $scope.Components.Modal.ok = function(){
            var isAllClear = $scope.Components.Modal.form.check( function( formVal ){
               var isFind = false ;
               $.each( $scope.clusterList, function( index, clusterInfo ){
                  if( formVal['ClusterName'] == clusterInfo['ClusterName'] )
                  {
                     isFind = true ;
                     return false ;
                  }
               } ) ;
               if( isFind == true )
               {
                  return [ { 'name': 'ClusterName', 'error': $scope.autoLanguage( '集群名已经存在' ) } ]
               }
               else
               {
                  return [] ;
               }
            } ) ;
            if( isAllClear )
            {
               var formVal = $scope.Components.Modal.form.getValue() ;
               createCluster( formVal, function(){
                  queryCluster() ;
               } ) ;
            }
            return isAllClear ;
         }
      }

      //删除集群
      var removeCluster = function( clusterIndex ){
         var clusterName = $scope.clusterList[clusterIndex]['ClusterName'] ;
         var data = { 'cmd': 'remove cluster', 'ClusterName': clusterName } ;
         SdbRest.OmOperation( data, function(){
            if( clusterIndex == $scope.currentCluster )
            {
               $scope.currentCluster = 0 ;
            }
            queryCluster() ;
         }, function( errorInfo ){
            _IndexPublic.createRetryModel( $scope, errorInfo, function(){
               removeCluster( clusterIndex ) ;
               return true ;
            } ) ;
         }, function(){
            _IndexPublic.createErrorModel( $scope, $scope.autoLanguage( '网络连接错误，请尝试按F5刷新浏览器。' ) ) ;
         } ) ;
      }

      //创建 删除集群 弹窗
      $scope.CreateRemoveClusterModel = function(){
         $scope.Components.Modal.icon = '' ;
         $scope.Components.Modal.title = $scope.autoLanguage( '删除集群' ) ;
         $scope.Components.Modal.isShow = true ;
         $scope.Components.Modal.form = {
            'inputList': [
               {
                  "name": 'ClusterName',
                  "webName": $scope.autoLanguage( '集群名' ),
                  "type": "select",
                  "value": $scope.currentCluster,
                  "valid": []
               }
            ]
         } ;
         $.each( $scope.clusterList, function( index ){
            $scope.Components.Modal.form['inputList'][0]['valid'].push( { 'key': $scope.clusterList[index]['ClusterName'], 'value': index } ) ;
         } ) ;
         $scope.Components.Modal.Context = '<div form-create para="data.form"></div>' ;
         $scope.Components.Modal.ok = function(){
            var isAllClear = $scope.Components.Modal.form.check() ;
            if( isAllClear )
            {
               var formVal = $scope.Components.Modal.form.getValue() ;
               removeCluster( formVal['ClusterName'] ) ;
            }
            return isAllClear ;
         }
      }

      //一键部署
      $scope.CreateDeployModuleModel = function(){
         $scope.Components.Modal.icon = '' ;
         $scope.Components.Modal.title = $scope.autoLanguage( '部署' ) ;
         $scope.Components.Modal.isShow = true ;
         $scope.Components.Modal.form = {
            'inputList': [
               {
                  "name": 'ClusterName',
                  "webName": $scope.autoLanguage( '集群名' ),
                  "type": "string",
                  "required": true,
                  "value": "",
                  "valid": {
                     'min': 1,
                     'max': 127,
                     'regex': '^[0-9a-zA-Z]+$'
                  }
               },
               {
                  'name': 'Desc',
                  'webName': $scope.autoLanguage( '描述' ),
                  'type': 'string',
                  'value': '',
                  'valid': {
                     'min': 0,
                     'max': 1024
                  }
               },
               {
                  "name": 'moduleName',
                  "webName": $scope.autoLanguage( '业务名' ),
                  "type": "string",
                  "required": true,
                  "value": "myModule",
                  "valid": {
                     "min": 1,
                     "max": 127,
                     "regex": '^[0-9a-zA-Z]+$'
                  }
               },
               {
                  "name": 'moduleType',
                  "webName": $scope.autoLanguage( '业务类型' ),
                  "type": "select",
                  "value": 0,
                  "valid": []
               },
               {
                  "name": 'SdbUser',
                  "webName": $scope.autoLanguage( '用户名' ),
                  "type": "string",
                  "required": true,
                  "value": 'sdbadmin',
                  "valid": {
                     'min': 1,
                     'max': 1024
                  }
               },
               {
                  "name": 'SdbPasswd',
                  "webName": $scope.autoLanguage( '密码' ),
                  "type": "string",
                  "required": true,
                  "value": 'sdbadmin',
                  "valid": {
                     'min': 1,
                     'max': 1024
                  }
               },
               {
                  "name": 'SdbUserGroup',
                  "webName": $scope.autoLanguage( '用户组' ),
                  "type": "string",
                  "required": true,
                  "value": 'sdbadmin_group',
                  "valid": {
                     'min': 1,
                     'max': 1024
                  }
               },
               {
                  "name": 'InstallPath',
                  "webName": $scope.autoLanguage( '安装路径' ),
                  "type": "string",
                  "required": true,
                  "value": '/opt/sequoiadb/',
                  "valid": {
                     'min': 1,
                     'max': 2048
                  }
               }
            ]
         } ;
         var num = 1 ;
         var defaultName = '' ;
         while( true )
         {
            var isFind = false ;
            defaultName = sprintf( 'myCluster?', num ) ;
            $.each( $scope.clusterList, function( index, clusterInfo ){
               if( defaultName == clusterInfo['ClusterName'] )
               {
                  isFind = true ;
                  return false ;
               }
            } ) ;
            if( isFind == false )
            {
               break ;
            }
            ++num ;
         }
         $scope.Components.Modal.form['inputList'][0]['value'] = defaultName ;
         num = 1 ;
         defaultName = '' ;
         while( true )
         {
            var isFind = false ;
            defaultName = sprintf( 'myModule?', num ) ;
            $.each( $scope.moduleList, function( index, moduleInfo ){
               if( defaultName == moduleInfo['BusinessName'] )
               {
                  isFind = true ;
                  return false ;
               }
            } ) ;
            if( isFind == false )
            {
               $.each( $rootScope.OmTaskList, function( index, taskInfo ){
                  if( taskInfo['Status'] != 4 && defaultName == taskInfo['Info']['BusinessName'] )
                  {
                     isFind = true ;
                     return false ;
                  }
               } ) ;
               if( isFind == false )
               {
                  break ;
               }
            }
            ++num ;
         }
         $scope.Components.Modal.form['inputList'][2]['value'] = defaultName ;
         $.each( $scope.moduleType, function( index, typeInfo ){
            $scope.Components.Modal.form['inputList'][3]['valid'].push( { 'key': typeInfo['BusinessDesc'], 'value': index } ) ;
         } ) ;
         $scope.Components.Modal.Context = '<div form-create para="data.form"></div>' ;
         $scope.Components.Modal.ok = function(){
            var isAllClear = $scope.Components.Modal.form.check( function( formVal ){
               var rv = [] ;
               var isFind = false ;
               $.each( $scope.clusterList, function( index, clusterInfo ){
                  if( formVal['ClusterName'] == clusterInfo['ClusterName'] )
                  {
                     isFind = true ;
                     return false ;
                  }
               } ) ;
               if( isFind == true )
               {
                  rv.push( { 'name': 'ClusterName', 'error': $scope.autoLanguage( '集群名已经存在' ) } ) ;
               }
               isFind = false ;
               $.each( $scope.moduleList, function( index, moduleInfo ){
                  if( formVal['moduleName'] == moduleInfo['BusinessName'] )
                  {
                     isFind = true ;
                     return false ;
                  }
               } ) ;
               if( isFind == true )
               {
                  rv.push( { 'name': 'moduleName', 'error': $scope.autoLanguage( '业务名已经存在' ) } ) ;
               }
               return rv ;
            } ) ;
            if( isAllClear )
            {
               var formVal = $scope.Components.Modal.form.getValue() ;
               createCluster( formVal, function(){
                  $rootScope.tempData( 'Deploy', 'ClusterName', formVal['ClusterName'] ) ;
                  $rootScope.tempData( 'Deploy', 'ModuleName', formVal['moduleName'] ) ;
                  $rootScope.tempData( 'Deploy', 'Model', 'Deploy' ) ;
                  $rootScope.tempData( 'Deploy', 'Module', $scope.moduleType[ formVal['moduleType'] ]['BusinessType'] ) ;
                  $rootScope.tempData( 'Deploy', 'InstallPath', formVal['InstallPath'] ) ;
                  $location.path( '/Deploy/ScanHost' ) ;
               } ) ;
            }
            return isAllClear ;
         }
      }

      //逐个更新主机信息
      var updateHostsInfo = function( hostList, index, success ) {
         if( index == hostList.length )
         {
            setTimeout( success ) ;
            return ;
         }

         if( hostList[index]['Flag'] != 0 )
         {
            updateHostsInfo( hostList, index + 1, success ) ;
            return ;
         }

         var hostInfo = {
            'HostInfo' : [
               {
                  'HostName': hostList[index]['HostName'],
                  'IP': hostList[index]['IP']
               }   
            ]
         }
         var data = { 'cmd': 'update host info', 'HostInfo': JSON.stringify( hostInfo ) } ;
         SdbRest.OmOperation( data, function( scanInfo ){

            hostList[index]['Status'] = $scope.autoLanguage( '更新主机信息成功。' ) ;
            $scope.HostList[ hostList[index]['SourceIndex'] ]['IP'] = hostList[index]['IP'] ;

            updateHostsInfo( hostList, index + 1, success ) ;

         }, function( errorInfo ){
            Loading.close() ;
            _IndexPublic.createRetryModel( $scope, errorInfo, function(){
               Loading.create() ;
               updateHostsInfo( hostList, index, success ) ;
               return true ;
            } ) ;
         }, function(){
            _IndexPublic.createErrorModel( $scope, $scope.autoLanguage( '网络连接错误，请尝试按F5刷新浏览器。' ) ) ;
         }, null, false ) ;

      }

      //逐个扫描主机
      var scanHosts = function( hostList, index, success ){
      
         if( index == hostList.length )
         {
            success() ;
            return ;
         }

         if( hostList[index]['IP'] == $scope.HostList[ hostList[index]['SourceIndex'] ]['IP'] )
         {
            hostList[index]['Status'] = $scope.autoLanguage( 'IP地址没有改变，跳过。' ) ;
            scanHosts( hostList, index + 1, success ) ;
            return ;
         }

         hostList[index]['Status'] = $scope.autoLanguage( '正在检测...' ) ;

         var scanHostInfo = [ {
            'IP': hostList[index]['IP'],
            'SshPort': hostList[index]['SshPort'],
            'AgentService': hostList[index]['AgentService']
         } ] ;
         var clusterName = $scope.clusterList[$scope.currentCluster]['ClusterName'] ;
         var clusterUser = $scope.clusterList[$scope.currentCluster]['SdbUser'] ;
         var clusterPwd  = $scope.clusterList[$scope.currentCluster]['SdbPasswd'] ;
         var hostInfo = {
            'ClusterName': clusterName,
            'HostInfo': scanHostInfo,
            'User': clusterUser,
            'Passwd': clusterPwd,
            'SshPort': '-',
            'AgentService': '-'
         } ;
         var data = { 'cmd': 'scan host', 'HostInfo': JSON.stringify( hostInfo ) } ;
         SdbRest.OmOperation( data, function( scanInfo ){
            if( scanInfo[0]['errno'] == -38 || scanInfo[0]['errno'] == 0 )
            {
               if( scanInfo[0]['HostName'] == hostList[index]['HostName'] )
               {
                  hostList[index]['Flag'] = 0 ;
                  hostList[index]['Status'] = $scope.autoLanguage( '匹配成功。' ) ;
               }
               else
               {
                  hostList[index]['Status'] = $scope.sprintf( $scope.autoLanguage( '主机名匹配错误，IP地址?的主机名是?。' ), scanInfo[0]['IP'], scanInfo[0]['HostName'] ) ;
               }
            }
            else
            {
               hostList[index]['Status'] = $scope.autoLanguage( '错误' ) + ': ' + scanInfo[0]['detail'] ;
            }

            scanHosts( hostList, index + 1, success ) ;

         }, function( errorInfo ){
            Loading.close() ;
            _IndexPublic.createRetryModel( $scope, errorInfo, function(){
               Loading.create() ;
               scanHosts( hostList, index, success ) ;
               return true ;
            } ) ;
         }, function(){
            _IndexPublic.createErrorModel( $scope, $scope.autoLanguage( '网络连接错误，请尝试按F5刷新浏览器。' ) ) ;
         }, null, false ) ;

      }

      //创建 更新主机IP 弹窗
      $scope.CreateUpdateIpModel = function(){
         if( $scope.clusterList.length > 0 )
         {

            $scope.UpdateHostList = [] ;
            $.each( $scope.HostList, function( index ){
               if( $scope.HostList[index]['checked'] == true && $scope.clusterList[ $scope.currentCluster ]['ClusterName'] == $scope.HostList[index]['ClusterName'] )
               {
                  $scope.UpdateHostList.push( {
                     'HostName': $scope.HostList[index]['HostName'],
                     'IP': $scope.HostList[index]['IP'],
                     'SshPort': $scope.HostList[index]['SshPort'],
                     'AgentService': $scope.HostList[index]['AgentService'],
                     'Flag': -1,
                     'Status': ( $scope.HostList[index]['Error']['Flag'] == 0 ? '' : $scope.autoLanguage( '错误' ) + ': ' + $scope.HostList[index]['Error']['Message'] ),
                     'SourceIndex': index
                  } ) ;
               }
            } ) ;
            if( $scope.UpdateHostList.length > 0 )
            {
               var hostBox = null ;
               var grid = null ;
               var tempHostList = $.extend( true, [], $scope.HostList ) ;
               $scope.Components.Modal.icon = '' ;
               $scope.Components.Modal.title = $scope.autoLanguage( '更新主机信息' ) ;
               $scope.Components.Modal.isShow = true ;
               $scope.Components.Modal.Context = function( bodyEle ){
                  var div  = $( '<div></div>' ) ;

                  hostBox = $( '<div></div>' ).css( { 'marginTop': '10px' } ) ;

                  grid = $compile( '\
<div class="Grid" style="border-bottom:1px solid #E3E7E8;" ng-grid="EditHostGridOptions"">\
   <div class="GridHeader">\
      <div class="GridTr">\
         <div class="GridTd Ellipsis">{{autoLanguage("主机名")}}</div>\
         <div class="GridTd Ellipsis">{{autoLanguage("IP地址")}}</div>\
         <div class="GridTd Ellipsis">{{autoLanguage("状态")}}</div>\
         <div class="clear-float"></div>\
      </div>\
   </div>\
   <div class="GridBody">\
      <div class="GridTr" ng-repeat="hostInfo in UpdateHostList track by $index">\
         <div class="GridTd Ellipsis" style="word-break:break-all;">{{hostInfo[\'HostName\']}}</div>\
         <div class="GridTd Ellipsis" style="word-break:break-all;">\
            <input class="form-control" ng-if="hostInfo[\'Flag\'] != 0" ng-model="hostInfo[\'IP\']" />\
            <span ng-if="hostInfo[\'Flag\'] == 0" ng-bind="hostInfo[\'IP\']"></span>\
         </div>\
         <div class="GridTd Ellipsis" style="word-break:break-all;">\
            {{hostInfo[\'Status\']}}\
         </div>\
         <div class="clear-float"></div>\
      </div>\
   </div>\
</div>' )( $scope ) ;
                  hostBox.append( grid ) ;
                  $compile( bodyEle )( $scope ).append( div ).append( hostBox ) ;
                  $scope.$apply() ;
               }
               $scope.Components.Modal.onResize = function( width, height ){
                  $( grid ).css( {
                     'width': width - 10,
                     'max-height': height - 40
                  } ) ;
                  $scope.bindResize() ;
               }
               $scope.Components.Modal.ok = function(){

                  Loading.create() ;

                  scanHosts( $scope.UpdateHostList, 0, function(){

                     updateHostsInfo( $scope.UpdateHostList, 0, function(){

                        //$scope.Components.Modal.isShow = false ;
                        $scope.$apply() ;
                        Loading.cancel() ;

                     } ) ;

                  } ) ;
                  return false ;
               }
            }
            else
            {
               $scope.Components.Confirm.type = 3 ;
               $scope.Components.Confirm.context = $scope.autoLanguage( '至少选择一台的主机。' ) ;
               $scope.Components.Confirm.isShow = true ;
               $scope.Components.Confirm.okText = $scope.autoLanguage( '好的' ) ;
            }

         }
      }

      //执行
      GetModuleType() ;
      queryCluster() ;

      if( defaultShow == 'host' )
      {
         $scope.SwitchPage( defaultShow ) ;
      }
   } ) ;
}());