(function(){
   var sacApp = window.SdbSacManagerModule ;
   //控制器
   sacApp.controllerProvider.register( 'Monitor.Preview.Index.Ctrl', function( $scope, $compile, $location, SdbRest, SdbFunction ){
     
      var clusterName = SdbFunction.LocalData( 'SdbClusterName' ) ;
      var moduleType = SdbFunction.LocalData( 'SdbModuleType' ) ;
      var moduleMode = SdbFunction.LocalData( 'SdbModuleMode' ) ;
      var moduleName = SdbFunction.LocalData( 'SdbModuleName' ) ;

      //初始化
      $scope.charts = {}; 
      $scope.charts['Host'] = {} ;
      $scope.clusterName = clusterName ;
      $scope.moduleName = moduleName ;
      $scope.moduleInfo = {} ;
      $scope.hostInfo = {} ;
      $scope.HostList = [] ;
      $scope.charts['Module'] = {} ;
      $scope.chartName = 'Record Insert' ;
      $scope.charts['Module']['options'] = window.SdbSacManagerConf.RecordInsertEchart ;
      $scope.DiskNum = 0 ;

      //测试用
      var s = 0 ;
      var sql = '' ;
      var RecordsNum = 0 ;

      $scope.ModuleInfo1 = { 'Version': '', 'Domain': 0, 'Session': 0,  'Groups': 0, 'Collections':0, 'TotalLobs': 0, 'TotalRecords':0 } ;
      //获取集合列表
      var getClList = function(){
         var cls = [] ;
         sql = 'SELECT t1.Name,t1.Details.TotalRecords,t1.Details.TotalLobs FROM (SELECT Name, Details FROM $SNAPSHOT_CL WHERE NodeSelect = "master" SPLIT By Details) As t1' ;
         SdbRest.Exec( sql, function( clList ){
            $.each( clList, function( index, clInfo ){
               
               $scope.ModuleInfo1['TotalLobs'] += clInfo['TotalLobs'] ;
               $scope.ModuleInfo1['TotalRecords'] += clInfo['TotalRecords'] ;
            } ) ;
         }, function( errorInfo ){
            _IndexPublic.createRetryModel( $scope, errorInfo, function(){
               getClList() ;
               return true ;
            } ) ;
         }, function(){
            _IndexPublic.createErrorModel( $scope, $scope.autoLanguage( '网络连接错误，请尝试按F5刷新浏览器。' ) ) ;
         } ) ;
      } ;
      getClList() ;

      var getDbList = function(){
         var SumInfo = {} ;
         $scope.DbInfo = { 'TotalInsert':0, 'TotalUpdate': 0, 'TotalDelete':0, 'TotalRead':0 } ;
         sql = 'SELECT Version, TotalInsert, TotalUpdate, TotalDelete, TotalRead FROM $SNAPSHOT_DB WHERE NodeSelect = "master"' ;
         SdbRest.Exec( sql, function( dbList ){
            $scope.ModuleInfo1['Version'] = dbList[0]['Version']['Major'] + '.' + dbList[0]['Version']['Minor']
            $.each( dbList, function( index, DbInfo ){
                  $scope.DbInfo['TotalInsert'] += DbInfo['TotalInsert'] ;
                  $scope.DbInfo['TotalUpdate'] += DbInfo['TotalUpdate'] ;
                  $scope.DbInfo['TotalDelete'] += DbInfo['TotalDelete'] ;
                  $scope.DbInfo['TotalRead'] += DbInfo['TotalRead'] ;
            } ) ;

            if( typeof( SumInfo['TotalInsert'] ) == 'undefined' )
            {
               SumInfo['TotalInsert'] = $scope.DbInfo['TotalInsert'] ;
               SumInfo['TotalUpdate'] = $scope.DbInfo['TotalUpdate'] ;
               SumInfo['TotalDelete'] = $scope.DbInfo['TotalDelete'] ;
               SumInfo['TotalRead'] = $scope.DbInfo['TotalRead'] ;
            }
            else
            {
               $scope.charts['Insert']['value'] = [ [ 0, ( $scope.DbInfo['TotalInsert'] - SumInfo['TotalInsert'] )/5, true, false ] ] ;
               $scope.charts['Update']['value'] = [ [ 0, ( $scope.DbInfo['TotalUpdate'] - SumInfo['TotalUpdate'] )/5, true, false ] ] ;
               $scope.charts['Delete']['value'] = [ [ 0, ( $scope.DbInfo['TotalDelete'] - SumInfo['TotalDelete'] )/5, true, false ] ] ;
               $scope.charts['Query']['value'] = [ [ 0, ( $scope.DbInfo['TotalRead'] - SumInfo['TotalRead'] )/5, true, false ] ] ;

               SumInfo['TotalInsert'] = $scope.DbInfo['TotalInsert'] ;
               SumInfo['TotalUpdate'] = $scope.DbInfo['TotalUpdate'] ;
               SumInfo['TotalDelete'] = $scope.DbInfo['TotalDelete'] ;
               SumInfo['TotalRead'] = $scope.DbInfo['TotalRead'] ;
            }
         }, function( errorInfo ){
            _IndexPublic.createRetryModel( $scope, errorInfo, function(){
               getDbList() ;
               return true ;
            } ) ;
         }, function(){
            _IndexPublic.createErrorModel( $scope, $scope.autoLanguage( '网络连接错误，请尝试按F5刷新浏览器。' ) ) ;
         }, null, false ) ;
      } ;
      getDbList() ;





      var queryHost = function(){
         var data = {
            'cmd': 'query host',
            //'filter' : JSON.stringify( { 'HostName':'ubuntu-test-02' } )
            //'HostInfo': JSON.stringify( {"HostInfo":[ {"HostName":"ubuntu-test-02"} ] } )
         } ;
         SdbRest.OmOperation( data, function( hostList ){
            $.each( hostList, function( index, hostInfo ){
               if( hostInfo['ClusterName'] == clusterName )
               {
                  $scope.HostList.push( hostInfo ) ;
               }               
            } ) ;
         }, function( errorInfo ){
            _IndexPublic.createRetryModel( $scope, errorInfo, function(){
               queryHost() ;
               return true ;
            } ) ;
         }, function(){
            _IndexPublic.createErrorModel( $scope, $scope.autoLanguage( '网络连接错误，请尝试按F5刷新浏览器。' ) ) ;
         } ) ;
      }

      var queryModule = function( data, success, failed, error, complete )
      {
         SdbRest._postTest( './test/moduleInfo', success, failed, error ) ;
         SdbRest.Exec( sql, function( csList ){

         }, function( errorInfo ){
            _IndexPublic.createRetryModel( $scope, errorInfo, function(){
               queryModule() ;
               return true ;
            } ) ;
         }, function(){
            _IndexPublic.createErrorModel( $scope, $scope.autoLanguage( '网络连接错误，请尝试按F5刷新浏览器。' ) ) ;
         } ) ;
      }
      

      var moduleList = function(){
         queryHost() ;
         queryModule( {}, function( test ){
            $scope.moduleInfo = test[0] ;
            $scope.charts['Host']['CPU'] = { 'percent': $scope.moduleInfo['cpuUse'], 'style': { 'progress': { 'background': '#FF9933' } } } ;   
            $scope.charts['Host']['Memory'] = { 'percent': $scope.moduleInfo['memoryUse'], 'style': { 'progress': { 'background': '#D9534F' } } } ;   
            $scope.charts['Host']['Disk'] = { 'percent': $scope.moduleInfo['diskUse'] } ;
            $.each( $scope.HostList, function( index, value ){
               $scope.DiskNum = $scope.DiskNum + value['Disk'].length ;
            } ) ;
         } ) ; 

         SdbFunction.Interval(function(){
            s = parseInt(Math.random()*100000) + 100000 ;
            $scope.charts['Module']['value'] = [ [ 0, s, true, false ] ] ;
         },5000);
      }
      
      //选择图表
      $scope.changeCharts = function( type ){
         $scope.charts['Module'] = {} ;
         if( type == 'Insert' )
         {
            $scope.chartName = 'Record Insert' ;
            $scope.charts['Module']['options'] = window.SdbSacManagerConf.RecordInsertEchart ;
         }
         else if( type == 'Read' )
         {
            $scope.chartName = 'Record Read' ;
            $scope.charts['Module']['options'] = window.SdbSacManagerConf.RecordReadEchart ;
         }
         else if( type == 'Delete' )
         {
            $scope.chartName = 'Record Delete' ;
            $scope.charts['Module']['options'] = window.SdbSacManagerConf.RecordDeleteEchart ;
         }
         else if( type == 'Update' )
         {
            $scope.chartName = 'Record Update' ;
            $scope.charts['Module']['options'] = window.SdbSacManagerConf.RecordUpdateEchart ;
         }
      } ;

      //图表下拉选项
      $scope.DropdownMenu = [ 
         { 'html': $compile( '<div style="padding:5px 10px" ng-click="changeCharts(\'Insert\')">Record Insert</div>' )( $scope ) },
         { 'html': $compile( '<div style="padding:5px 10px" ng-click="changeCharts(\'Read\')">Record Read</div>' )( $scope ) },
         { 'html': $compile( '<div style="padding:5px 10px" ng-click="changeCharts(\'Delete\')">Record Remove</div>' )( $scope ) },
         { 'html': $compile( '<div style="padding:5px 10px" ng-click="changeCharts(\'Update\')">Record Update</div>' )( $scope ) }
      ] ;

      moduleList() ;
      
      $scope.result = {} ;
      var result = [] ;
      for( var i = 1; i < 2; ++i )
      {
         result.push( 
             '2016-05-23 10:06:22 [Error] - 主机Ubuntu-12-02连接失败'
         )
      } ;

      $scope.result = result ;
      
      //跳转至部署
      $scope.GotoDeploy = function(){
         $location.path( '/Deploy/Index' ) ;
      } ;

      //跳转至分区组列表
      $scope.GotoGroups = function(){
         $location.path( '/Monitor/SDB-Overview/Index' ) ;
      } ;

      //跳转至主机列表
      $scope.GotoHosts = function(){
         $location.path( '/Monitor/Host-List/Index' ) ;
      } ;

      //跳转至会话列表
      $scope.GotoSessions = function(){
         $location.path( '/Monitor/SDB-Overview/Session' ) ;
      } ;

      //跳转至域列表
      $scope.GotoDomains = function(){
         $location.path( '/Monitor/SDB-Overview/Domain' ) ;
      } ;

      //跳转至节点列表
      $scope.GotoNodes = function(){
         $location.path( '/Monitor/SDB-Overview/Node' ) ;
      } ;

      //跳转至数据库操作
      $scope.GotoDatabase = function(){
         $location.path( '/Data/SDB-Database/Index' ) ;
      } ;

   } ) ;

}());

