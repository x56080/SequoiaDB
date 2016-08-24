
// --------------------- Index ---------------------
var _IndexPublic = {} ;

//创建社区版不支持提示的弹窗
_IndexPublic.createCommunityModel = function( $scope ){
   $scope.Components.Confirm.type = 1 ;
   $scope.Components.Confirm.noOK = false ;
   $scope.Components.Confirm.noClose = true ;
   $scope.Components.Confirm.title = $scope.autoLanguage( '提示' ) ;
   $scope.Components.Confirm.context = $scope.autoLanguage( '社区版不支持这个操作。' ) ;
   $scope.Components.Confirm.isShow = true ;
}

//创建操作不支持提示的弹窗
_IndexPublic.createNotSupportModel = function( $scope ){
   $scope.Components.Confirm.type = 1 ;
   $scope.Components.Confirm.noOK = false ;
   $scope.Components.Confirm.noClose = true ;
   $scope.Components.Confirm.title = $scope.autoLanguage( '提示' ) ;
   $scope.Components.Confirm.context = $scope.autoLanguage( '目前不支持这个操作。' ) ;
   $scope.Components.Confirm.isShow = true ;
}

//检查当前操作是不是不支持的
_IndexPublic.checkEditionAndSupport = function( $scope, moduleType, modelName ){
   var rc = true ;
   if( window.Config['Edition'] != 'Enterprise' )
   {
      _IndexPublic.createCommunityModel( $scope ) ;
      rc = false ;
   }
   else if( window.Config['Controller'][moduleType][modelName] == false )
   {
      _IndexPublic.createNotSupportModel( $scope ) ;
      rc = false ;
   }
   return rc ;
}

//创建错误弹窗
_IndexPublic.createErrorModel = function( $scope, context ){
   $scope.Components.Confirm.type = 3 ;
   $scope.Components.Confirm.noOK = true ;
   $scope.Components.Confirm.noClose = true ;
   $scope.Components.Confirm.context = context ;
   $scope.Components.Confirm.isShow = true ;
}

//关闭错误弹窗
_IndexPublic.closeErrorModel = function( $scope ){
   if( $scope.Components.Confirm.type == 3 )
   {
      $scope.Components.Confirm.isShow = false ;
   }
}

//关闭错误重试弹窗
_IndexPublic.closeRetryModel = function( $scope ){
   if( $scope.Components.Confirm.type == 1 )
   {
      $scope.Components.Confirm.isShow = false ;
   }
}

//创建错误重试弹窗
_IndexPublic.createRetryModel = function( $scope, errorInfo, okFun, title, context, okText, closeText ){
   $scope.Components.Confirm.isShow = true ;
   $scope.Components.Confirm.type = 1 ;
   $scope.Components.Confirm.title = typeof( title ) == 'string' ? title : $scope.autoLanguage( '获取数据失败' ) ;
   $scope.Components.Confirm.okText = typeof( okText ) == 'string' ? okText : $scope.autoLanguage( '重试' ) ;
   $scope.Components.Confirm.closeText = typeof( closeText ) == 'string' ? closeText : $scope.autoLanguage( '取消' ) ;
   $scope.Components.Confirm.context = typeof( context ) == 'string' ? context : sprintf( $scope.autoLanguage( '错误码: ?, ?。需要重试吗?' ), errorInfo['errno'], errorInfo['detail'].length > 0? errorInfo['detail'] : errorInfo['description'] ) ;
   $scope.Components.Confirm.ok = function(){
      if( okFun() == true )
      {
         $scope.Components.Confirm.isShow = false ;
      }
   }
}

//创建提示弹窗
_IndexPublic.createInfoModel = function( $scope, context, bt1, bt1Fun ){
   $scope.Components.Confirm.isShow = true ;
   $scope.Components.Confirm.type = 2 ;
   $scope.Components.Confirm.okText = bt1 ;
   $scope.Components.Confirm.closeText = $scope.autoLanguage( '取消' ) ;
   $scope.Components.Confirm.context = context ;
   $scope.Components.Confirm.ok = function(){
      $scope.Components.Confirm.isShow = false ;
      if( typeof( bt1Fun ) == 'function' ) bt1Fun() ;
   }
}

