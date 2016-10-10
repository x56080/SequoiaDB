(function(){
   var sacApp = window.SdbSacManagerModule ;
   //控制器
   sacApp.controllerProvider.register( 'Deploy.Sdb.Mod.Ctrl', function( $scope, $compile, $location, $rootScope, SdbRest ){

      //初始化
      $scope.NodeListOptions = {
         'titleWidth': [ '26px', 23, 18, 31, 10, 18 ]
      } ;
      $scope.GroupList          = [] ;
      $scope.search_hostname    = { 'text': '' } ;
      $scope.search_port        = { 'text': '' } ;
      $scope.search_path        = { 'text': '' } ;
      $scope.search_role        = { 'text': '' } ;
      $scope.search_group       = { 'text': '' } ;
      $scope.search_groupList   = [] ;
      $scope.search_roleList    = [
         { 'key': 'coord', 'value': 'coord' },
         { 'key': 'catalog', 'value': 'catalog' },
         { 'key': 'data', 'value': 'data' }
      ] ;
      $scope.search_group_disabled = false ;
      $scope.NodeList           = [] ;
      $scope.Template           = [] ;
      $scope.installConfig      = {} ;
      $scope.StandaloneShow     = 1 ;
      $scope.StandaloneForm1 = {
         'keyWidth': '160px',
         'inputList': []
      } ;
      $scope.StandaloneForm2 = {
         'keyWidth': '160px',
         'inputList': []
      } ;
      $scope.StandaloneForm3 = {
         'keyWidth': '160px',
         'inputList': []
      } ;

      $scope.Configure   = $rootScope.tempData( 'Deploy', 'ModuleConfig' ) ;
      var deployType     = $rootScope.tempData( 'Deploy', 'Model' ) ;
      var clusterName    = $rootScope.tempData( 'Deploy', 'ClusterName' ) ;
      $scope.ModuleName  = $rootScope.tempData( 'Deploy', 'ModuleName' ) ;
      if( deployType == null || clusterName == null || $scope.ModuleName == null || $scope.Configure == null )
      {
         $location.path( '/Deploy/Index' ) ;
         return ;
      }

      $scope.stepList = _Deploy.BuildSdbStep( $scope, $location, deployType, $scope['Url']['Action'], 'sequoiadb' ) ;
      if( $scope.stepList['info'].length == 0 )
      {
         $location.path( '/Deploy/Index' ) ;
         return ;
      }

      //单机模式，切换普通和高级
      $scope.SwitchParam = function( type ){
         $scope.StandaloneShow = type ;
      }

      //过滤节点列表
      function filterNodeList()
      {
         if( $scope.search_role.text == 'coord' || $scope.search_role.text == 'catalog' )
         {
            $scope.search_group.text = '' ;
            $scope.search_group_disabled = true ;
         }
         else
         {
            $scope.search_group_disabled = false ;
         }
         $.each( $scope.NodeList, function( index, nodeInfo ){
            if( ( $scope.search_hostname.text.length > 0 && nodeInfo['HostName'].indexOf( $scope.search_hostname.text ) < 0 ) ||
                  ( $scope.search_port.text.length > 0 && nodeInfo['svcname'].indexOf( $scope.search_port.text ) < 0 ) ||
                  ( $scope.search_path.text.length > 0 && nodeInfo['dbpath'].indexOf( $scope.search_path.text ) < 0 ) ||
                  ( ( $scope.search_role.text != '' && $scope.search_role.text != null ) && nodeInfo['role'] != $scope.search_role.text ) ||
                  ( ( $scope.search_group.text != '' && $scope.search_group.text != null ) && nodeInfo['datagroupname'] != $scope.search_group.text ) )
            {
               $scope.NodeList[index]['show'] = false ;
            }
            else
            {
               $scope.NodeList[index]['show'] = true ;
            }
         } ) ;
         setTimeout( function(){
            $scope.NodeListOptions.onResize() ;
         } ) ;
      }

      //创建 创建分区组 弹窗
      $scope.CreateGroupModel = function(){
         $scope.Components.Modal.icon = '' ;
         $scope.Components.Modal.title = $scope.autoLanguage( '创建分区组' ) ;
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
                     "ban": [ ".", "$", 'SYSCatalogGroup', 'SYSCoord' ]
                  }
               }
            ]
         } ;
         $scope.Components.Modal.Context = '<div form-create para="data.form"></div>' ;
         $scope.Components.Modal.ok = function(){
            var isAllClear = $scope.Components.Modal.form.check( function( thisValue ){
               var isFind = false ;
               $.each( $scope.GroupList, function( index, groupInfo ){
                  if( groupInfo['role'] == 'data' && groupInfo['groupName'] == thisValue['groupName'] )
                  {
                     isFind = true ;
                     return false ;
                  }
               } ) ;
               if( isFind == true )
               {
                  return [ { 'name': 'groupName', 'error': $scope.autoLanguage( '分区组已经存在' ) } ] ;
               }
               return [] ;
            } ) ;
            if( isAllClear )
            {
               var formVal = $scope.Components.Modal.form.getValue() ;
               countGroup( 'data', formVal['groupName'], 0 ) ;
            }
            return isAllClear ;
         }
      }

      //创建 设置节点配置 弹窗
      $scope.CreateSetNodeConfModel = function( type, groupIndex, hostIndex, nodeIndex, isShow ){
         $scope.Components.Modal.icon = '' ;
         $scope.Components.Modal.title = $scope.autoLanguage( '编辑节点配置' ) ;
         $scope.Components.Modal.ShowType = 1 ;
         $scope.Components.Modal.form1 = {
            'keyWidth': '160px',
            'inputList': _Deploy.ConvertTemplate( $scope.Template, 0 )
         } ;
         if( $scope.Configure['DeployMod'] == 'distribution' )
         {
            $scope.Components.Modal.form1['inputList'][0]['valid'] = {} ;
         }
         $scope.Components.Modal.form2 = {
            'keyWidth': '160px',
            'inputList': _Deploy.ConvertTemplate( $scope.Template, 1 )
         } ;
         $scope.Components.Modal.form3 = {
            'keyWidth': '160px',
            'inputList': [
               {
                  "name": "other",
                  "webName": $scope.autoLanguage( "自定义配置" ),
                  "type": "list",
                  "valid": {
                     "min": 0
                  },
                  "child": [
                     [
                        {
                           "name": "name",
                           "webName": $scope.autoLanguage( "参数名" ), 
                           "placeholder": $scope.autoLanguage( "参数名" ),
                           "type": "string",
                           "valid": {
                              "min": 1
                           },
                           "default": "",
                           "value": ""
                        },
                        {
                           "name": "value",
                           "webName": $scope.autoLanguage( "值" ), 
                           "placeholder": $scope.autoLanguage( "值" ),
                           "type": "string",
                           "valid": {
                              "min": 1
                           },
                           "default": "",
                           "value": ""
                        }
                     ]
                  ]
               }
            ]
         } ;
         $scope.Components.Modal.Switch1 = function(){
            $scope.Components.Modal.ShowType = 1 ;
         }
         $scope.Components.Modal.Switch2 = function(){
            $scope.Components.Modal.ShowType = 2 ;
         }
         $scope.Components.Modal.Switch3 = function(){
            $scope.Components.Modal.ShowType = 3 ;
         }
         if( type == 1 )
         {
            //空白配置
            $.each( $scope.Components.Modal.form1['inputList'], function( index ){
               if( $scope.Components.Modal.form1['inputList'][index]['type'] != 'select' )
               {
                  $scope.Components.Modal.form1['inputList'][index]['value'] = '' ;
               }
            } ) ;
            $.each( $scope.Components.Modal.form2['inputList'], function( index ){
               if( $scope.Components.Modal.form2['inputList'][index]['type'] != 'select' )
               {
                  $scope.Components.Modal.form2['inputList'][index]['value'] = '' ;
               }
            } ) ;
         }
         else if( type == 2 || type == 3 )
         {
            var loadName = [] ;
            //加载指定某个节点的配置
            $.each( $scope.Components.Modal.form1['inputList'], function( index ){
               var name = $scope.Components.Modal.form1['inputList'][index]['name'] ;
               loadName.push( name.toLowerCase() ) ;
               $scope.Components.Modal.form1['inputList'][index]['value'] = $scope.NodeList[nodeIndex][name] ;
            } ) ;
            $.each( $scope.Components.Modal.form2['inputList'], function( index ){
               var name = $scope.Components.Modal.form2['inputList'][index]['name'] ;
               loadName.push( name.toLowerCase() ) ;
               $scope.Components.Modal.form2['inputList'][index]['value'] = $scope.NodeList[nodeIndex][name] ;
            } ) ;
            //加载自定义配置项
            var isFirst = true ;
            $.each( $scope.NodeList[nodeIndex], function( key, value ){
               if( key.toLowerCase() != 'hostname' &&
                   key.toLowerCase() != 'datagroupname' &&
                   key.toLowerCase() != 'role' &&
                   key.toLowerCase() != 'checked' &&
                   key.toLowerCase() != 'show' &&
                   loadName.indexOf( key.toLowerCase() ) == -1 )
               {
                  if( isFirst )
                  {
                     $scope.Components.Modal.form3['inputList'][0]['child'][0][0]['value'] = key ;
                     $scope.Components.Modal.form3['inputList'][0]['child'][0][1]['value'] = value ;
                     isFirst = false ;
                  }
                  else
                  {
                     var newInput = $.extend( true, [], $scope.Components.Modal.form3['inputList'][0]['child'][0] ) ;
                     newInput[0]['value'] = key ;
                     newInput[1]['value'] = value ;
                     $scope.Components.Modal.form3['inputList'][0]['child'].push( newInput ) ;
                  }
               }
            } ) ;
         }
         else if( type == 4 )
         {
            var loadName = [] ;
            //批量加载配置
            var sum = 0 ;
            $.each( $scope.NodeList, function( index ){
               if( $scope.NodeList[index]['checked'] == true && $scope.NodeList[index]['show'] != false )
               {
                  ++sum ;
               }
            } ) ;
            if( sum == 0 )
            {
               _IndexPublic.createInfoModel( $scope, $scope.autoLanguage( '修改配置至少需要选择一个节点。' ), $scope.autoLanguage( '好的' ) ) ;
               return ;
            }
            $.each( $scope.Components.Modal.form1['inputList'], function( index ){
               var isFirst = true ;
               var name = $scope.Components.Modal.form1['inputList'][index]['name'] ;
               loadName.push( name.toLowerCase() ) ;
               var value = '' ;
               var offset = null ;
               $.each( $scope.NodeList, function( index2 ){
                  if( $scope.NodeList[index2]['checked'] == true && $scope.NodeList[index2]['show'] != false )
                  {
                     if( name == 'dbpath' )
                     {
                        if( isFirst == true )
                        {
                           value = selectDBPath( $scope.NodeList[index2]['dbpath'], $scope.NodeList[index2]['role'], $scope.NodeList[index2]['svcname'], $scope.NodeList[index2]['datagroupname'], $scope.NodeList[index2]['HostName'] ) ;
                           isFirst = false ;
                        }
                        if( value != selectDBPath( $scope.NodeList[index2]['dbpath'], $scope.NodeList[index2]['role'], $scope.NodeList[index2]['svcname'], $scope.NodeList[index2]['datagroupname'], $scope.NodeList[index2]['HostName'] ) )
                        {
                           value = '' ;
                           return false ;
                        }
                     }
                     else if( name == 'svcname' )
                     {
                        if( isFirst == true )
                        {
                           value = $scope.NodeList[index2]['svcname'] ;
                           isFirst = false ;
                        }
                        else
                        {
                           if( offset == null )
                           {
                              offset = parseInt( $scope.NodeList[index2]['svcname'] ) - parseInt( $scope.NodeList[index2-1]['svcname'] ) ;
                              if( offset != 0 )
                              {
                                 value = value + '[' + ( offset > 0 ? '+' : '' ) + offset + ']' ;
                              }
                           }
                           else
                           {
                              if( offset != parseInt( $scope.NodeList[index2]['svcname'] ) - parseInt( $scope.NodeList[index2-1]['svcname'] ) )
                              {
                                 value = '' ;
                                 return false ;
                              }
                           }
                        }
                     }
                     else
                     {
                        if( isFirst == true )
                        {
                           value = $scope.NodeList[index2][name] ;
                           isFirst = false ;
                        }
                        if( value != $scope.NodeList[index2][name] )
                        {
                           value = '' ;
                           return false ;
                        }
                     }
                  }
               } ) ;
               $scope.Components.Modal.form1['inputList'][index]['value'] = value ;
            } ) ;
            $.each( $scope.Components.Modal.form2['inputList'], function( index ){
               var isFirst = true ;
               var name = $scope.Components.Modal.form2['inputList'][index]['name'] ;
               loadName.push( name.toLowerCase() ) ;
               var value = '' ;
               $.each( $scope.NodeList, function( index2 ){
                  if( $scope.NodeList[index2]['checked'] == true && $scope.NodeList[index2]['show'] != false )
                  {
                     if( isFirst == true )
                     {
                        value = $scope.NodeList[index2][name] ;
                        isFirst = false ;
                     }
                     if( value != $scope.NodeList[index2][name] )
                     {
                        value = '' ;
                        return false ;
                     }
                  }
               } ) ;
               $scope.Components.Modal.form2['inputList'][index]['value'] = value ;
            } ) ;
            //加载自定义配置项
            var customConfig = [] ;
            $.each( $scope.NodeList, function( nodeIndex ){
               if( $scope.NodeList[nodeIndex]['checked'] == true && $scope.NodeList[nodeIndex]['show'] != false )
               {
                  $.each( $scope.NodeList[nodeIndex], function( key, value ){
                     if( key.toLowerCase() != 'hostname' &&
                         key.toLowerCase() != 'datagroupname' &&
                         key.toLowerCase() != 'role' &&
                         key.toLowerCase() != 'checked' &&
                         key.toLowerCase() != 'show' &&
                         loadName.indexOf( key.toLowerCase() ) == -1 &&
                         customConfig.indexOf( key.toLowerCase() ) == -1 )
                     {
                        customConfig.push( key ) ;
                     }
                  } ) ;
               }
            } ) ;
            var isFirst = true ;
            $.each( customConfig, function( customIndex, config ){
               var value = '' ;
               var isFirst2 = true ;
               $.each( $scope.NodeList, function( nodeIndex ){
                  if( $scope.NodeList[nodeIndex]['checked'] == true && $scope.NodeList[nodeIndex]['show'] != false )
                  {
                     if( isFirst2 == true )
                     {
                        value = $scope.NodeList[nodeIndex][config] ;
                        isFirst2 = false ;
                     }
                     if( value != $scope.NodeList[nodeIndex][config] )
                     {
                        value = '' ;
                        return false ;
                     }
                  }
               } ) ;
               if( isFirst )
               {
                  $scope.Components.Modal.form3['inputList'][0]['child'][0][0]['value'] = config ;
                  $scope.Components.Modal.form3['inputList'][0]['child'][0][1]['value'] = value ;
                  isFirst = false ;
               }
               else
               {
                  var newInput = $.extend( true, [], $scope.Components.Modal.form3['inputList'][0]['child'][0] ) ;
                  newInput[0]['value'] = config ;
                  newInput[1]['value'] = value ;
                  $scope.Components.Modal.form3['inputList'][0]['child'].push( newInput ) ;
               }
            } ) ;
         }
         if( isShow )
         {
            $scope.Components.Modal.isShow = true ;
         }
         else
         {
            $scope.Components.Modal.isRepaint = new Date().getTime() ;
         }
         $scope.Components.Modal.Context = '\
<div class="underlineTab" style="height:50px;">\
   <ul class="left">\
      <li ng-class="{true:\'active\'}[data.ShowType == 1]">\
         <a class="linkButton" ng-click="data.Switch1()">' + $scope.autoLanguage( '普通' ) + '</a>\
      </li>\
      <li ng-class="{true:\'active\'}[data.ShowType == 2]">\
         <a class="linkButton" ng-click="data.Switch2()">' + $scope.autoLanguage( '高级' ) + '</a>\
      </li>\
      <li ng-class="{true:\'active\'}[data.ShowType == 3]">\
         <a class="linkButton" ng-click="data.Switch3()">' + $scope.autoLanguage( '自定义' ) + '</a>\
      </li>\
   </ul>\
</div>\
<div form-create para="data.form1" ng-show="data.ShowType == 1"></div>\
<div form-create para="data.form2" ng-show="data.ShowType == 2"></div>\
<div form-create para="data.form3" ng-show="data.ShowType == 3"></div>' ;
         $scope.Components.Modal.ok = function(){
            var isAllClear1 = $scope.Components.Modal.form1.check( function( valueList ){
               var error = [] ;
               if( type != 4 && valueList['dbpath'].length == 0 )
               {
                  error.push( { 'name': 'dbpath', 'error': $scope.autoLanguage( '数据路径不能为空。' ) } ) ;
               }
               if( type != 4 && valueList['svcname'].length == 0 )
               {
                  error.push( { 'name': 'svcname', 'error': $scope.autoLanguage( '服务名不能为空。' ) } ) ;
               }
               else if( type != 4 && portEscape( valueList['svcname'], 0 ) == null )
               {
                  error.push( { 'name': 'svcname', 'error': $scope.autoLanguage( '服务名格式错误。' ) } ) ;
               }
               return error ;
            } ) ;
            var isAllClear2 = $scope.Components.Modal.form2.check() ;
            var isAllClear3 = $scope.Components.Modal.form3.check( function( valueList ){
               var error = [] ;
               $.each( valueList['other'], function( index, configInfo ){
                  if( configInfo['name'].toLowerCase() == 'hostname' ||
                      configInfo['name'].toLowerCase() == 'datagroupname' ||
                      configInfo['name'].toLowerCase() == 'role' ||
                      configInfo['name'].toLowerCase() == 'checked' ||
                      configInfo['name'].toLowerCase() == 'show' )
                  {
                     error.push( { 'name': 'other', 'error': $scope.autoLanguage( '自定义配置不能设置HostName、datagroupname、role、checked、show。' ) } ) ;
                     return false ;
                  }
               } )
               return error ;
            } ) ;
            if( isAllClear1 && isAllClear2 && isAllClear3 )
            {
               var formVal1 = $scope.Components.Modal.form1.getValue() ;
               var formVal2 = $scope.Components.Modal.form2.getValue() ;
               var formVal3 = $scope.Components.Modal.form3.getValue() ;
               var formVal = $.extend( true, formVal1, formVal2 ) ;
               $.each( formVal3['other'], function( index, configInfo ){
                  if( configInfo['name'] != '' )
                  {
                     formVal[ configInfo['name'] ] = configInfo['value'] ;
                  }
               } ) ;
               if( type == 0 || type == 1 || type == 2 )
               {
                  //创建节点
                  ++$scope.GroupList[groupIndex]['nodeNum'] ;
                  formVal['HostName'] = $scope.Configure['HostInfo'][hostIndex]['HostName'] ;
                  formVal['datagroupname'] = $scope.GroupList[groupIndex]['groupName'] ;
                  formVal['role'] = $scope.GroupList[groupIndex]['role'] ;
                  formVal['svcname'] = portEscape( formVal['svcname'], 0 ) ;
                  formVal['dbpath'] = dbpathEscape( formVal['dbpath'], formVal['HostName'], formVal['svcname'], formVal['role'], formVal['datagroupname'] ) ;
                  $scope.NodeList.push( formVal ) ;
                  if( $scope.GroupList[groupIndex]['role'] != 'coord' && $scope.GroupList[groupIndex]['nodeNum'] >= 7 )
                  {
                     $scope.GroupList[groupIndex]['DropdownMenu'][0]['disabled'] = true ;
                  }
                  if( $scope.GroupList[groupIndex]['nodeNum'] > 0 )
                  {
                     $scope.GroupList[groupIndex]['DropdownMenu'][1]['disabled'] = false ;
                  }
                  $scope.bindResize() ;
               }
               else if( type == 3 )
               {
                  //保存单个节点配置
                  formVal['svcname'] = portEscape( formVal['svcname'], 0 ) ;
                  formVal['dbpath']  = dbpathEscape( formVal['dbpath'], formVal['HostName'], formVal['svcname'], $scope.NodeList[nodeIndex]['role'], formVal['datagroupname'] ) ;
                  $scope.NodeList[nodeIndex] = {
                     'HostName': $scope.NodeList[nodeIndex]['HostName'],
                     'datagroupname': $scope.NodeList[nodeIndex]['datagroupname'],
                     'role': $scope.NodeList[nodeIndex]['role'],
                     'checked': $scope.NodeList[nodeIndex]['checked'],
                     'show': $scope.NodeList[nodeIndex]['show']
                  } ;
                  $.each( formVal, function( key, value ){
                     if( key == '' )
                     {
                        return true ;
                     }
                     $scope.NodeList[nodeIndex][key] = value ;
                  } ) ;
               }
               else if( type == 4 )
               {
                  //保存批量节点配置
                  var num = 0 ;
                  $.each( $scope.NodeList, function( index ){
                     if( $scope.NodeList[index]['checked'] == true && $scope.NodeList[index]['show'] != false )
                     {
                        //把配置复制出来
                        var newFormVal = $.extend( true, {}, formVal ) ;
                        //根据实际节点，转换服务名和路径
                        newFormVal['svcname'] = portEscape( newFormVal['svcname'], num ) ;
                        newFormVal['dbpath']  = dbpathEscape( formVal['dbpath'],
                                                              formVal['HostName'],
                                                              newFormVal['svcname'].length == 0 ? $scope.NodeList[index]['svcname'] : newFormVal['svcname'],
                                                              $scope.NodeList[index]['role'],
                                                              formVal['datagroupname'] ) ;
                        $scope.NodeList[index] = {
                           'HostName': $scope.NodeList[index]['HostName'],
                           'datagroupname': $scope.NodeList[index]['datagroupname'],
                           'dbpath': $scope.NodeList[index]['dbpath'],
                           'svcname': $scope.NodeList[index]['svcname'],
                           'role': $scope.NodeList[index]['role'],
                           'checked': $scope.NodeList[index]['checked'],
                           'show': $scope.NodeList[index]['show']
                        } ;
                        $.each( newFormVal, function( key, value ){
                           if( ( ( key == 'dbpath' || key == 'svcname' ) && value.length == 0 ) || key == '' )
                           {
                              return true ;
                           }
                           $scope.NodeList[index][key] = value ;
                        } ) ;
                        ++num ;
                     }
                  } ) ;
               }
            }
            else
            {
               if( !isAllClear1 )
               {
                  $scope.Components.Modal.ShowType = 1 ;
               }
               else if( !isAllClear2 )
               {
                  $scope.Components.Modal.ShowType = 2 ;
               }
               else if( !isAllClear3 )
               {
                  $scope.Components.Modal.ShowType = 3 ;
               }
            }
            return isAllClear1 && isAllClear2 && isAllClear3 ;
         }
      }

      //创建 添加节点第一步 弹窗
      $scope.CreateAddNodeModel = function( index ){
         if( $scope.GroupList[index]['nodeNum'] >=7 )
         {
            return ;
         }
         $scope.Components.Modal.icon = '' ;
         $scope.Components.Modal.title = $scope.autoLanguage( '添加节点' ) ;
         $scope.Components.Modal.isShow = true ;
         $scope.Components.Modal.form1 = {
            inputList: [
               {
                  "name": "createModel",
                  "webName": $scope.autoLanguage( '创建节点模式' ),
                  "type": "select",
                  "value": 0,
                  "valid": [
                     { 'key': $scope.autoLanguage( '默认配置' ), 'value': 0 },
                     { 'key': $scope.autoLanguage( '新建配置' ), 'value': 1 },
                     { 'key': $scope.autoLanguage( '复制节点配置' ), 'value': 2 }
                  ],
                  'onChange': function( name, key, value ){
                     if( value == 2 )
                     {
                        $scope.Components.Modal.ShowType = 2 ;
                        $scope.Components.Modal.form2['inputList'][0]['value'] = $scope.Components.Modal.form1['inputList'][0]['value'] ;
                        $scope.Components.Modal.form2['inputList'][1]['value'] = $scope.Components.Modal.form1['inputList'][1]['value'] ;
                     }
                  }
               },
               {
                  "name": "hostname",
                  "webName": $scope.autoLanguage( '添加节点的主机' ),
                  "type": "select",
                  "value": 0,
                  "valid": []
               }
            ]
         } ;
         $scope.Components.Modal.form2 = {
            inputList: [
               {
                  "name": "createModel",
                  "webName": $scope.autoLanguage( '创建节点模式' ),
                  "type": "select",
                  "value": 2,
                  "valid": [
                     { 'key': $scope.autoLanguage( '默认配置' ), 'value': 0 },
                     { 'key': $scope.autoLanguage( '新建配置' ), 'value': 1 },
                     { 'key': $scope.autoLanguage( '复制节点配置' ), 'value': 2 }
                  ],
                  'onChange': function( name, key, value ){
                     if( value != 2 )
                     {
                        $scope.Components.Modal.ShowType = 1 ;
                        $scope.Components.Modal.form1['inputList'][0]['value'] = $scope.Components.Modal.form2['inputList'][0]['value'] ;
                        $scope.Components.Modal.form1['inputList'][1]['value'] = $scope.Components.Modal.form2['inputList'][1]['value'] ;
                     }
                  }
               },
               {
                  "name": "hostname",
                  "webName": $scope.autoLanguage( '添加节点的主机' ),
                  "type": "select",
                  "value": 0,
                  "valid": []
               },
               {
                  "name": "copyNode",
                  "webName": $scope.autoLanguage( '复制的节点' ),
                  "type": "select",
                  "value": 0,
                  "valid": []
               }
            ]
         } ;
         $.each( $scope.Configure['HostInfo'], function( index2, hostInfo ){
            $scope.Components.Modal.form1['inputList'][1]['valid'].push( { 'key': hostInfo['HostName'], 'value': index2 } ) ;
            $scope.Components.Modal.form2['inputList'][1]['valid'].push( { 'key': hostInfo['HostName'], 'value': index2 } ) ;
         } ) ;
         $.each( $scope.NodeList, function( index2, nodeInfo ){
            $scope.Components.Modal.form2['inputList'][2]['valid'].push( { 'key': nodeInfo['HostName'] + ':' + nodeInfo['svcname'] + ' ' + nodeInfo['role'], 'value': index2 } ) ;
         } ) ;
         $scope.Components.Modal.ShowType = 1 ;
         $scope.Components.Modal.Context = '<div ng-show="data.ShowType == 1" form-create para="data.form1"></div><div ng-show="data.ShowType == 2" form-create para="data.form2"></div>' ;
         $scope.Components.Modal.ok = function(){
            var form ;
            if( $scope.Components.Modal.ShowType == 1 )
            {
               form = $scope.Components.Modal.form1 ;
            }
            else
            {
               form = $scope.Components.Modal.form2 ;
            }
            var isAllClear = form.check() ;
            if( isAllClear )
            {
               var formVal = form.getValue() ;
               $scope.CreateSetNodeConfModel( formVal['createModel'], index, formVal['hostname'], formVal['copyNode'], false ) ;
            }
            else
            {
               return false ;
            }
         }
      }

      //创建 删除节点 弹窗
      $scope.CreateRemoveNodeModel = function( index ){
         if( $scope.GroupList[index]['nodeNum'] == 0 )
         {
            return;
         }
         $scope.Components.Modal.icon = '' ;
         $scope.Components.Modal.title = $scope.autoLanguage( '删除节点' ) ;
         $scope.Components.Modal.isShow = true ;
         $scope.Components.Modal.form = {
            'inputList': [
               {
                  "name": "nodename",
                  "webName": $scope.autoLanguage( '节点名' ),
                  "type": "select",
                  "value": '',
                  "valid": []
               }
            ]
         } ;
         var isFirst = true ;
         $.each( $scope.NodeList, function( index2, nodeInfo ){
            if( $scope.GroupList[index]['role'] == 'data' && nodeInfo['datagroupname'] == $scope.GroupList[index]['groupName'] )
            {
               if( isFirst == true )
               {
                  $scope.Components.Modal.form['inputList'][0]['value'] = index2 ;
                  isFirst = false ;
               }
               $scope.Components.Modal.form['inputList'][0]['valid'].push( { 'key': nodeInfo['HostName'] + ':' + nodeInfo['svcname'], 'value': index2 } ) ;
            }
            if( $scope.GroupList[index]['role'] != 'data' && nodeInfo['role'] == $scope.GroupList[index]['role'] )
            {
               if( isFirst == true )
               {
                  $scope.Components.Modal.form['inputList'][0]['value'] = index2 ;
                  isFirst = false ;
               }
               $scope.Components.Modal.form['inputList'][0]['valid'].push( { 'key': nodeInfo['HostName'] + ':' + nodeInfo['svcname'], 'value': index2 } ) ;
            }
         } ) ;
         $scope.Components.Modal.Context = '<div form-create para="data.form"></div>' ;
         $scope.Components.Modal.ok = function(){
            var isAllClear = $scope.Components.Modal.form.check() ;
            if( isAllClear )
            {
               var formVal = $scope.Components.Modal.form.getValue() ;
               $scope.NodeList.splice( formVal['nodename'], 1 ) ;
               --$scope.GroupList[index]['nodeNum'] ;
               //没有节点了，禁用删除节点的按钮
               if( $scope.GroupList[index]['nodeNum'] < 7 )
               {
                  $scope.GroupList[index]['DropdownMenu'][0]['disabled'] = false ;
               }
               if( $scope.GroupList[index]['nodeNum'] == 0 )
               {
                  $scope.GroupList[index]['DropdownMenu'][1]['disabled'] = true ;
               }
            }
            return isAllClear ;
         }
      }

      //创建 修改分区组名 弹窗
      $scope.CreateRenameGroupModel = function( index ){
         $scope.Components.Modal.icon = '' ;
         $scope.Components.Modal.title = $scope.autoLanguage( '修改分区组名' ) ;
         $scope.Components.Modal.isShow = true ;
         $scope.Components.Modal.form = {
            inputList: [
               {
                  "name": "groupName",
                  "webName": $scope.autoLanguage( '新的分区组名' ),
                  "type": "string",
                  "required": true,
                  "value": $scope.GroupList[index]['groupName'],
                  "valid": {
                     "min": 1,
                     "max": 127,
                     "ban": [ ".", "$", 'SYSCatalogGroup', 'SYSCoord' ]
                  }
               }
            ]
         } ;
         $scope.Components.Modal.Context = '<div form-create para="data.form"></div>' ;
         $scope.Components.Modal.ok = function(){
            var isAllClear = $scope.Components.Modal.form.check( function( thisValue ){
               var isFind = false ;
               $.each( $scope.GroupList, function( index, groupInfo ){
                  if( groupInfo['role'] == 'data' && groupInfo['groupName'] == thisValue['groupName'] )
                  {
                     isFind = true ;
                     return false ;
                  }
               } ) ;
               if( isFind == true )
               {
                  return [ { 'name': 'groupName', 'error': $scope.autoLanguage( '分区组已经存在' ) } ] ;
               }
               return [] ;
            } ) ;
            if( isAllClear )
            {
               var formVal = $scope.Components.Modal.form.getValue() ;
               $.each( $scope.NodeList, function( index2 ){
                  if( $scope.NodeList[index2]['datagroupname'] == $scope.GroupList[index]['groupName'] )
                  {
                     $scope.NodeList[index2]['datagroupname'] = formVal['groupName'] ;
                  }
               } ) ;
               $scope.GroupList[index]['groupName'] = formVal['groupName'] ;
            }
            return isAllClear ;
         }
      }

      //创建 删除分区组 弹窗
      $scope.CreateRemoveGroupModel = function( index ){
         if( $scope.GroupList[index]['nodeNum'] > 0 )
         {
            _IndexPublic.createInfoModel( $scope, sprintf( $scope.autoLanguage( '确定要把分区组?和分区组下的?个节点都删除吗？' ), $scope.GroupList[index]['groupName'], $scope.GroupList[index]['nodeNum'] ), $scope.autoLanguage( '是的' ), function(){
               var removeNode = function(){
                  var isFind = false ;
                  $.each( $scope.NodeList, function( index2 ){
                     if( $scope.NodeList[index2]['datagroupname'] == $scope.GroupList[index]['groupName'] )
                     {
                        $scope.NodeList.splice( index2, 1 ) ;
                        isFind = true ;
                        return false ;
                     }
                  } ) ;
                  if( isFind == true )
                  {
                     removeNode() ;
                  }
               }
               removeNode() ;
               $scope.GroupList.splice( index, 1 ) ;
            } ) ;
         }
         else
         {
            $scope.GroupList.splice( index, 1 ) ;
         }
      }

      $scope.$watch( 'search_hostname.text', function(){
         filterNodeList() ;
      } ) ;
      $scope.$watch( 'search_port.text', function(){
         filterNodeList() ;
      } ) ;
      $scope.$watch( 'search_path.text', function(){
         filterNodeList() ;
      } ) ;
      $scope.$watch( 'search_role.text', function(){
         filterNodeList() ;
      } ) ;
      $scope.$watch( 'search_group.text', function(){
         filterNodeList() ;
      } ) ;

      $scope.SwitchGroup = function( index ){
         $.each( $scope.GroupList, function( index2 ){
            if( index != index2 )
            {
               $scope.GroupList[index2]['checked'] = false ;
            }
         } ) ;
         $scope.GroupList[index]['checked'] = !$scope.GroupList[index]['checked'] ;
         if( $scope.GroupList[index]['checked'] == true )
         {
            $scope.search_role.text = $scope.GroupList[index]['role'] ;
            if( $scope.GroupList[index]['role'] == 'data' )
            {
               $scope.search_group.text = $scope.GroupList[index]['groupName'] ;
            }
            else
            {
               $scope.search_group.text = '' ;
            }
         }
         else
         {
            $scope.search_role.text = '' ;
            $scope.search_group.text = '' ;
         }
         setTimeout( function(){
            $scope.NodeListOptions.onResize() ;
         } ) ;
      }

      //从节点列表中，聚合成分区组列表
      var countGroup = function( role, groupName, defaultNum ){
         var isFind = false ;
         $.each( $scope.GroupList, function( index, groupInfo ){
            if( ( role == 'data' && groupInfo['groupName'] == groupName ) || ( role != 'data' && groupInfo['role'] == role ) )
            {
               ++$scope.GroupList[index]['nodeNum'] ;
               $scope.GroupList[index]['DropdownMenu'][1]['disabled'] = false ;
               if( role != 'coord' && $scope.GroupList[index]['nodeNum'] >= 7 )
               {
                  $scope.GroupList[index]['DropdownMenu'][0]['disabled'] = true ;
               }
               isFind = true ;
               return false ;
            }
         } ) ;
         if( isFind == false )
         {
            var groupIndex = $scope.GroupList.length ;
            if( role == 'data' )
            {
               $scope.GroupList.push( { 'role': role, 'groupName': groupName, 'nodeNum': defaultNum, 'DropdownMenu': [
                  { 'html': $compile( '<div style="padding:5px 10px" ng-click="CreateAddNodeModel(' + groupIndex + ')">{{autoLanguage("添加节点")}}</div>' )( $scope ), 'disabled': false },
                  { 'html': $compile( '<div style="padding:5px 10px" ng-click="CreateRemoveNodeModel(' + groupIndex + ')">{{autoLanguage("删除节点")}}</div>' )( $scope ), 'disabled': true },
                  {},
                  { 'html': $compile( '<div style="padding:5px 10px" ng-click="CreateRenameGroupModel(' + groupIndex + ')">{{autoLanguage("修改分区组名")}}</div>' )( $scope ) },
                  { 'html': $compile( '<div style="padding:5px 10px" ng-click="CreateRemoveGroupModel(' + groupIndex + ')">{{autoLanguage("删除分区组")}}</div>' )( $scope ) }
               ] } ) ;
            }
            else
            {
               $scope.GroupList.push( { 'role': role, 'groupName': groupName, 'nodeNum': defaultNum, 'DropdownMenu': [
                  { 'html': $compile( '<div style="padding:5px 10px" ng-click="CreateAddNodeModel(' + groupIndex + ')">{{autoLanguage("添加节点")}}</div>' )( $scope ), 'disabled': false },
                  { 'html': $compile( '<div style="padding:5px 10px" ng-click="CreateRemoveNodeModel(' + groupIndex + ')">{{autoLanguage("删除节点")}}</div>' )( $scope ), 'disabled': true }
               ] } ) ;
            }
            if( role == 'data' )
            {
               $scope.search_groupList.push( { 'key': groupName, 'value': groupName } ) ;
            }
         }
      }

      //获取业务配置
      var getModuleConfig = function(){
         var data = { 'cmd': 'get business config', 'TemplateInfo': JSON.stringify( $scope.Configure ) } ;
         SdbRest.OmOperation( data, function( configure ){
            $scope.installConfig = configure[0] ;
            $scope.Template = configure[0]['Property'] ;
            $scope.NodeList = configure[0]['Config'] ;
            if( $scope.Configure['DeployMod'] == 'standalone' )
            {
               $scope.StandaloneForm1 = {
                  'keyWidth': '160px',
                  'inputList': _Deploy.ConvertTemplate( $scope.Template, 0 )
               } ;
               $scope.StandaloneForm2 = {
                  'keyWidth': '160px',
                  'inputList': _Deploy.ConvertTemplate( $scope.Template, 1 )
               } ;
               $scope.StandaloneForm3 = {
                  'keyWidth': '160px',
                  'inputList': [
                     {
                        "name": "other",
                        "webName": $scope.autoLanguage( "自定义配置" ),
                        "type": "list",
                        "valid": {
                           "min": 0
                        },
                        "child": [
                           [
                              {
                                 "name": "name",
                                 "webName": $scope.autoLanguage( "参数名" ), 
                                 "placeholder": $scope.autoLanguage( "参数名" ),
                                 "type": "string",
                                 "valid": {
                                    "min": 1
                                 },
                                 "default": "",
                                 "value": ""
                              },
                              {
                                 "name": "value",
                                 "webName": $scope.autoLanguage( "值" ), 
                                 "placeholder": $scope.autoLanguage( "值" ),
                                 "type": "string",
                                 "valid": {
                                    "min": 1
                                 },
                                 "default": "",
                                 "value": ""
                              }
                           ]
                        ]
                     }
                  ]
               } ;
               $.each( $scope.StandaloneForm1['inputList'], function( index ){
                  var name = $scope.StandaloneForm1['inputList'][index]['name'] ;
                  $scope.StandaloneForm1['inputList'][index]['value'] = $scope.NodeList[0][name] ;
               } ) ;
               $scope.StandaloneForm1['inputList'].splice( 0, 0, {
                  "name": "HostName",
                  "webName": $scope.autoLanguage( '主机名' ),
                  "type": "string",
                  "value": $scope.NodeList[0]['HostName'],
                  "disabled": true
               } ) ;
               $.each( $scope.StandaloneForm2['inputList'], function( index ){
                  var name = $scope.StandaloneForm2['inputList'][index]['name'] ;
                  $scope.StandaloneForm2['inputList'][index]['value'] = $scope.NodeList[0][name] ;
               } ) ;
            }
            $.each( $scope.NodeList, function( index, nodeInfo ){
               countGroup( nodeInfo['role'], nodeInfo['datagroupname'], 1 ) ;
            } ) ;
            $scope.NodeGridTool = sprintf( $scope.autoLanguage( '一共 ? 个节点' ), $scope.NodeList.length ) ;
            $scope.$apply() ;
            $scope.bindResize() ;
         }, function( errorInfo ){
            _IndexPublic.createRetryModel( $scope, errorInfo, function(){
               getModuleConfig() ;
               return true ;
            } ) ;
         }, function(){
            _IndexPublic.createErrorModel( $scope, $scope.autoLanguage( '网络连接错误，请尝试按F5刷新浏览器。' ) ) ;
         } ) ;
      }

      getModuleConfig() ;

      $scope.Helper = function(){
         $scope.Components.Modal.icon = '' ;
         $scope.Components.Modal.title = $scope.autoLanguage( '帮助' ) ;
         $scope.Components.Modal.isShow = true ;
         $scope.Components.Modal.Context = '\
<div>\
   ' + $scope.autoLanguage( '关于<b>[批量修改节点]</b>。您可以使用特殊规则来<b>批量修改</b>节点的<b>服务名</b>和<b>数据路径</b>：' ) + '\
   <table class="table loosen border" style="margin-top:20px;line-height:120%;">\
      <tr>\
         <td colspan="2" style="width:220px;"><b>' + $scope.autoLanguage( '服务名规则' ) + '</b></td>\
         <td>' + $scope.autoLanguage( '规则：服务名[+步进] 或 服务名[-步进]。' ) + '</td>\
      </tr>\
      <tr>\
         <td>' + $scope.autoLanguage( '规则' ) + '</td>\
         <td>' + $scope.autoLanguage( '例子' ) + '</td>\
         <td>' + $scope.autoLanguage( '描述' ) + '</td>\
      </tr>\
      <tr>\
         <td>' + $scope.autoLanguage( '普通方式' ) + '</td>\
         <td>11810</td>\
         <td>' + $scope.autoLanguage( '设置服务名为：11810，但要注意同一主机下的节点是不能有相同服务名。假设有3个节点：PcHost-1:11810，PcHost-2:11810，PcHost-3:11810' ) + '</td>\
      </tr>\
      <tr>\
         <td>' + $scope.autoLanguage( '递增方式' ) + '</td>\
         <td>11810[+10]</td>\
         <td>' + $scope.autoLanguage( '设置已选定节点的服务名从11810起始(含11810)，每一个节点递增10。假设有3个节点：PcHost-1:11810，PcHost-2:11820，PcHost-3:11830' ) + '</td>\
      </tr>\
      <tr>\
         <td>' + $scope.autoLanguage( '递减方式' ) + '</td>\
         <td>11810[-10]</td>\
         <td>' + $scope.autoLanguage( '设置已选定节点的服务名从11810起始(含11810)，每一个节点递减10。假设有3个节点：PcHost-1:11810，PcHost-2:11800，PcHost-3:11790' ) + '</td>\
      </tr>\
   </table>\
   <table class="table loosen border" style="margin-top:20px;line-height:120%;">\
      <tr>\
         <td style="width:340px;"><b>' + $scope.autoLanguage( '数据路径规则' ) + '</b></td>\
         <td>' + $scope.autoLanguage( '规则：可以在路径中任意添加这几个特殊命令，[role] -- 角色，[svcname] -- 服务名，[groupname] -- 分区组名，[hostname] --&nbsp;&nbsp;主机名。' ) + '</td>\
      </tr>\
      <tr>\
         <td>' + $scope.autoLanguage( '例子' ) + '</td>\
         <td>' + $scope.autoLanguage( '描述' ) + '</td>\
      </tr>\
      <tr>\
         <td>/opt/sequoiadb/database/[role]/[svcname]</td>\
         <td>' + $scope.autoLanguage( '假设已选定节点配置为：角色：data，服务名：11810，数据路径将会是：/opt/sequoiadb/database/data/11810，注意：协调节点和编目节点是没有分区组名的，因此当节点是协调节点或编目节点时，[groupname]是空字符。' ) + '</td>\
      </tr>\
   </table>\
</div>' ;
         $scope.Components.Modal.ok = function(){
            return true ;
         }
      }

      $scope.SelectAll = function(){
         $.each( $scope.NodeList, function( index ){
            if( $scope.NodeList[index]['show'] != false )
            {
               $scope.NodeList[index]['checked'] = true ;
            }
         } ) ;
      }

      $scope.Unselect = function(){
         $.each( $scope.NodeList, function( index ){
            if( $scope.NodeList[index]['show'] != false )
            {
               $scope.NodeList[index]['checked'] = !$scope.NodeList[index]['checked'] ;
            }
         } ) ;
      }

      $scope.GotoConf = function(){
         $location.path( '/Deploy/SDB-Conf' ) ;
      }

      var installSdb = function( installConfig ){
         var data = { 'cmd': 'add business', 'ConfigInfo': JSON.stringify( installConfig ) } ;
         SdbRest.OmOperation( data, function( taskInfo ){
            $rootScope.tempData( 'Deploy', 'ModuleTaskID', taskInfo[0]['TaskID'] ) ;
            $location.path( '/Deploy/InstallModule' ) ;
         }, function( errorInfo ){
            _IndexPublic.createRetryModel( $scope, errorInfo, function(){
               installSdb( installConfig ) ;
               return true ;
            } ) ;
         }, function(){
            _IndexPublic.createErrorModel( $scope, $scope.autoLanguage( '网络连接错误，请尝试按F5刷新浏览器。' ) ) ;
         } ) ;
      }

      $scope.GotoInstall = function(){
         var configure = {} ;
         configure['ClusterName']  = $scope.installConfig['ClusterName'] ;
         configure['BusinessType'] = $scope.installConfig['BusinessType'] ;
         configure['BusinessName'] = $scope.installConfig['BusinessName'] ;
         configure['DeployMod']    = $scope.installConfig['DeployMod'] ;
         if( $scope.Configure['DeployMod'] == 'distribution' )
         {
            configure['Config'] = $.extend( true, [], $scope.installConfig['Config'] ) ;
            $.each( configure['Config'], function( index ){
               configure['Config'][index] = deleteJson( configure['Config'][index], [ 'checked', 'show' ] ) ;
               configure['Config'][index] = convertJsonValueString( configure['Config'][index] ) ;
            } ) ;
         }
         else if( $scope.Configure['DeployMod'] == 'standalone' )
         {
            var isAllClear1 = $scope.StandaloneForm1.check( function( formVal ){
               var error = [] ;
               if( checkPort( formVal['svcname'] ) == false )
               {
                  error.push( { 'name': 'svcname', 'error': sprintf( $scope.autoLanguage( '?格式错误。' ), $scope.autoLanguage( '服务名' ) ) } ) ;
               }
               if( formVal['dbpath'].length == 0 )
               {
                  error.push( { 'name': 'dbpath', 'error': sprintf( $scope.autoLanguage( '?长度不能小于?。' ), $scope.autoLanguage( '数据路径' ), 1 ) } ) ;
               }
               return error ;
            } ) ;
            var isAllClear2 = $scope.StandaloneForm2.check() ;
            var isAllClear3 = $scope.StandaloneForm3.check( function( valueList ){
               var error = [] ;
               $.each( valueList['other'], function( index, configInfo ){
                  if( configInfo['name'].toLowerCase() == 'hostname' ||
                      configInfo['name'].toLowerCase() == 'datagroupname' ||
                      configInfo['name'].toLowerCase() == 'role' ||
                      configInfo['name'].toLowerCase() == 'checked' ||
                      configInfo['name'].toLowerCase() == 'show' )
                  {
                     error.push( { 'name': 'other', 'error': $scope.autoLanguage( '自定义配置不能设置HostName、datagroupname、role、checked、show。' ) } ) ;
                     return false ;
                  }
               } )
               return error ;
            } ) ;
            if( isAllClear1 && isAllClear2 && isAllClear3 )
            {
               var formVal1 = $scope.StandaloneForm1.getValue() ;
               var formVal2 = $scope.StandaloneForm2.getValue() ;
               var formVal3 = $scope.StandaloneForm3.getValue() ;
               var formVal = $.extend( true, formVal1, formVal2 ) ;
               $.each( formVal3['other'], function( index, configInfo ){
                  if( configInfo['name'] != '' )
                  {
                     formVal[ configInfo['name'] ] = configInfo['value'] ;
                  }
               } ) ;
               configure['Config'] = [ {} ] ;
               $.each( formVal, function( key, value ){
                  configure['Config'][0][key] = value ;
               } ) ;
               configure['Config'][0]['role'] = 'standalone' ;
               configure['Config'][0]['datagroupname'] = '' ;
               configure['Config'][0] = convertJsonValueString( configure['Config'][0] ) ;
            }
            else
            {
               return ;
            }
         }
         installSdb( configure ) ;
      }

   } ) ;
}());