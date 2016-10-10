(function(){
   var sacApp = window.SdbSacManagerModule ;
   //控制器
   sacApp.controllerProvider.register( 'Deploy.Sdb.Conf.Ctrl', function( $scope, $compile, $location, $rootScope, SdbRest ){

      //初始化
      var deployMode = 'distribution' ;
      $scope.IsAllClear = true ;
      $scope.Conf1 = {} ;
      $scope.Conf2 = {} ;
      $scope.HostList = [] ;
      $scope.templateList = [] ;
      $scope.ConfForm1 = { 'inputList': [] } ;
      $scope.ConfForm2 = { 'inputList': [] } ;
      $scope.currentTemplate  = {} ;
      $scope.RedundancyChart = {} ;
      $scope.HostGridOptions = { 'titleWidth': [ '30px', 50, 50 ] } ;

      $scope.RedundancyChart['options'] = $.extend( true, {}, window.SdbSacManagerConf.RedundancyChart ) ;
      $scope.RedundancyChart['options']['title']['text']     = $scope.autoLanguage( '容量信息' ) ;
      $scope.RedundancyChart['options']['legend']['data'][0] = $scope.autoLanguage( '可用容量' ) ;
      $scope.RedundancyChart['options']['legend']['data'][1] = $scope.autoLanguage( '冗余容量' ) ;
      $scope.RedundancyChart['options']['series'][0]['name'] = $scope.autoLanguage( '总容量' ) ;
      $scope.RedundancyChart['options']['series'][0]['data'][0]['name'] = $scope.autoLanguage( '可用容量' ) ;
      $scope.RedundancyChart['options']['series'][0]['data'][1]['name'] = $scope.autoLanguage( '冗余容量' ) ;

      $scope.DeployType  = $rootScope.tempData( 'Deploy', 'Model' ) ;
      var clusterName    = $rootScope.tempData( 'Deploy', 'ClusterName' ) ;
      $scope.ModuleName  = $rootScope.tempData( 'Deploy', 'ModuleName' ) ;
      if( $scope.DeployType == null || clusterName == null || $scope.ModuleName == null )
      {
         $location.path( '/Deploy/Index' ) ;
         return ;
      }

      $scope.stepList = _Deploy.BuildSdbStep( $scope, $location, $scope.DeployType, $scope['Url']['Action'], 'sequoiadb' ) ;
      if( $scope.stepList['info'].length == 0 )
      {
         $location.path( '/Deploy/Index' ) ;
         return ;
      }

      //计算冗余等信息
      var predictCapacity = function(){
         var isAllClear = $scope.ConfForm2.check() ;
         if( isAllClear )
         {
            var DeployMod = $scope.ConfForm1.getValue() ;
            var formVal = $scope.ConfForm2.getValue() ;
            $scope.currentTemplate = {
               'ClusterName':  clusterName,
               'BusinessName': $scope.ModuleName,
               'DeployMod':    DeployMod['type'],
               'BusinessType': 'sequoiadb',
               'Property': [
                  { "Name": "replicanum",   "Value": formVal['replicanum'] + '' },
                  { "Name": "datagroupnum", "Value": formVal['datagroupnum'] + '' },
                  { "Name": "catalognum",   "Value": formVal['catalognum'] + '' },
                  { "Name": "coordnum",     "Value": formVal['coordnum'] + '' }
               ],
               'HostInfo':[]
            } ;
            $scope.Conf1['DiskNum'] = 0 ;
            $scope.Conf1['HostNum'] = 0 ;
            $.each( $scope.HostList, function( index ){
               if( $scope.HostList[index]['checked'] == true )
               {
                  $scope.currentTemplate['HostInfo'].push( { 'HostName': $scope.HostList[index]['HostName'] } ) ;
                  ++$scope.Conf1['HostNum'] ;
                  $.each( $scope.HostList[index]['Disk'], function( index2, diskInfo ){
                     if( diskInfo['IsLocal'] == true )
                     {
                        ++$scope.Conf1['DiskNum'] ;
                     }
                  } ) ;
               }
            } ) ;
            if( $scope.Conf1['HostNum'] == 0 )
            {
               $scope.Conf1['HostNum'] = $scope.HostList.length ;
               $.each( $scope.HostList, function( index ){
                  $scope.currentTemplate['HostInfo'].push( { 'HostName': $scope.HostList[index]['HostName'] } ) ;
                  $.each( $scope.HostList[index]['Disk'], function( index2, diskInfo ){
                     if( diskInfo['IsLocal'] == true )
                     {
                        ++$scope.Conf1['DiskNum'] ;
                     }
                  } ) ;
               } ) ;
            }
            //计算总节点数
            if( deployMode == 'distribution' )
            {
               $scope.Conf1['SumNode'] = parseInt( formVal['replicanum'] ) * parseInt( formVal['datagroupnum'] ) + parseInt( formVal['catalognum'] ) + ( formVal['coordnum'] == 0 ? $scope.Conf1['HostNum'] : parseInt( formVal['coordnum'] ) ) ;
            }
            else
            {
               $scope.Conf1['SumNode'] = 1 ;
            }
            //计算节点利用率
            $scope.Conf1['nodeSpread'] = twoDecimalPlaces( $scope.Conf1['SumNode'] / $scope.Conf1['DiskNum'] ) ;
	         $scope.Conf1['nodeSpread'] = $scope.Conf1['nodeSpread'] < 1 ? 1 : $scope.Conf1['nodeSpread'] ;
            //获取冗余信息
            var data = { 'cmd': 'predict capacity', 'TemplateInfo': JSON.stringify( $scope.currentTemplate ) } ;
            SdbRest.OmOperation( data, function( conf2 ){
               var newConf = {} ;
               newConf['TotalSize'] = sizeConvert( conf2[0]['TotalSize'] ) ;
               newConf['ValidSize'] = sizeConvert( conf2[0]['ValidSize'] ) ;
               newConf['RedundancySize'] = sizeConvert( conf2[0]['TotalSize'] - conf2[0]['ValidSize'] ) ;
               newConf['Redundancy'] = ( conf2[0]['RedundancyRate'] * 100 ) + '%'
               $scope.Conf2 = newConf ;
               $scope.RedundancyChart = {} ;
               $scope.RedundancyChart['options'] = $.extend( true, {}, window.SdbSacManagerConf.RedundancyChart ) ;
               $scope.RedundancyChart['options']['title']['text']     = $scope.autoLanguage( '容量信息' ) ;
               $scope.RedundancyChart['options']['legend']['data'][0] = $scope.autoLanguage( '可用容量' ) ;
               $scope.RedundancyChart['options']['legend']['data'][1] = $scope.autoLanguage( '冗余容量' ) ;
               $scope.RedundancyChart['options']['series'][0]['name'] = $scope.autoLanguage( '总容量' ) + sizeConvert( conf2[0]['TotalSize'] ) ;
               $scope.RedundancyChart['options']['series'][0]['data'][0]['name'] = $scope.autoLanguage( '可用容量' ) ;
               $scope.RedundancyChart['options']['series'][0]['data'][1]['name'] = $scope.autoLanguage( '冗余容量' ) ;
               $scope.RedundancyChart['options']['series'][0]['data'][0]['value'] = conf2[0]['ValidSize'] ;
               $scope.RedundancyChart['options']['series'][0]['data'][1]['value'] = conf2[0]['TotalSize'] - conf2[0]['ValidSize'] ;              
               $scope.$apply() ;
            }, function( errorInfo ){
               _IndexPublic.createRetryModel( $scope, errorInfo, function(){
                  predictCapacity() ;
                  return true ;
               } ) ;
            }, function(){
               _IndexPublic.createErrorModel( $scope, $scope.autoLanguage( '网络连接错误，请尝试按F5刷新浏览器。' ) ) ;
            } ) ;
         }
         $scope.IsAllClear = isAllClear ;
      }

      //获取主机列表
      var getHostList = function(){
         var filter = { "ClusterName": clusterName } ;
         var data = { 'cmd': 'query host', 'filter': JSON.stringify( filter ) } ;
         SdbRest.OmOperation( data, function( hostList ){
            $scope.HostList = hostList ;
            $scope.Conf1['DiskNum'] = 0 ;
            $.each( $scope.HostList, function( index, hostInfo ){
               $scope.HostList[index]['checked'] = true ;
               $.each( hostInfo['Disk'], function( index2, diskInfo ){
                  if( diskInfo['IsLocal'] == true )
                  {
                     ++$scope.Conf1['DiskNum'] ;
                  }
               } ) ;
            } )  ;
            getBusinessTemplate() ;
            //$scope.$apply() ;
         }, function( errorInfo ){
            _IndexPublic.createRetryModel( $scope, errorInfo, function(){
               getHostList() ;
               return true ;
            } ) ;
         }, function(){
            _IndexPublic.createErrorModel( $scope, $scope.autoLanguage( '网络连接错误，请尝试按F5刷新浏览器。' ) ) ;
         } ) ;
      }

      //获取业务模板
      var getBusinessTemplate = function(){
         var data = { 'cmd': 'get business template', 'BusinessType': 'sequoiadb' } ;
         SdbRest.OmOperation( data, function( templateList ){
            $.each( templateList, function( index ){
               templateList[index]['Property'] = _Deploy.ConvertTemplate( templateList[index]['Property'] ) ;
            } ) ;
            var confForm = {
               'inputList': [
                  {
                     "name": "type",
                     "webName": $scope.autoLanguage( '部署模式' ),
                     "type": "select",
                     "value": "",
                     "valid": [],
                     "onChange": function( name, key, value ){
                        deployMode = value ;
                        if( deployMode == 'distribution' )
                        {
                           $.each( $scope.HostList, function( index ){
                              $scope.HostList[index]['checked'] = true ;
                           } ) ;
                        }
                        else
                        {
                           $.each( $scope.HostList, function( index ){
                              $scope.HostList[index]['checked'] = index == 0 ;
                           } ) ;
                        }
                        $.each( $scope.templateList, function( index, template ){
                           if( template['DeployMod'] == value )
                           {
                              $.each( template['Property'], function( index2 ){
                                 template['Property'][index2]['onChange'] = function(){
                                    predictCapacity() ;
                                 }
                              } ) ;
                              $scope.ConfForm2 = { 'inputList': template['Property'] } ;
                              return false ;
                           }
                        } ) ;
                        setTimeout( predictCapacity ) ;
                     }
                  }
               ]
            } ;
            $.each( templateList, function( index, template ){
               if( index == 0 )
               {
                  confForm['inputList'][0]['value'] = template['DeployMod'] ;
                  $.each( template['Property'], function( index2 ){
                     template['Property'][index2]['onChange'] = function(){
                        predictCapacity() ;
                     }
                  } ) ;
                  $scope.ConfForm2 = { 'inputList': template['Property'] } ;
               }
               confForm['inputList'][0]['valid'].push( { 'key': template['WebName'], 'value': template['DeployMod'] } ) ;
            } ) ;
            $scope.templateList = templateList ;
            $scope.ConfForm1 = confForm ;
            $scope.$apply() ;
            setTimeout( predictCapacity ) ;
         }, function( errorInfo ){
            _IndexPublic.createRetryModel( $scope, errorInfo, function(){
               getBusinessTemplate() ;
               return true ;
            } ) ;
         }, function(){
            _IndexPublic.createErrorModel( $scope, $scope.autoLanguage( '网络连接错误，请尝试按F5刷新浏览器。' ) ) ;
         } ) ;
      }

      //返回
      $scope.GotoDeploy = function(){
         $location.path( '/Deploy/Index' ) ;
      }

      //上一步
      $scope.GotoScanHost = function(){
         $location.path( '/Deploy/Install' ) ;
      }

      //下一步
      $scope.GotoModSdb = function(){
         if( $scope.IsAllClear == true )
         {
            $rootScope.tempData( 'Deploy', 'ModuleConfig', $scope.currentTemplate ) ;
            $location.path( '/Deploy/SDB-Mod' ) ;
         }
      }

      $scope.SelectAll = function(){
         $.each( $scope.HostList, function( index ){
            $scope.HostList[index]['checked'] = true ;
         } ) ;
      }

      $scope.Unselect = function(){
         $.each( $scope.HostList, function( index ){
            $scope.HostList[index]['checked'] = !$scope.HostList[index]['checked'] ;
         } ) ;
      }

      //单机模式用单选
      $scope.HostRadio = function(){
         if( deployMode == 'standalone' )
         {
            $.each( $scope.HostList, function( index ){
               if( $scope.HostList[index]['checked'] == true )
               {
                  $scope.HostList[index]['checked'] = false ;
               }
            } ) ;
         }
      }
  
      //创建 选择安装业务的主机 弹窗
      $scope.CreateSwitchHostModel = function(){
         var hostBox = null ;
         var grid = null ;
         var tempHostList = $.extend( true, [], $scope.HostList ) ;
         $scope.Components.Modal.icon = '' ;
         $scope.Components.Modal.title = $scope.autoLanguage( '主机列表' ) ;
         $scope.Components.Modal.isShow = true ;
         $scope.Components.Modal.Context = function( bodyEle ){
            var div  = $( '<div></div>' ) ;
            if( deployMode == 'distribution' )
            {
               var btn1 = $compile( '<button ng-click="SelectAll()"></button>' )( $scope ).addClass( 'btn btn-default' ).text( $scope.autoLanguage( '全选' ) ) ;
               var btn2 = $compile( '<button ng-click="Unselect()"></button>' )( $scope ).addClass( 'btn btn-default' ).text( $scope.autoLanguage( '反选' ) ) ;
               div.append( btn1 ).append( '&nbsp;' ).append( btn2 ) ;
            }

            hostBox = $( '<div></div>' ).css( { 'marginTop': '10px' } ) ;

            grid = $compile( '\
<div class="Grid" style="border-bottom:1px solid #E3E7E8;" ng-grid="HostGridOptions"">\
   <div class="GridHeader">\
      <div class="GridTr">\
         <div class="GridTd Ellipsis"></div>\
         <div class="GridTd Ellipsis">{{autoLanguage("主机名")}}</div>\
         <div class="GridTd Ellipsis">{{autoLanguage("IP地址")}}</div>\
         <div class="clear-float"></div>\
      </div>\
   </div>\
   <div class="GridBody">\
      <div class="GridTr" ng-repeat="hostInfo in HostList track by $index">\
         <div class="GridTd Ellipsis" style="word-break:break-all;">\
            <input type="checkbox" ng-model="HostList[$index][\'checked\']" ng-click="HostRadio()"/>\
         </div>\
         <div class="GridTd Ellipsis" style="word-break:break-all;">{{hostInfo[\'HostName\']}}</div>\
         <div class="GridTd Ellipsis" style="word-break:break-all;">{{hostInfo[\'IP\']}}</div>\
         <div class="clear-float"></div>\
      </div>\
   </div>\
</div>' )( $scope ) ;

            hostBox.append( grid ) ;
            $compile( bodyEle )( $scope ).append( div ).append( hostBox ) ;
            //$scope.$apply() ;
         }
         $scope.Components.Modal.onResize = function( width, height ){
            $( grid ).css( {
               'width': width - 10,
               'max-height': height - 40
            } ) ;
            $scope.bindResize() ;
         }
         $scope.Components.Modal.ok = function(){
            predictCapacity() ;
            return true ;
         }
         $scope.Components.Modal.close = function(){
            $scope.HostList = tempHostList ;
            return true ;
         }
      }

      getHostList() ;

   } ) ;
}());