//语言控制器
_IndexPublic.languageCtrl = function( $scope, text ){
   var newText = text ;
   if( $scope.Language == 'en' )
   {
      function setLanguage()
      {
         if( typeof( window.SdbSacLanguage[text] ) == 'undefined' )
         {
            printfDebug( '"' + text + '" 没翻译！' ) ;
         }
         else
         {
            newText = window.SdbSacLanguage[text] ;
         }
      }
      if( typeof( window.SdbSacLanguage ) == 'undefined' )
      {
         //获取语言
         $.ajax( './app/language/English.json', { 'async': false, 'success': function( reqData ){
            window.SdbSacLanguage = JSON.parse( reqData ) ;
            setLanguage() ;
         }, 'error': function( XMLHttpRequest, textStatus, errorThrown ){
            window.SdbSacLanguage = {} ;
            _IndexPublic.createErrorModel( $scope, 'Can not find the language file, please try to refresh your browser by pressing F5.' ) ;
         } } ) ;
      }
      else
      {
         setLanguage() ;
      }
   }
   return newText ;
}

// --------------------- Index.Left ---------------------
var _IndexLeft = {} ;

//更新导航信息
_IndexLeft.updateNav = function( $scope, $rootScope, SdbRest, callBack )
{
   //获取业务实例列表
   var data = { 'cmd': 'query business', 'sort': JSON.stringify( { 'BusinessName': 1, 'ClusterName': 1 } ) } ;
   SdbRest.OmOperation( data, function( instanceList ){
      $rootScope.initNav() ;
      var navMenuLength = $scope.Left.navMenu.length ;
      $.each( instanceList, function( index, moduleInfo ){
         var thisModule = { 'name': moduleInfo['BusinessName'],
                            'type': moduleInfo['BusinessType'],
                            'mode': moduleInfo['DeployMod'],
                            'cluster': moduleInfo['ClusterName'] } ;
         if( thisModule['type'] == 'spark' )
         {
            thisModule['href'] = 'http://' + moduleInfo['BusinessInfo']['HostName'] + ':' + moduleInfo['BusinessInfo']['WebServicePort'] ;
         }
         for( var i = 0; i < navMenuLength; ++i )
         {
            if( isArray( $scope.Left.navMenu[i]['list'] ) == true )
            {
               var moduleLength = $scope.Left.navMenu[i]['list'].length ;
               for( var k = 0; k < moduleLength; ++k )
               {
                  if( $scope.Left.navMenu[i]['list'][k]['title'].toLocaleLowerCase() == moduleInfo['BusinessType'].toLocaleLowerCase() )
                  {
                     $scope.Left.navMenu[i]['list'][k]['list'].push( thisModule ) ;
                  }
               }
            }
         }
      } ) ;
      $scope.$apply() ;
      if( typeof( callBack ) == 'function' )
      {
         callBack( instanceList, $scope.Left.navMenu ) ;
      }
      setTimeout( function(){
         $rootScope.updateNav( false ) ;
      }, 5000 ) ;
   }, function( errorInfo ){
      setTimeout( function(){
         $rootScope.updateNav( false ) ;
      }, 5000 ) ;
   }, null, null, false ) ;
}

