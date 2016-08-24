(function(){
   var sacApp = window.SdbSacManagerModule ;
   //控制器
   sacApp.controllerProvider.register( 'Monitor.SdbGroup.Index.Ctrl', function( $scope, $compile, $location, SdbRest, SdbFunction ){
      
      var clusterName = SdbFunction.LocalData( 'SdbClusterName' ) ;
      var moduleType = SdbFunction.LocalData( 'SdbModuleType' ) ;
      var moduleName = SdbFunction.LocalData( 'SdbModuleName' ) ;
      var groupName = SdbFunction.LocalData( 'SdbGroupName' ) ;
      $scope.clusterName = clusterName ;
      $scope.moduleName = moduleName ;
      $scope.moduleType = moduleType ;
      $scope.hostList = [] ;
      $scope.PrimaryNode = '' ;
      var i = 0 ;

      $scope.GroupList = [] ;
      var getGroupList = function(){
         var data = { 'cmd': 'list groups' } ;
         SdbRest.DataOperation( data, function( groups ){
            $.each( groups, function( index, groupInfo ){
               if( groupInfo['GroupName'] == groupName )
               {
                  if( groupInfo['Role'] == 0 )
                  {
                     groupInfo['Role'] = '数据组' ;
                  }
                  else if ( groupInfo['Role'] == 2 )
                  {
                     groupInfo['Role'] = '编目组' ;
                  }
                  $scope.groupInfo = groupInfo ;
               }
            } ) ;
            $.each( $scope.groupInfo['Group'], function( index, value ){
               $scope.hostList.push( { "key": value['HostName'] + ':' + value['Service'][0]['Name'], "value": index } ) ;
               //获取主节点节点名
               if( $scope.groupInfo['PrimaryNode'] == value['NodeID'] )
               {
                  $scope.PrimaryNode = value['HostName'] + ':' + value['Service'][0]['Name'] ;
               }
            } ) ;
         }, function( errorInfo ){
            _IndexPublic.createRetryModel( $scope, errorInfo, function(){
               getGroupList() ;
               return true ;
            } ) ;
         }, function(){
            _IndexPublic.createErrorModel( $scope, $scope.autoLanguage( '网络连接错误，请尝试按F5刷新浏览器。' ) ) ;
         } ) ;
      } ;

      var getDbList = function(){
         var sql  = 'SELECT NodeName, HostName, GroupName, IsPrimary, ServiceName, ServiceStatus, NodeID FROM $SNAPSHOT_DB WHERE Groups = "' + groupName + '"' ;
         SdbRest.Exec( sql, function( DbList ){
            $scope.DbList = DbList ;
         }, function( errorInfo ){
            _IndexPublic.createRetryModel( $scope, errorInfo, function(){
               getDbList() ;
               return true ;
            } ) ;
         }, function(){
            _IndexPublic.createErrorModel( $scope, $scope.autoLanguage( '网络连接错误，请尝试按F5刷新浏览器。' ) ) ;
         } ) ;
      } ;
      getDbList() ;
      getGroupList() ;

      //创建节点
      $scope.addNode = function(){
         $scope.Components.Modal.icon = '' ;
         $scope.Components.Modal.title = '创建节点' ;
         $scope.Components.Modal.noOK = false ;
         $scope.Components.Modal.isShow = true ;
         $scope.Components.Modal.form = {
            inputList: [
               {
                  "name": "hostName",
                  "webName": $scope.autoLanguage( '选择主机' ),
                  "type": "select",
                  "required": true,
                  "value": "1",
                  "valid": [
                     { "key":"ubuntu-test-02", "value": "1" },
                     { "key":"ubuntu-test-03", "value": "2" },
                     { "key":"ubuntu-test-04", "value": "3" },
                     { "key":"ubuntu-test-05", "value": "4" }
                  ]
               },
               {
                  "name": "svcname",
                  "webName": $scope.autoLanguage( '节点端口号' ),
                  "type": "string",
                  "required": true,
                  "value": "",
                  "valid": {
                     "min": 1,
                     "max": 127,
                     "ban": [ ".", "$" ]
                  }
               },
               {
                  "name": "path",
                  "webName": $scope.autoLanguage( '节点路径' ),
                  "type": "string",
                  "required": true,
                  "value": "",
                  "valid": {
                     "min": 1,
                     "max": 127,
                     "ban": [ ".", "$" ]
                  }
               },
               {
                  "name": "config",
                  "webName": $scope.autoLanguage( '配置' ),
                  "type": "string",
                  "required": true,
                  "value": "",
                  "valid": {
                     "min": 1,
                     "max": 127,
                     "ban": [ ".", "$" ]
                  }
               }
            ]
         } ;
         $scope.Components.Modal.Context = '<div form-create para="data.form"></div>';
         $scope.Components.Modal.ok = function(){
            $scope.Components.Modal.isShow = false ;
         }
      }

      //删除节点
      $scope.deleteNode = function(){
         $scope.Components.Modal.icon = '' ;
         $scope.Components.Modal.title = '删除节点' ;
         $scope.Components.Modal.noOK = false ;
         $scope.Components.Modal.isShow = true ;
         $scope.Components.Modal.form = {
            inputList: [
               {
                  "name": "nodeName",
                  "webName": $scope.autoLanguage( '选择节点' ),
                  "type": "select",
                  "required": true,
                  "value": 0,
                  "valid": $scope.hostList
               }
            ]
         } ;
         $scope.Components.Modal.Context = '<div form-create para="data.form"></div>';
         $scope.Components.Modal.ok = function(){
            $scope.Components.Confirm.isShow = true ;
            $scope.Components.Confirm.type = 1 ;
            $scope.Components.Confirm.okText = $scope.autoLanguage( '确定' ) ;
            $scope.Components.Confirm.closeText = $scope.autoLanguage( '取消' ) ;
            $scope.Components.Confirm.title = $scope.autoLanguage( '要删除该节点吗？' ) ;
            $scope.Components.Confirm.context = '节点名：ubuntu-test-02:11830' ;
            $scope.Components.Confirm.ok = function(){
               return true ;
            }
            return true ;
         }
      }

      //启动节点
      $scope.startNode = function(){
         $scope.Components.Modal.icon = '' ;
         $scope.Components.Modal.title = '启动节点' ;
         $scope.Components.Modal.noOK = false ;
         $scope.Components.Modal.isShow = true ;
         $scope.Components.Modal.form = {
            inputList: [
               {
                  "name": "nodeName",
                  "webName": $scope.autoLanguage( '选择节点' ),
                  "type": "select",
                  "required": true,
                  "value": 0,
                  "valid": $scope.hostList
               }
            ]
         } ;
         $scope.Components.Modal.Context = '<div form-create para="data.form"></div>';
         $scope.Components.Modal.ok = function(){
            return true ;
         }
      }

      //停止节点
      $scope.stopNode = function(){
         $scope.Components.Modal.icon = '' ;
         $scope.Components.Modal.title = '停止节点' ;
         $scope.Components.Modal.noOK = false ;
         $scope.Components.Modal.isShow = true ;
         $scope.Components.Modal.form = {
            inputList: [
               {
                  "name": "nodeName",
                  "webName": $scope.autoLanguage( '选择节点' ),
                  "type": "select",
                  "required": true,
                  "value": 0,
                  "valid": $scope.hostList             
               }
            ]
         } ;
         $scope.Components.Modal.Context = '<div form-create para="data.form"></div>';
         $scope.Components.Modal.ok = function(){
            return true ;
         }
      } ;

       //跳转至部署
      $scope.GotoDeploy = function(){
         $location.path( '/Deploy/Index' ) ;
      } ;

      //跳转至监控主页
      $scope.GotoModule = function(){
         $location.path( '/Monitor/Index' ) ;
      } ;

      //跳转至分区组列表
      $scope.GotoGroups = function(){
         $location.path( '/Monitor/SDB-Overview/Index' ) ;
      } ;

      //跳转至节点信息
      $scope.GotoNode = function(){
         $location.path( '/Monitor/SDB-Node/Index' ) ;
      } ;
      
      //跳转至主机信息
      $scope.GotoHost = function(){
         $location.path( '/Monitor/Host-Info/Index' ) ;
      } ;

      //跳转至域
      $scope.GotoDomains = function(){
         $location.path( '/Monitor/SDB-Overview/Domain' ) ;
      } ;

   } ) ;
}());