(function(){
   var sacApp = window.SdbSacManagerModule ;
   //控制器
   sacApp.controllerProvider.register( 'Deploy.Task.Install.Ctrl', function( $scope, $compile, $location, $rootScope, SdbRest, SdbFunction ){

      //初始化
      $scope.ContainerBox = [ { offsetY: -70 }, { offsetY: -4 } ] ;
      $scope.HostTaskGridOptions = { 'titleWidth': [ '24px', 30, 20, 15, 35 ] } ;
      $scope.SdbTaskGridOptions  = { 'titleWidth': [ '24px', 25, 15, '100px', 15, 10, 35 ] } ;
      $scope.SsqlTaskGridOptions = { 'titleWidth': [ '24px', 30, 20, 20, 30 ] } ;
      $scope.ZkpTaskGridOptions  = { 'titleWidth': [ '24px', 15, 30, 15, 40 ] } ;
      $scope.IsFinish            = false ;
      $scope.TimeLeft            = '' ;
      $scope.BarColor            = 0 ;

      $scope.DeployType  = $rootScope.tempData( 'Deploy', 'Model' ) ;
      $scope.ModuleType  = $rootScope.tempData( 'Deploy', 'Module' ) ;
      var installTask    = $rootScope.tempData( 'Deploy', 'HostTaskID' ) ;
      if( $scope.DeployType == null || $scope.ModuleType == null || installTask == null )
      {
         $location.path( '/Deploy/Index' ).search( { 'r': new Date().getTime() } ) ;
         return ;
      }

      if( $scope.DeployType != 'Task' )
      {
         $scope.stepList = _Deploy.BuildSdbStep( $scope, $location, $scope.DeployType, $scope['Url']['Action'], $scope.ModuleType ) ;
         if( $scope.stepList['info'].length == 0 )
         {
            $location.path( '/Deploy/Index' ).search( { 'r': new Date().getTime() } ) ;
            return ;
         }
      }

      //上一步
      $scope.GotoAddHost = function(){
         $location.path( '/Deploy/AddHost' ).search( { 'r': new Date().getTime() } ) ;
      }

      //返回
      $scope.GotoDeploy2 = function(){
         $location.path( '/Deploy/Index' ).search( { 'r': new Date().getTime() } ) ;
      }

      //下一步
      $scope.GotoDeploy = function(){
         if( $scope.IsFinish == true )
         {
            $location.path( '/Deploy/Index' ).search( { 'r': new Date().getTime() } ) ;
         }
      }

      //下一步
      $scope.GotoConf = function(){
         if( $scope.IsFinish == true )
         {
            if( $scope.ModuleType == 'sequoiadb' )
            {
               $location.path( '/Deploy/SDB-Conf' ).search( { 'r': new Date().getTime() } ) ;
            }
            else if( $scope.ModuleType == 'sequoiasql' )
            {
               $location.path( '/Deploy/SSQL-Conf' ).search( { 'r': new Date().getTime() } ) ;
            }
            else if( $scope.ModuleType == 'zookeeper' )
            {
               $location.path( '/Deploy/ZKP-Mod' ).search( { 'r': new Date().getTime() } ) ;
            }
            else
            {
               $location.path( '/Deploy/Index' ).search( { 'r': new Date().getTime() } ) ;
            }
         }
      }

      //获取日志
      $scope.GetLog = function(){
         var data = { 'cmd': 'get log', 'name': './task/' + installTask + '.log' } ;
         var pre = null ;
         var div = null ;
         SdbRest.GetLog( data, function( logstr ){
            var browser = SdbFunction.getBrowserInfo() ;
            if( browser[0] == 'ie' && browser[1] == 7 )
            {
               logstr = logstr.replace( /\n/gi, '\n\r' ) ;
            }
            $scope.Components.Modal.icon = '' ;
            $scope.Components.Modal.title = $scope.autoLanguage( '日志' ) ;
            $scope.Components.Modal.isShow = true ;
            $scope.Components.Modal.Context = function( bodyEle ){
               pre = $( '<pre></pre>' ).text( logstr ).css( {
                  'padding': '10px',
                  'margin': '0',
                  'white-space': 'pre-wrap'
               } ) ;
               div = $( '<div></div>' ).addClass( 'well' ).append( pre ).css( { 'overflow-y': 'auto' } ) ;
               $( bodyEle ).html( div ) ;
            }
            $scope.Components.Modal.onResize = function( width, height ){
               if( div !== null )
               {
                  div.css( { 'height': height - 5 } ) ;
               }
            }
            $scope.Components.Modal.ok = function(){
               return true ;
            }
         }, function(){
            _IndexPublic.createRetryModel( $scope, null, function(){
               $scope.GetLog() ;
               return true ;
            }, $scope.autoLanguage( '错误' ), $scope.autoLanguage( '获取日志失败。' ) ) ;
         } ) ;
      }

      //循环查询任务信息
      var queryTask = function( taskID ){
         $rootScope.tempData( 'Deploy', 'InstallTask', taskID ) ;
         var data = { 'cmd': 'query task', 'filter': JSON.stringify( { 'TaskID': taskID } ) } ;
         SdbRest.OmOperation( data, {
            'success': function( taskInfo ){
               $scope.TimeLeft = '' ;
               if( taskInfo.length > 0 )
               {
                  $scope.TaskInfo = taskInfo[0] ;
                  $scope.BarColor = 0 ;
                  $scope.TimeLeft = 0 ;
                  var errorNum = 0 ;
                  $.each( $scope.TaskInfo['ResultInfo'], function( index, hostInfo ){
                     if( hostInfo['errno'] != 0 )
                     {
                        ++errorNum ;
                        $scope.BarColor = 1 ;
                     }
                     if( hostInfo['Status'] == 4 )
                     {
                        return true ;
                     }
                     $scope.TimeLeft += 2 ;
                  } ) ;
                  if( $scope.TaskInfo['ResultInfo'].length == errorNum )
                  {
                     $scope.BarColor = 2 ;
                  }

                  if( $scope.TimeLeft == 0 )
                  {
                     $scope.TimeLeft = 1 ;
                  }

                  if( $scope.TaskInfo['Status'] == 4 )
                  {
                     $scope.BarColor = 3 ;
                     $scope.TimeLeft = '' ;
                     $scope.IsFinish = true ;
                     if( typeof( $scope.TaskInfo['errno'] ) == 'number' && $scope.TaskInfo['errno'] != 0 )
                     {
                        $scope.TaskInfo['Progress'] = 100 ;
                        $scope.BarColor = 2 ;
                        var errorNum = 0 ;
                        $.each( $scope.TaskInfo['ResultInfo'], function( index, hostInfo ){
                           if( $scope.TaskInfo['ResultInfo'][index]['errno'] == 0 )
                           {
                              $scope.TaskInfo['ResultInfo'][index]['errno'] = 1 ;
                              ++errorNum ;
                           }
                           $scope.TaskInfo['ResultInfo'][index]['Status'] = 4 ;
                           $scope.TaskInfo['ResultInfo'][index]['StatusDesc'] = 'FINISH' ;
                        } ) ;
                        //检查如果全失败，就禁止到下一步
                        if( errorNum >= $scope.TaskInfo['ResultInfo'].length )
                        {
                           $scope.IsFinish = false ;
                        }
                     }
                  }
                  else
                  {
                     if( $scope.TaskInfo['Status'] == 2 )
                     {
                        $scope.BarColor = 1 ;
                     }
                     $scope.TimeLeft = sprintf( $scope.autoLanguage( '?分钟' ), $scope.TimeLeft ) ;
                     if( $scope.TaskInfo['Progress'] == 100 )
                     {
                        $scope.TaskInfo['Progress'] = 90 ;
                     }
                  }
               
                  $rootScope.bindResize() ;
                  $scope.$apply() ;

                  if( $scope.IsFinish == false )
                  {
                     SdbFunction.Timeout( function(){
                        queryTask( taskID ) ;
                     }, 1500 ) ;
                  }
               }
               else
               {
                  _IndexPublic.createRetryModel( $scope, null, function(){
                     queryTask( taskID ) ;
                     return true ;
                  }, $scope.autoLanguage( '获取任务信息失败' ), $scope.autoLanguage( '获取任务信息失败，该任务不存在。' ) ) ;
               }
            },
            'failed': function( errorInfo ){
               _IndexPublic.createRetryModel( $scope, errorInfo, function(){
                  queryTask( taskID ) ;
                  return true ;
               } ) ;
            }
         }, {
            'showLoading': false
         } ) ;
      }

      queryTask( installTask ) ;

   } ) ;
}());