//激活导航要激活的业务的索引
_IndexLeft.getActiveIndex = function( $rootScope, SdbFunction, navMenu )
{
   var defaultIndex   = [ -1, -1, -1 ] ;
   var cursorIndex    = [  0, -1, -1 ] ;
   var cursorModule   = $rootScope.Url.Module ;
   var cursorCluster  = SdbFunction.LocalData( 'SdbClusterName' ) ;
   var cursorInstance = SdbFunction.LocalData( 'SdbModuleName' ) ;
   $.each( navMenu, function( key, val ){
      if( val['module'] == cursorModule )
      {
         cursorIndex[0] = key ;
      }
   } ) ;
   defaultIndex[0] = cursorIndex[0] ;
   if( typeof( navMenu[ cursorIndex[0] ]['list'] ) != 'undefined' )
   {
      $.each( navMenu[ cursorIndex[0] ]['list'], function( index1, moduleNav ){
         $.each( moduleNav['list'], function( index2, instanceNav ){
            if( defaultIndex[1] == -1 )
            {
               defaultIndex[1] = index1 ;
            }
            if( defaultIndex[2] == -1 )
            {
               defaultIndex[2] = index2 ;
            }
            if( instanceNav['cluster'] == cursorCluster &&
                instanceNav['name'] == cursorInstance )
            {
               cursorIndex[1] = index1 ;
               cursorIndex[2] = index2 ;
               return false ;
            }
         } ) ;
         if( cursorIndex[0] >= 0 && cursorIndex[1] >= 0 && cursorIndex[2] >= 0 )
         {
            return false
         }
      } ) ;
   }
   if( cursorIndex[0] >= 0 && cursorIndex[1] >= 0 && cursorIndex[2] >= 0 )
   {
      return cursorIndex ;
   }
   else
   {
      return defaultIndex ;
   }
}

// --------------------- Index.Bottom ---------------------
var _IndexBottom = {} ;

//获取系统时间
_IndexBottom.getSystemTime = function( $scope )
{
   var times = $.now() ;
   setInterval( function(){
      $scope.$apply( function(){
         var date = new Date( times ) ;
         var year = date.getFullYear() ;
         var hour = date.getHours() ;
         var minute = date.getMinutes() ;
         var second = date.getSeconds() ;
         $scope.Bottom.year = year ;
         $scope.Bottom.nowtime = pad( hour, 2 ) + ':' + pad( minute, 2 ) + ':' + pad( second, 2 ) ;
         times += 1000 ;
      } ) ;
   }, 1000 ) ;
}

//获取ping值
_IndexBottom.checkPing = function( $scope, SdbRest, errorNum )
{
   if( typeof( errorNum ) == 'undefined') errorNum = 0 ;
   SdbRest.getPing( function( times ){
      if( times >= 0 )
      {
         if( errorNum > 0 )
         {
            window.location.reload();
         }
         else
         {
            $scope.Bottom.sysStatus = $scope.autoLanguage( '良好' ) ;
            $scope.Bottom.statusColor = 'success' ;
            setTimeout( function(){
               _IndexBottom.checkPing( $scope, SdbRest, 0 ) ;
            }, 5000 ) ;
         }
      }
      else
      {
         ++errorNum ;
          $scope.Bottom.sysStatus = $scope.autoLanguage( '网络错误' ) ;
         $scope.Bottom.statusColor = 'error' ;
         _IndexPublic.createErrorModel( $scope, $scope.autoLanguage( '服务端连接断开，正在尝试重新连接...' ) ) ;
         setTimeout( function(){
            _IndexBottom.checkPing( $scope, SdbRest, errorNum ) ;
         }, 5000 ) ;
      }
      $scope.$apply() ;
   } ) ;
}

// --------------------- Index.Top ---------------------
var _IndexTop = {} ;

