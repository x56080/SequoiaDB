(function(){
   var sacApp = window.SdbSacManagerModule ;
   //控制器
   sacApp.controllerProvider.register( 'Deploy.AddHost.Ctrl', function( $scope, $compile, $location, $rootScope, SdbRest ){

      //初始化
      $scope.CurrentHost      = 0 ;
      $scope.CheckedHostNum   = 0 ;
      $scope.HostDataList     = [] ;
      $scope.CpuChart         = { 'percent': 0 } ;
      $scope.MemoryChart      = { 'percent': 0 } ;
      $scope.DiskChart        = { 'percent': 0 } ;
      $scope.HostDiskGrid     = { 'titleWidth': [ '40px', 30, 30, '80px', 40 ] } ;
      $scope.HostPortGrid     = { 'titleWidth': [ 50, 50 ] } ;
      $scope.HostServiceGrid  = { 'titleWidth': [ 35, 35, 30 ] } ;
      $scope.HostSafetyGrid   = { 'titleWidth': [ 35, 35, 30 ] } ;
      $scope.HostDiskList     = [] ;
      $scope.HostPortList     = [] ;
      $scope.HostServiceList  = [] ;
      $scope.Search           = { 'text': '' } ;

      //读取部署的传参
      $scope.HostList  = $rootScope.tempData( 'Deploy', 'AddHost' ) ;
      var deployModel  = $rootScope.tempData( 'Deploy', 'Model' ) ;
      var deplpyModule = $rootScope.tempData( 'Deploy', 'Module' ) ;
      var installPath  = $rootScope.tempData( 'Deploy', 'InstallPath' ) ;
      var clusterName  = $rootScope.tempData( 'Deploy', 'ClusterName' ) ;
      if( deployModel == null || clusterName == null || installPath == null || deplpyModule == null || $scope.HostList == null )
      {
         $location.path( '/Deploy/Index' ).search( { 'r': new Date().getTime() } ) ;
         return ;
      }

      //创建步骤条
      $scope.stepList = _Deploy.BuildSdbStep( $scope, $location, deployModel, $scope['Url']['Action'], deplpyModule ) ;
      if( $scope.stepList['info'].length == 0 )
      {
         $location.path( '/Deploy/Index' ).search( { 'r': new Date().getTime() } ) ;
         return ;
      }

      //跳转到上一步
      $scope.GotoScanHost = function(){
         $location.path( '/Deploy/ScanHost' ).search( { 'r': new Date().getTime() } ) ;
      }

      //安装主机
      var installHost = function( installConfig ){
         var data = { 'cmd': 'add host', 'HostInfo': JSON.stringify( installConfig ) } ;
         SdbRest.OmOperation( data, {
            'success': function( taskInfo ){
               $rootScope.tempData( 'Deploy', 'HostTaskID', taskInfo[0]['TaskID'] ) ;
               $location.path( '/Deploy/InstallHost' ).search( { 'r': new Date().getTime() } ) ;
            },
            'failed': function( errorInfo ){
               _IndexPublic.createRetryModel( $scope, errorInfo, function(){
                  installHost( installConfig ) ;
                  return true ;
               } ) ;
            }
         } ) ;
      }

      //跳转下一步 安装主机
      $scope.GotoInstall = function(){
         if( $scope.CheckedHostNum > 0 )
         {
            var tempPoraryHosts = [] ;
            $.each( $scope.HostDataList, function( index, hostInfo ){
		         //判断是不是错误的主机
		         if( hostInfo['errno'] === 0 && hostInfo['IsUse'] === true )
		         {
			         var tempHostInfo = {} ;
                  tempHostInfo['User']          = $scope.HostList['HostInfo'][index]['User'] ;
                  tempHostInfo['Passwd']        = $scope.HostList['HostInfo'][index]['Passwd'] ;
                  tempHostInfo['SshPort']       = $scope.HostList['HostInfo'][index]['SshPort'] ;
			         tempHostInfo['AgentService']	= $scope.HostList['HostInfo'][index]['AgentService'] ;
                  tempHostInfo['HostName']		= hostInfo['HostName'] ;
			         tempHostInfo['IP']				= hostInfo['IP'] ;
			         tempHostInfo['CPU']				= hostInfo['CPU'] ;
			         tempHostInfo['Memory']			= hostInfo['Memory'] ;
			         tempHostInfo['Net']				= hostInfo['Net'] ;
			         tempHostInfo['Port']				= hostInfo['Port'] ;
			         tempHostInfo['Service']			= hostInfo['Service'] ;
			         tempHostInfo['OMA']				= hostInfo['OMA'] ;
			         tempHostInfo['Safety']			= hostInfo['Safety'] ;
			         tempHostInfo['OS']				= hostInfo['OS'] ;
			         tempHostInfo['InstallPath']	= hostInfo['InstallPath'] ;
			         tempHostInfo['Disk']				= [] ;
			         $.each( hostInfo['Disk'], function( index2, hostDisk ){
				         if( hostDisk['IsUse'] === true )
				         {
					         var tempHostDisk = {} ;
					         tempHostDisk['Name']    = hostDisk['Name'] ;
					         tempHostDisk['Mount']   = hostDisk['Mount'] ;
					         tempHostDisk['Size']    = hostDisk['Size'] ;
					         tempHostDisk['Free']    = hostDisk['Free'] ;
					         tempHostDisk['IsLocal'] = hostDisk['IsLocal'] ;
					         tempHostInfo['Disk'].push( tempHostDisk ) ;
				         }
			         } ) ;
			         tempPoraryHosts.push( tempHostInfo ) ;
		         }
	         } ) ;
            var newHostInfo = { 'ClusterName': clusterName, 'HostInfo': tempPoraryHosts, 'User': '-', 'Passwd': '-', 'SshPort': '-', 'AgentService': '-' } ;
            installHost( newHostInfo ) ;
         }
         else
         {
            $scope.Components.Confirm.type = 3 ;
            $scope.Components.Confirm.context = $scope.autoLanguage( '至少选择一台主机，才可以进入下一步操作。' ) ;
            $scope.Components.Confirm.isShow = true ;
            $scope.Components.Confirm.okText = $scope.autoLanguage( '好的' ) ;
         }
      }

      //左侧栏的过滤
      $scope.Filter = function(){
         if( $scope.Search.text.length > 0 )
         {
            $.each( $scope.HostList['HostInfo'], function( index ){
               if( $scope.HostList['HostInfo'][index]['HostName'].indexOf( $scope.Search.text ) >= 0 )
               {
                  $scope.HostList['HostInfo'][index]['IsHostName'] = true ;
               }
               else
               {
                  $scope.HostList['HostInfo'][index]['IsHostName'] = false ;
               }
               if( $scope.HostList['HostInfo'][index]['IP'].indexOf( $scope.Search.text ) >= 0 )
               {
                  $scope.HostList['HostInfo'][index]['IsIP'] = true ;
               }
               else
               {
                  $scope.HostList['HostInfo'][index]['IsIP'] = false ;
               }
            } ) ;
         }
         else
         {
            $.each( $scope.HostList['HostInfo'], function( index ){
                  $scope.HostList['HostInfo'][index]['IsHostName'] = false ;
                  $scope.HostList['HostInfo'][index]['IsIP'] = false ;
            } ) ;
         }
      } ;

      //选择颜色
      function switchColor( percent )
      {
         if( percent <= 60 )
         {
            return '#00CC66' ;
         }
         else if( percent <= 80 )
         {
            return '#FF9933' ;
         }
         else
         {
            return '#D9534F' ;
         }
      }

      //磁盘
      var checkDiskList = function(){
         var diskFree = 0 ;
         var diskSize = 0 ;
         $.each( $scope.HostDataList[$scope.CurrentHost]['Disk'], function( index, diskInfo ){
            if( diskInfo['IsLocal'] == true )
            {
               diskSize += diskInfo['Size'] ;
               diskFree += diskInfo['Free'] ;
            }
            var tmpDiskPercent = diskInfo['Size'] == 0 ? 0 : parseInt( ( diskInfo['Size'] - diskInfo['Free'] ) * 100 / diskInfo['Size'] ) ;
            $scope.HostDataList[$scope.CurrentHost]['Disk'][index]['Chart'] = {
               'percent': tmpDiskPercent,
               'style': {
                  'progress': {
                     'background': switchColor( tmpDiskPercent )
                  }
               },
               'text': sprintf( '?MB / ?MB', diskInfo['Size'] - diskInfo['Free'], diskInfo['Size'] )
            } ;
         } ) ;
         var disk = parseInt( ( diskSize - diskFree ) * 100 / diskSize ) ;
         $scope.DiskChart = {
            'percent': disk,
            'style': {
               'progress': {
                  'background': switchColor( disk )
               }
            },
            'text': sprintf( '?MB / ?MB', diskSize - diskFree, diskSize )
         } ;
         $scope.HostDiskList = $scope.HostDataList[$scope.CurrentHost]['Disk'] ;
      }

      //端口
      var checkPortList = function(){
         $scope.HostPortList = $scope.HostDataList[$scope.CurrentHost]['Port'] ;
      }

      //服务
      var checkServiceList = function(){
         $scope.HostServiceList = $scope.HostDataList[$scope.CurrentHost]['Service'] ;
      }

      //cpu
      var checkCpu = function(){
         var cpu = parseInt( ( $scope.HostDataList[$scope.CurrentHost]['CPU']['Other'] + $scope.HostDataList[$scope.CurrentHost]['CPU']['Sys'] + $scope.HostDataList[$scope.CurrentHost]['CPU']['User'] ) * 100 / ( $scope.HostDataList[$scope.CurrentHost]['CPU']['Idle'] + $scope.HostDataList[$scope.CurrentHost]['CPU']['Other'] + $scope.HostDataList[$scope.CurrentHost]['CPU']['Sys'] + $scope.HostDataList[$scope.CurrentHost]['CPU']['User'] ) ) ;
         $scope.CpuChart = { 'percent': cpu, 'style': { 'progress': { 'background': switchColor( cpu ) } } } ;
      }

      //内存
      var checkMemory = function(){
         var memory = parseInt( ( $scope.HostDataList[$scope.CurrentHost]['Memory']['Size'] - $scope.HostDataList[$scope.CurrentHost]['Memory']['Free'] ) * 100 / $scope.HostDataList[$scope.CurrentHost]['Memory']['Size'] ) ;
         $scope.MemoryChart = {
            'percent': memory,
            'style': {
               'progress': {
                  'background': switchColor( memory )
               }
            },
            'text': sprintf( '?MB / ?MB', $scope.HostDataList[$scope.CurrentHost]['Memory']['Size'] - $scope.HostDataList[$scope.CurrentHost]['Memory']['Free'], $scope.HostDataList[$scope.CurrentHost]['Memory']['Size'] )
         } ;
      }

      //检查主机
      $scope.CheckedHost = function( index ){
         if( typeof( $scope.HostDataList[index]['errno'] ) == 'undefined' || $scope.HostDataList[index]['errno'] == 0 )
         {
            if( $scope.HostDataList[index]['IsUse'] == true )
            {
               --$scope.CheckedHostNum ;
            }
            else
            {
               ++$scope.CheckedHostNum ;
            }
            $scope.HostDataList[index]['IsUse'] = !$scope.HostDataList[index]['IsUse'] ;
         }
      }

      //切换主机
      $scope.SwitchHost = function( index ){
         $scope.CurrentHost = index ;
         if( typeof( $scope.HostDataList[$scope.CurrentHost]['errno'] ) == 'undefined' || $scope.HostDataList[$scope.CurrentHost]['errno'] == 0 )
         {
            checkCpu() ;
            checkMemory() ;
            checkDiskList() ;
            checkPortList() ;
            checkServiceList() ;
         }
         else
         {
            $scope.CpuChart        = { 'percent': 0 } ;
            $scope.MemoryChart     = { 'percent': 0 } ;
            $scope.DiskChart       = { 'percent': 0 } ;
            $scope.HostDiskList    = [] ;
            $scope.HostPortList    = [] ;
            $scope.HostServiceList = [] ;
         }
         $rootScope.bindResize() ;
      }

      //创建 添加自定义路径 弹窗
      $scope.CreateAddCustomPathModel = function(){
         $scope.Components.Modal.icon = '' ;
         $scope.Components.Modal.title = $scope.autoLanguage( '添加自定义路径' ) ;
         $scope.Components.Modal.isShow = true ;
         $scope.Components.Modal.form = {
            inputList: [
               {
                  "name": "Name",
                  "webName": $scope.autoLanguage( '磁盘名' ),
                  "required": true,
                  "type": "string",
                  "value": '',
                  "valid": {
                     "min": 1
                  }
               },
               {
                  "name": "Mount",
                  "webName": $scope.autoLanguage( '挂载路径' ),
                  "required": true,
                  "type": "string",
                  "value": '',
                  "valid": {
                     "min": 1
                  }
               },
               {
                  "name": "Free",
                  "webName": $scope.autoLanguage( '可用容量(MB)' ),
                  "required": true,
                  "type": "int",
                  "value": '',
                  "valid": {
                     "min": 1
                  }
               },
               {
                  "name": "Size",
                  "webName": $scope.autoLanguage( '总容量(MB)' ),
                  "required": true,
                  "type": "int",
                  "value": '',
                  "valid": {
                     "min": 1
                  }
               }
            ]
         } ;
         $scope.Components.Modal.Context = '<div form-create para="data.form"></div>' ;
         $scope.Components.Modal.ok = function(){
            var isAllClear = $scope.Components.Modal.form.check( function( formVal ){
               var error = [] ;
               if( formVal['Free'] > formVal['Size'] )
               {
                  error.push( { 'name': 'Free', 'error': sprintf( $scope.autoLanguage( '?的值不能大于?' ), $scope.autoLanguage( '可用容量' ), $scope.autoLanguage( '总容量' ) ) } ) ;
               }
               return error ;
            } ) ;
            if( isAllClear )
            {
               var formVal = $scope.Components.Modal.form.getValue() ;
               formVal['IsUse']   = true ;
               formVal['CanUse']  = true ;
               formVal['IsLocal'] = true ;
               var tmpDiskPercent = formVal['Size'] == 0 ? 0 : parseInt( ( formVal['Size'] - formVal['Free'] ) * 100 / formVal['Size'] ) ;
               formVal['Chart'] = {
                  'percent': tmpDiskPercent,
                  'style': {
                     'progress': {
                        'background': switchColor( tmpDiskPercent )
                     }
                  },
                  'text': sprintf( '?MB / ?MB', formVal['Size'] - formVal['Free'], formVal['Size'] )
               } ;
               $scope.HostDataList[$scope.CurrentHost]['Disk'].push( formVal ) ;
               ++$scope.HostDataList[$scope.CurrentHost]['IsUseNum'] ;
               $scope.HostDataList[$scope.CurrentHost]['CanUse'] = true ;
               $scope.bindResize() ;
            }
            return isAllClear ;
         }
      }

      //计算磁盘
      $scope.CountCheckedDisk = function(){
         setTimeout( function(){
            var isUseHost = false ;
            $scope.HostDataList[$scope.CurrentHost]['IsUseNum'] = 0 ;
            $.each( $scope.HostDataList[$scope.CurrentHost]['Disk'], function( index, diskInfo ){
               if( diskInfo['IsUse'] == true )
               {
                  ++$scope.HostDataList[$scope.CurrentHost]['IsUseNum'] ;
                  isUseHost = true ;
                  return false ;
               }
            } ) ;
            if( isUseHost == true )
            {
               $scope.HostDataList[$scope.CurrentHost]['CanUse'] = true ;
            }
            else
            {
               if( $scope.HostDataList[$scope.CurrentHost]['IsUse'] == true )
               {
                  $scope.HostDataList[$scope.CurrentHost]['IsUse']  = false ;
                  --$scope.CheckedHostNum ;
               }
               $scope.HostDataList[$scope.CurrentHost]['CanUse'] = false ;
            }
            $scope.$apply() ;
         } ) ;
      }

      //获取检查主机的数据
      var checkHost = function(){
         var data = { 'cmd': 'check host', 'HostInfo': JSON.stringify( $scope.HostList ) } ;
         SdbRest.OmOperation( data, {
            'success': function( hostDataList ){
               $.each( $scope.HostList['HostInfo'], function( index, hostInfo ){
                  $.each( hostDataList, function( index2, hostDataInfo ){
                     if( hostInfo['HostName'] == hostDataInfo['HostName'] || hostInfo['IP'] == hostDataInfo['IP'] )
                     {
                        hostDataInfo['HostName'] = hostInfo['HostName'] ;
                        hostDataInfo['IP'] = hostInfo['IP'] ;
                        if( typeof( hostDataInfo['errno'] ) == 'undefined' || hostDataInfo['errno'] == 0 )
                        {
                           hostDataInfo['errno'] = 0 ;
                           hostDataInfo['CanUse'] = false ;
                           hostDataInfo['IsUseNum'] = 0 ;
                           hostDataInfo['CanNotUseNum'] = 0 ;
                           hostDataInfo['DiskWarning'] = 0 ;

                           var diskNameList = {} ;
                           $.each( hostDataInfo['Disk'], function( index3, diskInfo ){

                              if( diskNameList[diskInfo['Name']] === 0 || diskNameList[diskInfo['Name']] === 1 )
                              {
                                 diskNameList[diskInfo['Name']] = 1 ;
                              }
                              else
                              {
                                 diskNameList[diskInfo['Name']] = 0 ;
                              }

                              if( hostDataInfo['Disk'][index3]['CanUse'] == true && hostDataInfo['Disk'][index3]['IsLocal'] == true )
                              {
                                 hostDataInfo['Disk'][index3]['IsUse'] = true ;
                                 hostDataInfo['CanUse'] = true ;
                                 ++hostDataInfo['IsUseNum'] ;
                              }
                              else
                              {
                                 ++hostDataInfo['CanNotUseNum'] ;
                              }
                           } ) ;

                           $.each( hostDataInfo['Disk'], function( index3, diskInfo ){
                              if( diskNameList[diskInfo['Name']] === 1 )
                              {
                                 if( diskInfo['CanUse'] == true && diskInfo['IsLocal'] == true )
                                 {
                                    //磁盘出现大于1次
                                    diskInfo['IsUse'] = false ;
                                    if( hostDataInfo['IsUseNum'] > 0 )
                                    {
                                       --hostDataInfo['IsUseNum'] ;
                                    }
                                    if( hostDataInfo['IsUseNum'] == 0 )
                                    {
                                       hostDataInfo['CanUse'] = false ;
                                    }
                                 }
                              }
                           } ) ;

                           var isFind = true ;
                           while( isFind )
                           {
                              isFind = false ;
                              $.each( hostDataInfo['Port'], function( index3 ){
                                 if( hostDataInfo['Port'][index3]['Port'].length == 0 )
                                 {
                                    hostDataInfo['Port'].splice( index3, 1 ) ;
                                    isFind = true ;
                                    return false ;
                                 }
                              } ) ;
                           }
                           isFind = true ;
                           while( isFind )
                           {
                              isFind = false ;
                              $.each( hostDataInfo['Service'], function( index3 ){
                                 if( hostDataInfo['Service'][index3]['Name'].length == 0 )
                                 {
                                    hostDataInfo['Service'].splice( index3, 1 ) ;
                                    isFind = true ;
                                    return false ;
                                 }
                              } ) ;
                           }
                           hostDataInfo['DiskWarning'] = sprintf( $scope.autoLanguage( '有?个磁盘剩余容量不足。' ), hostDataInfo['CanNotUseNum'] ) ;
                           hostDataInfo['IsUse'] = hostDataInfo['CanUse'] ;
                           if( hostDataInfo['OMA']['Path'].length > 0 )
                           {
                              hostDataInfo['InstallPath'] = hostDataInfo['OMA']['Path'] ;
                              hostDataInfo['IsUse'] = false ;
                           }
                           else
                           {
                              hostDataInfo['InstallPath'] = installPath ;
                           }
                           if( hostDataInfo['IsUse'] == true )
                           {
                              ++$scope.CheckedHostNum ;
                           }
                        }
                        else
                        {
                           hostDataInfo['CanUse'] = false ;
                           hostDataInfo['IsUse'] = false ;
                        }
                        $scope.HostDataList.push( hostDataInfo ) ;
                        return false ;
                     }
                  } ) ;
               } ) ;
               $scope.SwitchHost( $scope.CurrentHost ) ;
            },
            'failed': function( errorInfo ){
               _IndexPublic.createRetryModel( $scope, errorInfo, function(){
                  checkHost() ;
                  return true ;
               } ) ;
            }
         } ) ;
      }

      checkHost() ;
   } ) ;
}());