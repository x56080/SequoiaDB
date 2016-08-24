(function(){
   var sacApp = window.SdbSacManagerModule ;
   //控制器
   sacApp.controllerProvider.register( 'Monitor.SdbOverview.Node.Ctrl', function( $scope, $compile, $location, SdbRest, SdbFunction ){
      var clusterName = SdbFunction.LocalData( 'SdbClusterName' ) ;
      var moduleType = SdbFunction.LocalData( 'SdbModuleType' ) ;
      var moduleName = SdbFunction.LocalData( 'SdbModuleName' ) ;
      var isOpenSelectMenu = false ;
      $scope.clusterName = clusterName ;
      $scope.moduleName = moduleName ;
      $scope.moduleType =  moduleType ;
      $scope.NodeList = [] ;
      $scope.NodeGridOptions = { 'titleWidth': [] } ;
      $scope.ShowKeyList = [ 'NodeName', 'GroupName', 'IsPrimary', 'Role', 'TotalRecords', 'TotalLobs', 'LSN' ] ;
      $scope.ShowKey = [] ;
      $scope.SelectMenu = [] ;
      $scope.OrderByField = [] ;
      $scope.NodesList = [] ;
      $scope.DbList = [] ;
      $scope.ClList = [] ;
      var nodesList = [] ;

       //获取节点列表
      var getNodesList = function(){
         var data = { 'cmd': 'list groups' } ;
         SdbRest.DataOperation( data, function( groups ){
            $scope.GroupList = groups ;
            $.each( groups, function( index, groupInfo ){
               if( groupInfo['Role'] == 2 )
               {
                  groupInfo['Role'] = 'catalog' ;
               }
               else if( groupInfo['Role'] == 1 )
               {
                  groupInfo['Role'] = 'coord' ;
               }
               else if( groupInfo['Role'] == 0 )
               {
                  groupInfo['Role'] = 'data' ;
               }
               $.each( groupInfo['Group'], function( index2, nodeInfo ){
                  nodesList.push( { 'HostName': nodeInfo['HostName'], 'ServiceName': nodeInfo['Service']['0']['Name'], 'GroupName': groupInfo['GroupName'], 'Role': groupInfo['Role']  } )
               } ) ;
            } ) ;
            $.each( nodesList, function( index, nodeInfo ){
               nodesList[index]['NodeName'] = nodeInfo['HostName'] + ':' + nodeInfo['ServiceName'] ;
               nodesList[index]['IsPrimary'] = false ;
               nodesList[index]['Status'] = true ;
               nodesList[index]['TotalRecords'] = 0 ;
               nodesList[index]['TotalLobs'] = 0 ;
               nodesList[index]['TotalCL'] = 0 ;
               nodesList[index]['NodeID'] = 0 ;
               //LSN 虚构
               nodesList[index]['LSN'] = 1 ;
               
               if( $scope.DbList[0]['ErrNodes'] != undefined )
               {
                  if( $scope.DbList[0]['ErrNodes'].length > 0 )
                  {
                     $.each( $scope.DbList[0]['ErrNodes'], function( nodeIndex, errNodeInfo ){

                        if( nodesList[index]['NodeName'] == errNodeInfo['NodeName'] )
                        {
                           nodesList[index]['Status'] = false ;
                           nodesList[index]['Flag'] = -79 ;
                           nodesList[index]['TotalRecords'] = '' ;
                           nodesList[index]['TotalLobs'] = '' ;
                           nodesList[index]['TotalCL'] = '' ;
                        }
                     
                     } )
                  }
               }
               
               
               $.each( $scope.DbList, function( index2, DbInfo ){
                  if( nodesList[index]['NodeName'] == DbInfo['NodeName'] )
                  {
                     nodesList[index]['ServiceName'] = DbInfo['ServiceName'] ;
                     nodesList[index]['GroupName'] = DbInfo['GroupName'] ;
                     if( DbInfo['IsPrimary'] == true )
                     {
                        nodesList[index]['IsPrimary'] = true ;
                     }
                     if( DbInfo['ServiceStatus'] == false )
                     {
                        nodesList[index]['Status'] = false ;
                     }
                  }
               } ) ;
               $.each( $scope.ClList, function( index3, ClInfo ){
                  if( nodesList[index]['NodeName'] == ClInfo['NodeName'] )
                  {
                     nodesList[index]['TotalCL'] += 1 ;
                     nodesList[index]['TotalRecords'] += ClInfo['TotalRecords'] ;
                     nodesList[index]['TotalLobs'] += ClInfo['TotalLobs'] ;
                  }
               } ) ;
            } ) ;
            $scope.NodesList = nodesList ;
            var keyList = [ "NodeName", "HostName", "GroupName", "IsPrimary", "Role", "TotalCL", "TotalRecords", "TotalLobs", "LSN" ] ;
            $scope.ShowKey = [] ;
            $scope.SelectMenu = [] ;
            $.each( keyList, function( index, key ){
               $scope.ShowKey.push( { 'key': key, 'show': $scope.ShowKeyList.indexOf( key ) >= 0 } ) ;
               $scope.SelectMenu.push( { 
                  'html': $compile( '<label><div class="Ellipsis" style="padding:5px 10px"><input type="checkbox" ng-model="ShowKey[\'' + index + '\'][\'show\']"/>&nbsp;' + key + '</div></label>' )( $scope ),
                  'onClick': function(){}
               } ) ;
            } ) ;
            $scope.SelectMenu.push( { 
               'html': $compile( '<button class="btn btn-primary" ng-click="SaveShowKeyList()" style="width:100%;">确定</button>' )( $scope )
            } ) ;
            gridShowColumn() ;
            $scope.Timer.complete = true ;

         }, function( errorInfo ){
            _IndexPublic.createRetryModel( $scope, errorInfo, function(){
               getNodesList() ;
               return true ;
            } ) ;
         }, function(){
            _IndexPublic.createErrorModel( $scope, $scope.autoLanguage( '网络连接错误，请尝试按F5刷新浏览器。' ) ) ;
         } ) ;
      } ;

      //获取CL快照
      var getClList = function(){
         var sql = 'SELECT t1.Name,t1.Details.TotalRecords, t1.Details.TotalLobs,t1.Details.NodeName from (SELECT Name, Details FROM $SNAPSHOT_CL WHERE Global = true split By Details) As t1' ;
         SdbRest.Exec( sql, function( ClList ){
            $scope.ClList = ClList ;
            getNodesList() ;
         }, function( errorInfo ){
            _IndexPublic.createRetryModel( $scope, errorInfo, function(){
               getDbList() ;
               return true ;
            } ) ;
         }, function(){
            _IndexPublic.createErrorModel( $scope, $scope.autoLanguage( '网络连接错误，请尝试按F5刷新浏览器。' ) ) ;
         } ) ;
      } ;

      //获取DB快照
      var getDbList = function(){
         var sql  = 'SELECT NodeName, HostName, GroupName, IsPrimary, ServiceName, ServiceStatus, NodeID FROM $SNAPSHOT_DB' ;
         SdbRest.Exec( sql, function( DbList ){
            $scope.DbList = DbList ;
            
            getClList() ;
         }, function( errorInfo ){
            _IndexPublic.createRetryModel( $scope, errorInfo, function(){
               getDbList() ;
               return true ;
            } ) ;
         }, function(){
            _IndexPublic.createErrorModel( $scope, $scope.autoLanguage( '网络连接错误，请尝试按F5刷新浏览器。' ) ) ;
         } ) ;
      } ;

      //设置排序字段
      $scope.SetOrderField = function( fieldName ){
         var normal  = $scope.OrderByField.indexOf( fieldName ) ;
         var reverse = $scope.OrderByField.indexOf( '-' + fieldName ) ;
         if( normal == -1 && reverse == -1 )
         {
            $scope.OrderByField.push( fieldName ) ;
         }
         else if( normal >= 0 )
         {
            $scope.OrderByField[normal] = '-' + fieldName ;
         }
         else if( reverse >= 0 )
         {
            $scope.OrderByField.splice( reverse, 1 ) ;
         }
      }

      //创建 设置实时刷新 的弹窗
      $scope.CreatePlayIntervalModel = function(){
         $scope.Components.Modal.icon = '' ;
         $scope.Components.Modal.title = $scope.autoLanguage( '实时刷新设置' ) ;
         $scope.Components.Modal.isShow = true ;
         $scope.Components.Modal.form = {
            inputList: [
               {
                  "name": "play",
                  "webName": $scope.autoLanguage( '自动刷新' ),
                  "type": "select",
                  "value": $scope.Timer.status == 'start',
                  "valid": [
                     { 'key': '开启', 'value': true },
                     { 'key': '停止', 'value': false }
                  ]
               },
               {
                  "name": "interval",
                  "webName": $scope.autoLanguage( '刷新间距(秒)' ),
                  "type": "int",
                  "value": $scope.Timer.interval,
                  "valid": {
                     'min': 1
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
               $scope.Timer.interval = formVal['interval'] ;
               if( formVal['play'] == true )
               {
                  $scope.Timer.status = 'start' ;
               }
               else
               {
                  $scope.Timer.status = 'stop' ;
               }
            }
            return isAllClear ;
         }
      }
      
      //打开 网格显示列 的下拉菜单
      $scope.OpenSelecMenu = function(){
         if( $scope.Timer.status == 'start' )
         {
            isOpenSelectMenu = true ;
            $scope.Timer.status = 'stop' ;
         }
      }

      //渲染网格显示的列
      var gridShowColumn = function(){
         $scope.NodeGridOptions['titleWidth'] = [] ;
         $scope.NodeGridOptions['titleWidth'].push( '40px',17 ) ;
         var widthPercent = 100 / $scope.ShowKeyList.length ;
         $.each( $scope.ShowKeyList, function( index, keyName ){
            $scope.NodeGridOptions['titleWidth'].push( widthPercent ) ;
         } ) ;
         $scope.NodeGridOptions.onResize() ;
      }

      //保存显示列
      $scope.SaveShowKeyList = function(){
         if( isOpenSelectMenu == true && $scope.Timer.status == 'stop' )
         {
            isOpenSelectMenu = false ;
            $scope.Timer.status = 'start' ;
         }
         $scope.ShowKeyList = [] ;
         $.each( $scope.ShowKey, function( index, keyInfo ){
            if( keyInfo['show'] == true )
            {
               $scope.ShowKeyList.push( keyInfo['key'] ) ;
            }
         } ) ;
         gridShowColumn() ;
      }

      $scope.Timer = {
         status: 'stop',
         interval: 5,
         currentTimer: 0,
         complete: false,
         fn: getDbList
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
      $scope.GotoNode = function( HostName, ServiceName ){
         SdbFunction.LocalData( 'SdbHostName', HostName ) ;
         SdbFunction.LocalData( 'SdbServiceName', ServiceName ) ;
         $location.path( '/Monitor/SDB-Node/Index' ) ;
      } ;

      //跳转至分区组信息
      $scope.GotoGroup = function( GroupName ){
         SdbFunction.LocalData( 'SdbGroupName', GroupName ) ;
         $location.path( '/Monitor/SDB-Group/Index' ) ;
      } ;

      //跳转至主机信息
      $scope.GotoHost = function(){
         $location.path( '/Monitor/Host-Info/Index' ) ;
      } ;

      //跳转至数据库操作
      $scope.GotoDatabase = function(){
         $location.path( '/Data/SDB-Database/Index' ) ;
      } ;

      getDbList() ;
   } ) ;
}());