//创建修改密码弹窗
_IndexTop.createPasswdModel = function( $scope, SdbRest ){
   $scope.Components.Modal.icon = 'fa-lock' ;
   $scope.Components.Modal.title = $scope.autoLanguage( '修改密码' ) ;
   $scope.Components.Modal.isShow = true ;
   $scope.Components.Modal.form = {
      inputList: [
         {
            "name": "password",
            "webName": $scope.autoLanguage( "密码" ),
            "type": "password",
            "required": true,
            "value": "",
            "valid": {
               "min": 1,
               "max": 1024
            }
         },
         {
            "name": "newPassword",
            "webName": $scope.autoLanguage( "新密码" ),
            "type": "password",
            "required": true,
            "value": "",
            "valid": {
               "min": 1,
               "max": 1024
            }
         }
      ]
   } ;
   $scope.Components.Modal.Context = '<div form-create para="data.form"></div>' ;
   $scope.Components.Modal.ok = function(){
      var isAllClear = $scope.Components.Modal.form.check() ;
      if( isAllClear )
      {
         var value = $scope.Components.Modal.form.getValue() ;
         SdbRest.ChangePasswd( 'admin', value['password'], value['newPassword'], function( json ){
            $scope.Components.Confirm.isShow = true ;
            $scope.Components.Confirm.type = 4 ;
            $scope.Components.Confirm.closeText = $scope.autoLanguage( '取消' ) ;
            $scope.Components.Confirm.context = $scope.autoLanguage( '修改密码成功。' ) ;
            $scope.Components.Confirm.noOK = true ;
         }, function( errorInfo ){
            _IndexPublic.createRetryModel( $scope, errorInfo, function(){
               _IndexTop.createPasswdModel( $scope, SdbRest ) ;
               return true ;
            }, $scope.autoLanguage( '修改密码失败。' ) ) ;
         }, function(){
            _IndexPublic.createErrorModel( $scope, $scope.autoLanguage( '网络连接错误，请尝试按F5刷新浏览器。' ) ) ;
         }, function(){
            //关闭弹窗
            $scope.Components.Modal.isShow = false ;
         } ) ;
      }
      return isAllClear ;
   }
}

//登出
_IndexTop.logout = function( $location, SdbFunction ){
   //删除会话
   SdbFunction.LocalData( 'SdbSessionID', null ) ;
   //删除用户名
   SdbFunction.LocalData( 'SdbUser', null ) ;
   //删除选中的集群
   SdbFunction.LocalData( 'SdbClusterName', null ) ;
   //删除选中的业务
   SdbFunction.LocalData( 'SdbModuleType', null ) ;
   SdbFunction.LocalData( 'SdbModuleMode', null ) ;
   SdbFunction.LocalData( 'SdbModuleName', null ) ;
   //删除cs
   SdbFunction.LocalData( 'SdbCsName', null ) ;
   //删除cl
   SdbFunction.LocalData( 'SdbClType', null ) ;
   SdbFunction.LocalData( 'SdbClName', null ) ;
   window.location.href = '/login.html' ;
}

// --------------------- Deploy ---------------------

var _Deploy = {} ;

_Deploy.GotoStep = function( $location, action ){
   $location.path( '/Deploy/' + action ) ;
}

