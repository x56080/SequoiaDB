(function(){
   var sacApp = window.SdbSacManagerModule ;
   //控制器
   sacApp.controllerProvider.register( 'Monitor.SdbNode.Index.Ctrl', function( $scope, $compile, SdbRest, SdbFunction ){
      var clusterName = SdbFunction.LocalData( 'SdbClusterName' ) ;
      var moduleType = SdbFunction.LocalData( 'SdbModuleType' ) ;
      var moduleMode = SdbFunction.LocalData( 'SdbModuleMode' ) ;
      var moduleName = SdbFunction.LocalData( 'SdbModuleName' ) ;
      var HostName = SdbFunction.LocalData( 'SdbHostName' ) ;
      var svcname = SdbFunction.LocalData( 'SdbServiceName' ) ;
      $scope.clusterName = clusterName ;
      $scope.moduleName = moduleName ;
      $scope.moduleType = moduleType ;
      $scope.TotalRecords = 0 ;
      $scope.TotalLobs = 0 ;
      $scope.TotalCl = 0 ;
      var s = 0 ;
      var sql = '' ;

      //节点信息，后期根据 跳转函数 获取节点名（主机名+端口号）
      

      var getChartInfo = function(){
         var chartInfo = { 'TotalInsert':0, 'TotalUpdate': 0, 'TotalDelete':0, 'TotalRead':0 } ;
         var SumInfo = {} ;
         sql = 'SELECT TotalInsert, TotalRead, TotalDelete, TotalUpdate FROM $SNAPSHOT_DB WHERE HostName="' + HostName +'" AND svcname="'+ svcname + '"' ;
         SdbFunction.Interval( function(){
            SdbRest.Exec( sql, function( DbInfo ){
               DbInfo = DbInfo[0] ;
               if( typeof( SumInfo['TotalInsert'] ) == 'undefined' )
               {
                  SumInfo['TotalInsert'] = DbInfo['TotalInsert'] ;
                  SumInfo['TotalUpdate'] = DbInfo['TotalUpdate'] ;
                  SumInfo['TotalDelete'] = DbInfo['TotalDelete'] ;
                  SumInfo['TotalRead'] = DbInfo['TotalRead'] ;
               }
               else
               {
                  $scope.charts['Insert']['value'] = [ [ 0, ( DbInfo['TotalInsert'] - SumInfo['TotalInsert'] )/5, true, false ] ] ;
                  $scope.charts['Update']['value'] = [ [ 0, ( DbInfo['TotalUpdate'] - SumInfo['TotalUpdate'] )/5, true, false ] ] ;
                  $scope.charts['Delete']['value'] = [ [ 0, ( DbInfo['TotalDelete'] - SumInfo['TotalDelete'] )/5, true, false ] ] ;
                  $scope.charts['Query']['value'] = [ [ 0, ( DbInfo['TotalRead'] - SumInfo['TotalRead'] )/5, true, false ] ] ;

                  SumInfo['TotalInsert'] = DbInfo['TotalInsert'] ;
                  SumInfo['TotalUpdate'] = DbInfo['TotalUpdate'] ;
                  SumInfo['TotalDelete'] = DbInfo['TotalDelete'] ;
                  SumInfo['TotalRead'] = DbInfo['TotalRead'] ;
               }

            }, function( errorInfo ){
               _IndexPublic.createRetryModel( $scope, errorInfo, function(){
                  getDbInfo() ;
                  return true ;
               } ) ;
            }, function(){
               _IndexPublic.createErrorModel( $scope, $scope.autoLanguage( '网络连接错误，请尝试按F5刷新浏览器。' ) ) ;
            }, null, false ) ;
         }, 5000 ) ;
      } ;

      var getDbInfo = function(){
         sql = 'SELECT NodeName, Role, IsPrimary, GroupName, HostName, Disk, NodeID, ServiceStatus, TotalInsert, TotalRead, TotalDelete, TotalUpdate FROM $SNAPSHOT_DB WHERE HostName="' + HostName +'" AND svcname="'+ svcname + '"' ;
         SdbRest.Exec( sql, function( DbInfo ){
            $scope.nodeInfo = DbInfo[0] ;
            //LSN无法获取
            $scope.nodeInfo['TotalRecords'] = $scope.TotalRecords ;
            $scope.nodeInfo['TotalLobs'] = $scope.TotalLobs ;
            $scope.nodeInfo['TotalCl'] = $scope.TotalCl ;
            getChartInfo()
         }, function( errorInfo ){
            _IndexPublic.createRetryModel( $scope, errorInfo, function(){
               getDbInfo() ;
               return true ;
            } ) ;
         }, function(){
            _IndexPublic.createErrorModel( $scope, $scope.autoLanguage( '网络连接错误，请尝试按F5刷新浏览器。' ) ) ;
         } ) ;
      } ;

      var getClList = function(){
         sql = 'SELECT t1.Name,t1.Details.TotalRecords, t1.Details.TotalLobs,t1.Details.NodeName FROM (SELECT Name, Details FROM $SNAPSHOT_CL WHERE HostName="' + HostName +'" AND svcname="'+ svcname + '"' + ' SPLIT By Details) As t1'
         SdbRest.Exec( sql, function( ClList ){
            $.each( ClList, function( index, ClInfo ){
               $scope.TotalRecords += ClInfo['TotalRecords'] ;
               $scope.TotalLobs += ClInfo['TotalLobs'] ;
            } ) ;
            $scope.TotalCl = ClList.length ;

            $scope.ClList = ClList ;
            getDbInfo()
         }, function( errorInfo ){
            _IndexPublic.createRetryModel( $scope, errorInfo, function(){
               getClInfo() ;
               return true ;
            } ) ;
         }, function(){
            _IndexPublic.createErrorModel( $scope, $scope.autoLanguage( '网络连接错误，请尝试按F5刷新浏览器。' ) ) ;
         } ) ;
      } ;

      getClList()
      

      $scope.charts = {};
      $scope.charts['Insert'] = {} ;
      $scope.charts['Insert']['options'] = window.SdbSacManagerConf.RecordInsertEchart ;

      $scope.charts['Update'] = {} ;
      $scope.charts['Update']['options'] = window.SdbSacManagerConf.RecordUpdateEchart ;

      $scope.charts['Delete'] = {} ;
      $scope.charts['Delete']['options'] = window.SdbSacManagerConf.RecordDeleteEchart ;

      $scope.charts['Query'] = {} ;
      $scope.charts['Query']['options'] = window.SdbSacManagerConf.RecordReadEchart ;
   } ) ;
}());