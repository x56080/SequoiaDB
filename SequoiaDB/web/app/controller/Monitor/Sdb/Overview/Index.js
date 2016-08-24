(function(){
   var sacApp = window.SdbSacManagerModule ;
   //控制器
   sacApp.controllerProvider.register( 'Monitor.SdbOverview.Index.Ctrl', function( $scope, $compile, $location, SdbRest, SdbFunction ){
      var ClusterName = SdbFunction.LocalData( 'SdbClusterName' ) ;
      var ModuleType = SdbFunction.LocalData( 'SdbModuleType' ) ;
      var ModuleName = SdbFunction.LocalData( 'SdbModuleName' ) ;

      //初始化
      $scope.ClusterName = ClusterName ;
      $scope.ModuleName = ModuleName ;
      $scope.ModuleType =  ModuleType ;
      $scope.GroupList = [] ;
      $scope.groupNameList = [] ;
      $scope.waitingGroupList = [] ;
      $scope.runningGroupList = [] ;
      $scope.clList = [] ;
      $scope.DatagroupNum = 0 ;
      var statusIcon = '' ;

      //新表格
      $scope.GroupGridOptions = { 'titleWidth': [] } ;

      //渲染网格显示的列
      var gridShowColumn = function(){
         $scope.GroupGridOptions['titleWidth'] = [] ;
         $scope.GroupGridOptions['titleWidth'].push( '40px',15,15,30,20,20 ) ;
         
      }


      var sql = '' ;
      sql = 'SELECT Name, Details FROM $SNAPSHOT_CL WHERE NodeSelect="master" ORDER BY Name' ;
      
      $scope.queryList = function( data, success, failed, error, complete ){
         SdbRest._postTest( './test/groupList', success, failed, error ) ;
      } 
      

      var getClInfo = function(){
         //获取CL信息
         SdbRest.Exec( sql, function( clList ){
            $scope.clList = clList ;
            $.each( $scope.GroupList, function( index, groupInfo ){
               //role 0 is data group
               if( groupInfo['Role'] == 0 )
               {
                  $scope.DatagroupNum = $scope.DatagroupNum + 1 ;
                  $scope.GroupList[index]['TotalCL'] = 0 ;
                  $scope.GroupList[index]['TotalRecords'] = 0 ;
                  $.each( $scope.clList, function( index2, clInfo ){
                     $.each( clInfo['Details'], function( index3, clDetail ){
                        if( clDetail['GroupName'] == groupInfo['GroupName'] )
                        {
                           ++$scope.GroupList[index]['TotalCL'] ;
                           $scope.GroupList[index]['TotalRecords'] += clDetail['TotalRecords'] ;
                        }
                     } )
                  } ) ;
               }
            } ) ;
            gridShowColumn() ;
         }, function( errorInfo ){
            _IndexPublic.createRetryModel( $scope, errorInfo, function(){
               $scope.getGroupList() ;
               return true ;
            } ) ;
         }, function(){
            _IndexPublic.createErrorModel( $scope, $scope.autoLanguage( '网络连接错误，请尝试按F5刷新浏览器。' ) ) ;
         } ) ;
      }

      var getGroupList = function(){
         var data = { 'cmd': 'list groups' } ;
         SdbRest.DataOperation( data, function( groups ){
            $scope.GroupList = groups ;
            $.each( groups, function( index, value ){
               $scope.groupNameList.push(
                  { "key":value['GroupName'], "value": index }
               ) ;
               
               if( value['Status'] == 0 )
               {
                  $scope.waitingGroupList.push(
                     { "key":value['GroupName'], "value": index }
                  ) ;
               }
               else if( value['Status'] == 1 )
               {
                  $scope.runningGroupList.push(
                     { "key":value['GroupName'], "value": index }
                  ) ;
               }
               $scope.dataNumber = index + 1 ;
            } ) ;
            getClInfo() ;
         }, function( errorInfo ){
            _IndexPublic.createRetryModel( $scope, errorInfo, function(){
               getGroupList() ;
               return true ;
            } ) ;
         }, function(){
            _IndexPublic.createErrorModel( $scope, $scope.autoLanguage( '网络连接错误，请尝试按F5刷新浏览器。' ) ) ;
         } ) ;
      } ;
      getGroupList() ;
      




      $scope.charts = {}; 
      $scope.charts['Storage'] = {} ;
      $scope.charts['Storage']['options'] = window.SdbSacManagerConf.DiskStorageEchart ;
      $scope.charts['Storage']['value'] = [ [ 0, 0, true, false ] ] ;

      $scope.charts['Insert'] = {} ;
      $scope.charts['Insert']['options'] = window.SdbSacManagerConf.RecordInsertEchart ;
      $scope.charts['Insert']['value'] = [ [ 0, 0, true, false ] ] ;

      $scope.charts['Ram'] = {} ;
      $scope.charts['Ram']['options'] = window.SdbSacManagerConf.RamBarEchart ;
      $scope.charts['Ram']['value'] = [ [ 0, 0, true, false ] ] ;

      

      //创建分区组
      $scope.addGroup = function(){
         $scope.Components.Modal.icon = '' ;
         $scope.Components.Modal.title = '创建分区组' ;
         $scope.Components.Modal.noOK = false ;
         $scope.Components.Modal.isShow = true ;
         $scope.Components.Modal.form = {
            inputList: [
               {
                  "name": "groupName",
                  "webName": $scope.autoLanguage( '分区组名' ),
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
                  "name": "role",
                  "webName": $scope.autoLanguage( '角色' ),
                  "type": "select",
                  "value": "data",
                  "valid": [
                     { "key": "coord", "value": "coord" },
                     { "key": "catalog", "value": "catalog" },
                     { "key": "data", "value": "data" }
                  ]
               }
            ]
         } ;
         $scope.Components.Modal.Context = '<div form-create para="data.form"></div>' ;
         $scope.Components.Modal.ok = function () {
             var isAllClear = $scope.Components.Modal.form.check();
             if (isAllClear) {
                 var formVal = $scope.Components.Modal.form.getValue();
             }
             //return isAllClear;
         }
      }
      //删除分区组  
      $scope.deleteGroup = function(){
         $scope.Components.Modal.icon = '' ;
         $scope.Components.Modal.title = '删除分区组' ;
         $scope.Components.Modal.noOK = false ;
         $scope.Components.Modal.isShow = true ;
         $scope.Components.Modal.form = {
            inputList: [
               {
                  "name": "groupName",
                  "webName": $scope.autoLanguage( '选择分区组' ),
                  "type": "select",
                  "required": true,
                  "value": 1,
                  "valid": $scope.groupNameList
               }
            ]
         } ;
         $scope.Components.Modal.Context = '<div form-create para="data.form"></div>' ;
         $scope.Components.Modal.ok = function(){
            $scope.Components.Confirm.isShow = true ;
            $scope.Components.Confirm.type = 1 ;
            $scope.Components.Confirm.okText = $scope.autoLanguage( '确定' ) ;
            $scope.Components.Confirm.closeText = $scope.autoLanguage( '取消' ) ;
            $scope.Components.Confirm.title = $scope.autoLanguage( '要删除该分区组吗？' ) ;
            $scope.Components.Confirm.context = '分区组名：datagroup1' ;
            $scope.Components.Confirm.ok = function(){
               return true ;
            }
            return true ;
         }
      }
      //启动分区组
      $scope.startGroup = function(){
         $scope.Components.Modal.icon = '' ;
         $scope.Components.Modal.title = '启动分区组' ;
         $scope.Components.Modal.noOK = false ;
         $scope.Components.Modal.isShow = true ;
         $scope.Components.Modal.form = {
            inputList: [
               {
                  "name": "groupName",
                  "webName": $scope.autoLanguage( '选择分区组' ),
                  "type": "select",
                  "required": true,
                  "value": 6,
                  "valid": $scope.waitingGroupList
               }
            ]
         } ;
         $scope.Components.Modal.Context = '<div form-create para="data.form"></div>' ;
         $scope.Components.Modal.ok = function(){
            return true ;
         } ;
      }

      //停止分区组
      $scope.stopGroup = function(){
         $scope.Components.Modal.icon = '' ;
         $scope.Components.Modal.title = '停止分区组' ;
         $scope.Components.Modal.noOK = false ;
         $scope.Components.Modal.isShow = true ;
         $scope.Components.Modal.form = {
            inputList: [
               {
                  "name": "groupName",
                  "webName": $scope.autoLanguage( '选择分区组' ),
                  "type": "select",
                  "required": true,
                  "value": 1,
                  "valid": $scope.runningGroupList
               }
            ]
         } ;
         $scope.Components.Modal.Context = '<div form-create para="data.form"></div>';
         $scope.Components.Modal.ok = function(){
            return true ;
         } ;
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

      //跳转至分区组信息
      $scope.GotoGroup = function( GroupName ){
         SdbFunction.LocalData( 'SdbGroupName', GroupName ) ;
         $location.path( '/Monitor/SDB-Group/Index' ) ;
      } ;

      //跳转至节点信息
      $scope.GotoNode = function( HostName, ServiceName ){
         SdbFunction.LocalData( 'SdbHostName', HostName ) ;
         SdbFunction.LocalData( 'SdbServiceName', ServiceName ) ;
         $location.path( '/Monitor/SDB-Node/Index' ) ;
      } ;


   } ) ;
}());