//生成步骤图
_Deploy.BuildSdbStep = function( $scope, $location, deployModel, action, deployModule ){
   var stepList = {
      'step': 0,
      'info': [] 
   } ;
   switch( deployModel )
   {
   case 'Host':
   {
      switch( action )
      {
      case 'ScanHost':
         stepList['step'] = 1 ;
         break ;
      case 'AddHost':
         stepList['step'] = 2 ;
         break ;
      case 'InstallHost':
         stepList['step'] = 3 ;
         break ;
      }
      stepList['info'].push( { 'text': $scope.autoLanguage( '扫描主机' ), 'click': function(){ _Deploy.GotoStep( $location, 'ScanHost' ); } } ) ;
      stepList['info'].push( { 'text': $scope.autoLanguage( '添加主机' ), 'click': function(){ _Deploy.GotoStep( $location, 'AddHost'  ); } } ) ;
      stepList['info'].push( { 'text': $scope.autoLanguage( '安装主机' ), 'click': function(){ _Deploy.GotoStep( $location, 'InstallHost'  ); } } ) ;
      break ;
   }
   case 'Module':
   {
      switch( action )
      {
      case 'SDB-Conf':
      case 'SSQL-Conf':
         stepList['step'] = 1 ;
         break ;
      case 'SDB-Mod':
      case 'SSQL-Mod':
         stepList['step'] = 2 ;
         break ;
      case 'ZKP-Mod':
         stepList['step'] = 1 ;
         break ;
      case 'InstallModule':
         if( deployModule == 'sequoiadb' || deployModule == 'sequoiasql' )
         {
            stepList['step'] = 3 ;
         }
         else if( deployModule == 'zookeeper' )
         {
            stepList['step'] = 2 ;
         }
         break ;
      }
      if( deployModule == 'sequoiadb' )
      {
         stepList['info'].push( { 'text': $scope.autoLanguage( '配置业务' ), 'click': function(){ _Deploy.GotoStep( $location, 'SDB-Conf' ); } } ) ;
         stepList['info'].push( { 'text': $scope.autoLanguage( '修改业务' ), 'click': function(){ _Deploy.GotoStep( $location, 'SDB-Mod'  ); } } ) ;
      }
      else if( deployModule == 'sequoiasql' )
      {
         stepList['info'].push( { 'text': $scope.autoLanguage( '配置业务' ), 'click': function(){ _Deploy.GotoStep( $location, 'SSQL-Conf' ); } } ) ;
         stepList['info'].push( { 'text': $scope.autoLanguage( '修改业务' ), 'click': function(){ _Deploy.GotoStep( $location, 'SSQL-Mod'  ); } } ) ;
      }
      else if( deployModule == 'zookeeper' )
      {
         stepList['info'].push( { 'text': $scope.autoLanguage( '修改业务' ), 'click': function(){ _Deploy.GotoStep( $location, 'ZKP-Mod'  ); } } ) ;
      }
      stepList['info'].push( { 'text': $scope.autoLanguage( '安装业务' ), 'click': function(){ _Deploy.GotoStep( $location, 'Install'  ); } } ) ;
      break ;
   }
   case 'Deploy':
   {
      switch( action )
      {
      case 'ScanHost':
         stepList['step'] = 1 ;
         break ;
      case 'AddHost':
         stepList['step'] = 2 ;
         break ;
      case 'InstallHost':
         stepList['step'] = 3 ;
         break ;
      case 'SDB-Conf':
      case 'SSQL-Conf':
         stepList['step'] = 4 ;
         break ;
      case 'SDB-Mod':
      case 'SSQL-Mod':
         stepList['step'] = 5 ;
         break ;
      case 'InstallModule':
         if( deployModule == 'zookeeper' )
         {
            stepList['step'] = 5 ;
         }
         else
         {
            stepList['step'] = 6 ;
         }
         break ;
      }
      stepList['info'].push( { 'text': $scope.autoLanguage( '扫描主机' ), 'click': function(){ _Deploy.GotoStep( $location, 'ScanHost' ); } } ) ;
      stepList['info'].push( { 'text': $scope.autoLanguage( '添加主机' ), 'click': function(){ _Deploy.GotoStep( $location, 'AddHost'  ); } } ) ;
      stepList['info'].push( { 'text': $scope.autoLanguage( '安装主机' ), 'click': function(){ _Deploy.GotoStep( $location, 'Install'  ); } } ) ;
      if( deployModule == 'sequoiadb' )
      {
         stepList['info'].push( { 'text': $scope.autoLanguage( '配置业务' ), 'click': function(){ _Deploy.GotoStep( $location, 'SDB-Conf' ); } } ) ;
         stepList['info'].push( { 'text': $scope.autoLanguage( '修改业务' ), 'click': function(){ _Deploy.GotoStep( $location, 'SDB-Mod'  ); } } ) ;
      }
      else if( deployModule == 'sequoiasql' )
      {
         stepList['info'].push( { 'text': $scope.autoLanguage( '配置业务' ), 'click': function(){ _Deploy.GotoStep( $location, 'SSQL-Conf' ); } } ) ;
         stepList['info'].push( { 'text': $scope.autoLanguage( '修改业务' ), 'click': function(){ _Deploy.GotoStep( $location, 'SSQL-Mod'  ); } } ) ;
      }
      else if( deployModule == 'zookeeper' )
      {
         stepList['info'].push( { 'text': $scope.autoLanguage( '修改业务' ), 'click': function(){ _Deploy.GotoStep( $location, 'ZKP-Mod'  ); } } ) ;
      }
      stepList['info'].push( { 'text': $scope.autoLanguage( '安装业务' ), 'click': function(){ _Deploy.GotoStep( $location, 'Install'  ); } } ) ;
      break ;
   }
   }
   return stepList ;
}

//参数模板转换
_Deploy.ConvertTemplate = function( templateList, level ){
   var setLevel = 0 ;
   if( typeof( level ) != 'undefined' )
   {
      setLevel = level ;
   }
   var newTemplateList = [] ;
   $.each( templateList, function( index, templateInfo ){
      if( typeof( templateInfo['Name'] ) == 'string' )
      {
         if( setLevel != templateInfo['Level'] )
         {
            return true ;
         }
         var newTemplateInfo = {
            'name':     templateInfo['Name'],
            'value':    templateInfo['Default'],
            'webName':  templateInfo['WebName'],
            'disabled': templateInfo['Edit'] == "false" ? true : false,
            'desc':     templateInfo['Desc'],
            'type':     '',
            'valid':    ''
         } ;
         if( templateInfo['Display'] == 'select box' )
         {
            newTemplateInfo['type'] = 'select' ;
            newTemplateInfo['valid'] = [] ;
            var validArr = templateInfo['Valid'].split( ',' ) ;
            $.each( validArr, function( index2 ){
               newTemplateInfo['valid'].push( { 'key': validArr[index2], 'value': validArr[index2] } ) ;
            } ) ;
         }
         else if( templateInfo['Display'] == 'edit box' )
         {
            if( templateInfo['Type'] == 'int' )
            {
               newTemplateInfo['type'] = 'int' ;
               newTemplateInfo['valid'] = {} ;
               if( templateInfo['Valid'] !== '' && templateInfo['Valid'].indexOf('-') !== -1 )
			      {
				      var splitValue = templateInfo['Valid'].split( '-' ) ;
				      var minValue = splitValue[0] ;
				      var maxValue = splitValue[1] ;
				      if( isNaN( minValue ) == false )
                  {
                     newTemplateInfo['valid']['min'] = parseInt( minValue ) ;
                  }
                  if( isNaN( maxValue ) == false )
                  {
                     newTemplateInfo['valid']['max'] = parseInt( maxValue ) ;
                  }
			      }
            }
            else if( templateInfo['Type'] === 'port' )
		      {
               newTemplateInfo['type'] = 'port' ;
               newTemplateInfo['valid'] = {} ;
               if( templateInfo['Valid'] !== '' && templateInfo['Valid'].indexOf('-') !== -1 )
			      {
				      var splitValue = templateInfo['Valid'].split( '-' ) ;
				      var minValue = splitValue[0] ;
				      var maxValue = splitValue[1] ;
				      if( isNaN( minValue ) == false )
                  {
                     newTemplateInfo['valid']['min'] = parseInt( minValue ) ;
                  }
                  if( isNaN( maxValue ) == false )
                  {
                     newTemplateInfo['valid']['max'] = parseInt( maxValue ) ;
                  }
			      }
               else
               {
                  newTemplateInfo['valid']['empty'] = true ;
               }
		      }
            else if( templateInfo['Type'] === 'string' )
		      {
               newTemplateInfo['type'] = 'string' ;
               newTemplateInfo['valid'] = {} ;
               if( templateInfo['Valid'] !== '' && templateInfo['Valid'].indexOf('-') !== -1 )
			      {
				      var splitValue = templateInfo['Valid'].split( '-' ) ;
				      var minValue = splitValue[0] ;
				      var maxValue = splitValue[1] ;
				      if( isNaN( minValue ) == false )
                  {
                     newTemplateInfo['valid']['min'] = parseInt( minValue ) ;
                  }
                  if( isNaN( maxValue ) == false )
                  {
                     newTemplateInfo['valid']['max'] = parseInt( maxValue ) ;
                  }
			      }
		      }
         }
         else if( templateInfo['Display'] == 'text box' )
         {
            newTemplateInfo['type'] = 'text' ;
            if( templateInfo['Type'] == 'path' )
            {
               newTemplateInfo['valid'] = {
                  'min': 1
               } ;
            }
         }
         if( templateInfo['Display'] == 'hidden' )
         {
            return true ;
         }
         newTemplateList.push( newTemplateInfo ) ;
      }
      else
      {
         newTemplateList.push( templateInfo ) ;
      }
   } ) ;
   return newTemplateList ;
}