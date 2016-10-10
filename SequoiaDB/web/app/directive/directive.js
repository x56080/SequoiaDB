(function(){
   var sacApp = window.SdbSacManagerModule ;

   //创建网格的指令(免html构造)
   sacApp.directive( 'createGrid', function( $filter, $compile, $window, $rootScope, SdbFunction ){
      var dire = {
         restrict: 'A',
         scope: {
            data: '=createGrid'
         },
         templateUrl: './app/template/Component/Grid.html',
         replace: false,
         controller: function( $scope, $element ){
            $scope.isFirst_firefox = 0 ;
            $scope.Setting = {
               titleWidth: [],
               bodyWidth: [],
               grid:{
                  borderBottom: '',
                  height: '0px',
                  maxHeight: 'none',
                  tdHeight: []
               }
            } ;
            $scope.Grid = { 'orderName': '0.text', 'reverse': false } ;
         },
         compile: function( element, attributes ){
            //设置列宽
            function setColumnWidth( scope, parentEle, isFirefox )
            {
               var gridEle = $( '> .Grid:first', parentEle ) ;
               var bodyEle = $( '> .GridBody:first', gridEle ) ;
               var tdBorderEle = $( '> .GridHeader:first > .GridTr:first > .GridTd:first', gridEle ) ;
               var tdBorderWidth = numberCarry( tdBorderEle.outerWidth() - tdBorderEle.width() ) ;
               var titleNum = scope.data.title.length ;
               var width = parseInt( gridEle.outerWidth() - numberCarry( gridEle.outerWidth() - gridEle.width() ) - titleNum * tdBorderWidth ) ;
               var bodyWidth = parseInt( $( bodyEle ).outerWidth() - titleNum * tdBorderWidth ) ;//width - 18 ;
               if( isFirefox == true  && scope.isFirst_firefox < 2 )
               {
                  ++scope.isFirst_firefox ;
                  bodyWidth -= 17 ;
               }
               var scrollWidth = width - bodyWidth ;
               var titleWidth = scope.data.options.grid.titleWidth ;
               var sumWidth = 0 ;
               var sumBodyWidth = 0 ;
               scope.Setting.titleWidth = [] ;
               scope.Setting.bodyWidth = [] ;
               if( typeof( titleWidth ) == 'undefined' || titleWidth == 'auto' )
               {
                  var aveWidth = parseInt( width / titleNum ) ;
                  for( var i = 0; i < titleNum; ++i )
                  {
                     //标题
                     var tmpTdWidth = aveWidth ;
                     if( i + 1 == titleNum )
                     {
                        tmpTdWidth = width - sumWidth - scrollWidth ;
                     }
                     scope.Setting.titleWidth.push( tmpTdWidth + 'px' ) ;
                     sumWidth += tmpTdWidth ;
                     //内容
                     tmpTdWidth = aveWidth ;
                     if( i + 1 == titleNum )
                     {
                        tmpTdWidth = bodyWidth - sumBodyWidth ;
                     }
                     scope.Setting.bodyWidth.push( tmpTdWidth + 'px' ) ;
                     sumBodyWidth += tmpTdWidth ;
                  }
               }
               else
               {
                  $.each( titleWidth, function( index ){
                     scope.Setting.titleWidth.push( 0 ) ;
                     scope.Setting.bodyWidth.push( 0 ) ;
                  } ) ;
                  var lastIndex = 0 ;
                  $.each( titleWidth, function( index ){
                     if( typeof( titleWidth[index] ) == 'string' )
                     {
                        //标题
                        var tmpTdWidth = parseInt( titleWidth[index] ) ;
                        var bodyTdWidth = tmpTdWidth ;
                        /*
                        if( index + 1 == titleNum )
                        {
                           tmpTdWidth += scrollWidth ;
                        }
                        */
                        scope.Setting.titleWidth[index] = ( tmpTdWidth + 'px' ) ;
                        sumWidth += tmpTdWidth ;
                        //内容
                        tmpTdWidth = bodyTdWidth ;
                        sumBodyWidth += tmpTdWidth ;
                        scope.Setting.bodyWidth[index] = ( tmpTdWidth + 'px' ) ;
                     }
                     else if( typeof( titleWidth[index] ) == 'number' )
                     {
                        lastIndex = index ;
                     }
                  } ) ;
                  var lastSumWidth = bodyWidth - sumWidth ;
                  $.each( titleWidth, function( index ){
                     if( typeof( titleWidth[index] ) == 'number' )
                     {
                        //标题
                        var tmpTdWidth = 0 ;
                        tmpTdWidth = parseInt( titleWidth[index] * lastSumWidth * 0.01 ) ;
                        var bodyTdWidth = tmpTdWidth ;
                        if( index == lastIndex )
                        {
                           tmpTdWidth = bodyWidth - sumWidth ;
                        }
                        if( tmpTdWidth < 0 ) tmpTdWidth = 0 ;
                        scope.Setting.titleWidth[index] = ( tmpTdWidth + 'px' ) ;
                        sumWidth += tmpTdWidth ;
                        //内容
                        tmpTdWidth = bodyTdWidth ;
                        if( index == lastIndex )
                        {
                           tmpTdWidth = bodyWidth - sumBodyWidth ;
                        }
                        if( tmpTdWidth < 0 ) tmpTdWidth = 0 ;
                        scope.Setting.bodyWidth[index] = ( tmpTdWidth + 'px' ) ;
                        sumBodyWidth += tmpTdWidth ;
                     }
                  } ) ;
               }
            }
            //设置行高
            function setRowHeight( scope, parentEle )
            {
               var model = scope.data.options.grid.tdModel ;
               if( typeof( model ) == 'undefined' ) model = 'auto' ;
               scope.Setting.grid.tdHeight = [] ;
               if( model == 'dynamic' )
               {
                  $.each( scope.data['body'], function( index, row ){
                     scope.Setting.grid.tdHeight.push( 'auto' ) ;
                  } ) ;
               }
               else if( model == 'fixed' && typeof( scope.data.options.grid.tdHeight ) != 'undefined' )
               {
                  if( typeof( scope.data.options.grid.tdHeight ) == 'string' )
                  {
                     $.each( scope.data['body'], function( index, row ){
                        scope.Setting.grid.tdHeight.push( scope.data.options.grid.tdHeight ) ;
                     } ) ;
                  }
                  else if( typeof( scope.data.options.grid.tdHeight ) == 'number' )
                  {
                     $.each( scope.data['body'], function( index, row ){
                        scope.Setting.grid.tdHeight.push( scope.data.options.grid.tdHeight + 'px' ) ;
                     } ) ;
                  }
               }
               else if( model == 'auto' )
               {
                  scope.$apply() ;
                  var columnNum = 0 ;
                  if( scope.data['body'].length > 0 )
                  {
                     columnNum = scope.data['body'][0].length ;
                  }
                  var tdEle = $( '> .Grid:first > .GridBody:first > .GridTr > .GridTd', parentEle ) ;
                  $.each( scope.data['body'], function( index, row ){
                     var maxHeight = 0 ;
                     $.each( row, function( index2, column ){
                        var tdHeight = $( tdEle[ index * columnNum + index2 ] ).height() ;
                        if( tdHeight > maxHeight )
                        {
                           maxHeight = tdHeight ;
                        }
                     } ) ;
                     scope.Setting.grid.tdHeight.push( maxHeight + 'px' ) ;
                  } ) ;
               }
            }
            //设置网格高度
            function setGridHeight( scope, parentEle )
            {
               scope.$apply( function(){
                  var height = $( parentEle ).outerHeight() ;
                  var gridModel = scope.data.options.grid.gridModel ;
                  if( typeof( gridModel ) == 'undefined' ) gridModel = 'auto' ;
                  if( typeof( scope.data['tool'] ) == 'object' )
                  {
                     if( typeof( scope.data.tool['position'] ) == 'string' )
                     {
                        if( scope.data.tool.position == 'top' || scope.data.tool.position == 'bottom' )
                        {
                           height -= 30 ;
                        }
                        else
                        {
                           scope.Setting.grid.borderBottom = '1px solid #E3E7E8' ;
                        }
                     }
                  }
                  else
                  {
                     scope.Setting.grid.borderBottom = '1px solid #E3E7E8' ;
                  }
                  if( gridModel == 'auto' )
                  {
                     scope.Setting.grid.height = 'auto' ;
                     scope.Setting.grid.maxHeight = height + 'px' ;
                  }
                  else if( gridModel == 'maxHeight' )
                  {
                     scope.Setting.grid.height = 'auto' ;
                     scope.Setting.grid.maxHeight = scope.data.options.grid.maxHeight + 'px' ;
                  }
                  else
                  {
                     //gridModel = fixed
                     scope.Setting.grid.height = height + 'px' ;
                     scope.Setting.grid.maxHeight = 'none' ;
                  }
               } ) ;
            }
            //网格副标题追加html代码和事件
            function setGridSubTitle( scope, parentEle )
            {
               if( typeof( scope.data['subtitle'] ) == 'object' )
               {
                  var rowNum = scope.data['subtitle'].length ;
                  var tdEle = $( '> .Grid:first > .GridHeader:first > .GridTr:last > .GridTd', parentEle ) ;
                  for( var index = 0; index < rowNum; ++index )
                  {
                     if( typeof( scope.data['subtitle'][index]['html'] ) == 'string' )
                     {
                        var newEle = $compile( scope.data['subtitle'][index]['html'] )( scope ) ;
                        $( tdEle[ index ] ).append( newEle ) ;
                     }
                     else if( typeof( scope.data['subtitle'][index]['html'] ) == 'object' )
                     {
                        $( tdEle[ index ] ).append( scope.data['subtitle'][index]['html'] ) ;
                     }
                     else
                     {
                        $( tdEle[ index ] ).text( scope.data['subtitle'][index]['text'] ) ;
                     }
                  }
               }
            }
            //网格内容追加html代码和事件
            function setGridBody( scope, parentEle )
            {
               if( typeof( scope.data['subtitle'] ) == 'object' )
               {
                  var marTop = $( '> .Grid:first > .GridHeader:first', parentEle ).height() ;
                  $( '> .Grid:first > .GridBody:first', parentEle ).css( 'marginTop', marTop ) ;
               }
               var rowNum = scope.data['body'].length ;
               var columnNum = 0 ;
               var tdEle = $( '> .Grid:first > .GridBody:first > .GridTr > .GridTd', parentEle ) ;
               for( var index = 0; index < rowNum; ++index )
               {
                  if( index == 0 ) columnNum = scope.data['body'][index].length ;
                  for( var index2 = 0; index2 < columnNum; ++index2 )
                  {
                     if( typeof( scope.data['body'][index][index2]['html'] ) == 'string' )
                     {
                        var newEle = $compile( scope.data['body'][index][index2]['html'] )( scope ) ;
                        $( tdEle[ index * columnNum + index2 ] ).append( newEle ) ;
                        scope.data['body'][index][index2]['string'] = $( newEle ).text() ;
                     }
                     else if( typeof( scope.data['body'][index][index2]['html'] ) == 'object' )
                     {
                        $( tdEle[ index * columnNum + index2 ] ).append( scope.data['body'][index][index2]['html'] ) ;
                        scope.data['body'][index][index2]['string'] = $( scope.data['body'][index][index2]['html'] ).text() ;
                     }
                     else
                     {
                        $( tdEle[ index * columnNum + index2 ] ).text( scope.data['body'][index][index2]['text'] ) ;
                        scope.data['body'][index][index2]['string'] = scope.data['body'][index][index2]['text'] ;
                     }
                  }
               }
            }
            //网格工具栏追加html代码和事件
            function setGridTool( scope, parentEle )
            {
               if( typeof( scope.data['tool'] ) != 'undefined' )
               {
                  if( typeof( scope.data['tool']['left'] ) != 'undefined' )
                  {
                     $.each( scope.data['tool']['left'], function( index, left ){
                        if( typeof( left['html'] ) == 'string' )
                        {
                           var ele = $( '> .GridTool > .ToolLeft > span:eq(' + index + ')', parentEle ) ;
                           var newEle = $compile( left['html'] )( scope ) ;
                           angular.element( ele.get(0) ).empty() ;
                           angular.element( ele.get(0) ).append( newEle ) ;
                        }
                        else if( typeof( left['html'] ) == 'object' )
                        {
                           var ele = $( '> .GridTool > .ToolLeft > span:eq(' + index + ')', parentEle ) ;
                           var newEle = left['html'] ;
                           $( ele ).empty() ;
                           $( ele ).append( newEle ) ;
                        }
                     } ) ;
                  }
                  if( typeof( scope.data['tool']['right'] ) != 'undefined' )
                  {
                     $.each( scope.data['tool']['right'], function( index, right ){
                        if( typeof( right['html'] ) == 'string' )
                        {
                           var ele = $( '> .GridTool > .ToolRight > span:eq(' + index + ')', parentEle ) ;
                           var newEle = $compile( right['html'] )( scope ) ;
                           angular.element( ele.get(0) ).empty() ;
                           angular.element( ele.get(0) ).append( newEle ) ;
                        }
                        else if( typeof( right['html'] ) == 'object' )
                        {
                           var ele = $( '> .GridTool > .ToolRight > span:eq(' + index + ')', parentEle ) ;
                           var newEle = right['html'] ;
                           $( ele ).empty() ;
                           $( ele ).append( newEle ) ;
                        }
                     } ) ;
                  }
               }
            }
            //清除网格内容
            function clearup( scope, parentEle )
            {
               $( '> .Grid:first > .GridBody:first > .GridTr > .GridTd', parentEle ).empty() ;
            }
            //执行Resize事件
            function onResize( scope, widthArr, heightArr )
            {
               if( scope.data['options']['event'] && typeof( scope.data['options']['event']['onResize'] ) == 'function' )
               {
                  var column = scope.data['title'].length ;
                  $.each( scope.data['body'], function( y, columnInfo ){
                     $.each( columnInfo, function( x ){
                        scope.data['options']['event']['onResize']( x, y, scope.Setting.bodyWidth[x], scope.Setting.grid.tdHeight[y] ) ;
                     } ) ;
                  } ) ;
               }
            }
            return {
               pre: function preLink( scope, element, attributes ){
                  $( element ).css( 'position', 'relative' ) ;
                  var gridOnResize = function () {
                     if( !scope.data )
                     {
                        return;
                     }
                     //设置列宽
                     setColumnWidth( scope, element ) ;
                     //设置行高
                     setRowHeight( scope, element ) ;
                     //设置总高度
                     setGridHeight( scope, element ) ;
                     //响应事件
                     onResize( scope ) ;
                  } ;
                  if( scope.data )
                  {
                     scope.data.onResize = gridOnResize ;
                  }
                  angular.element( $window ).bind( 'resize', gridOnResize ) ;
                  var listener1 = $rootScope.$watch( 'onResize', function(){
                     setTimeout( gridOnResize ) ;
                  } ) ;
                  var browserInfo = SdbFunction.getBrowserInfo() ;
                  var listener2 = scope.$watch( 'data', function(){
                     //清除网格内容
                     if( typeof( scope.data ) == 'object' )
                     {
                        scope.data.onResize = gridOnResize ;
                        clearup( scope, element ) ;
                        $( '> .Grid:first', element ).css( 'visibility', 'hidden' ) ;
                        $( '> .GridTool:first', element ).css( 'visibility', 'hidden' ) ;
                        setTimeout( function(){
                           //网格副标题追加html代码和事件
                           setGridSubTitle( scope, element ) ;
                           //网格内容追加html代码和事件
                           setGridBody( scope, element ) ;
                           //网格工具栏追加html代码和事件
                           setGridTool( scope, element ) ;
                           //设置列宽
                           setColumnWidth( scope, element, ( browserInfo[0] == 'firefox' ) ) ;
                           //设置行高
                           setRowHeight( scope, element ) ;
                           //设置总高度
                           setGridHeight( scope, element ) ;
                           //响应事件
                           onResize( scope ) ;
                           $( '> .Grid:first', element ).css( 'visibility', 'visible' ) ;
                           $( '> .GridTool:first', element ).css( 'visibility', 'visible' ) ;
                        } ) ;
                     }
                  } ) ;
                  var listener3 = scope.$watch( 'data.body', function(){
                     //清除网格内容
                     if( typeof( scope.data ) == 'object' )
                     {
                        scope.data.onResize = gridOnResize ;
                        clearup( scope, element ) ;
                        $( '> .Grid:first', element ).css( 'visibility', 'hidden' ) ;
                        $( '> .GridTool:first', element ).css( 'visibility', 'hidden' ) ;
                        setTimeout( function(){
                           //网格内容追加html代码和事件
                           setGridBody( scope, element ) ;
                           //网格工具栏追加html代码和事件
                           setGridTool( scope, element ) ;
                           //设置列宽
                           setColumnWidth( scope, element, ( browserInfo[0] == 'firefox' ) ) ;
                           //设置行高
                           setRowHeight( scope, element ) ;
                           //设置总高度
                           setGridHeight( scope, element ) ;
                           //响应事件
                           onResize( scope ) ;
                           $( '> .Grid:first', element ).css( 'visibility', 'visible' ) ;
                           $( '> .GridTool:first', element ).css( 'visibility', 'visible' ) ;
                        } ) ;
                     }
                  } ) ;
                  scope.$on( '$destroy', function(){
                     angular.element( $window ).unbind( 'resize', gridOnResize ) ;
                     listener1() ;
                     listener2() ;
                     listener3() ;
                  } ) ;
               },
               post: function postLink( scope, element, attributes ){
                  //添加排序事件
                  var removeWatch = scope.$watch( 'data', function(){
                     if( typeof( scope.data ) == 'object' )
                     {
                        setTimeout( function(){
                           if( typeof( scope.data['options'] ) == 'object' &&
                               typeof( scope.data['options']['order'] ) != 'undefined' &&
                               scope.data['options']['order']['active'] == true )
                           {
                              var GridHeaderTd = [] ;
                              $.each( scope.data['title'], function( index ){
                                 var tdEle = $( '> .Grid > .GridHeader > .GridTr:eq(0) > .GridTd:eq(' + index + ')', element ).css( 'cursor', 'pointer' ) ;
                                 GridHeaderTd.push( tdEle ) ;
                                 var lastColumn = -1 ;
                                 $( tdEle ).bind( 'click', function(){
                                    (function( ele, column ){
                                       if( lastColumn != column )
                                       {
                                          scope.Grid.reverse = true ;
                                       }
                                       else
                                       {
                                          scope.Grid.reverse = !scope.Grid.reverse ;
                                       }
                                       lastColumn = column ;
                                       var orderBy = $filter( 'orderObjectBy' ) ;
                                       scope.data['body'] = orderBy( scope.data['body'], column + '.string', scope.Grid.reverse ) ;
                                       var g = ele ;
                                       var caret = $( ' > .fa', g ) ;
                                       var isDown = true ;
                                       if( caret.length > 0 )
                                       {
                                          isDown = caret.hasClass( 'fa-caret-up' ) ;
                                       }
                                       $.each( GridHeaderTd, function( index, ele ){
                                          $( ' > .fa ', ele ).remove() ;
                                       } ) ;
                                       if( isDown )
                                       {
                                          $( g ).prepend( '<i class="fa fa-lg fa-caret-down"></i>' ) ;
                                       }
                                       else
                                       {
                                          $( g ).prepend( '<i class="fa fa-lg fa-caret-up"></i>' ) ;
                                       }
                                    }( this, index )) ;
                                    setGridBody( scope, element ) ;
                                 } ) ;
                              } ) ;
                           }
                        } ) ;
                     }
                  } ) ;
                  scope.$on( '$destroy', function(){
                     removeWatch() ;
                  } ) ;
               }
            } ;
         }
      } ;
      return dire ;
   });
  
   //创建网格的指令(需要html构造)
   sacApp.directive( 'ngGrid', function( $filter, $compile, $window, $rootScope, SdbFunction ){
      var browserInfo = SdbFunction.getBrowserInfo() ;
      var dire = {
         restrict: 'A',
         scope: {
            data: '=ngGrid'
         },
         replace: false,
         controller: function( $scope, $element ){
            $scope.isFirst_firefox = 0 ;
            $scope.Setting = {
               titleWidth: [],
               bodyWidth: [],
               tdHeight: []
            } ;
         },
         compile: function( element, attributes ){
            //设置表格头列宽高
            function setHeader( scope, gridEle, isFirefox )
            {
               var bodyEle    = $( '> .GridBody:first', gridEle ) ;
               var titleTdEle = $( '> .GridHeader:first > .GridTr:first > .GridTd', gridEle ) ;
               var subTitleTdEle = $( '> .GridHeader:first > .GridTr:last > .GridTd', gridEle ) ;
               var tdBorder   = numberCarry( $( titleTdEle[0] ).outerWidth() - $( titleTdEle[0] ).width() ) ;
               var titleNum   = titleTdEle.length ;
               var width      = parseInt( $( gridEle ).outerWidth() - numberCarry( $( gridEle ).outerWidth() - $( gridEle ).width() ) - titleNum * tdBorder ) ;
               var titleWidth = scope.data ? scope.data.titleWidth : undefined ;
               var bodyWidth = parseInt( $( bodyEle ).outerWidth() - titleNum * tdBorder ) ;
               if( isFirefox == true  && scope.isFirst_firefox < 2 )
               {
                  bodyWidth -= 17 ;
               }
               var scrollWidth = width - bodyWidth ;
               var sumWidth = 0 ;
               var cursorTitleWidth = [] ;
               if( typeof( titleWidth ) == 'undefined' || titleWidth == 'auto' )
               {
                  var aveWidth = parseInt( width / titleNum ) ;
                  for( var i = 0; i < titleNum; ++i )
                  {
                     //标题
                     var tmpTdWidth = aveWidth ;
                     if( i + 1 == titleNum )
                     {
                        tmpTdWidth = width - sumWidth - scrollWidth ;
                     }
                     cursorTitleWidth.push( tmpTdWidth ) ;
                     sumWidth += tmpTdWidth ;
                  }
                  for( var c = 0; c < titleNum; ++c )
                  {
                     $( titleTdEle[c] ).width( cursorTitleWidth[c] ) ;
                     if( subTitleTdEle.length == titleNum )
                     {
                        $( subTitleTdEle[c] ).width( cursorTitleWidth[c] ) ;
                     }
                  }
               }
               else
               {
                  for( var index = 0; index < titleNum; ++index )
                  {
                     cursorTitleWidth.push( 0 ) ;
                  }
                  var lastIndex = 0 ;
                  for( var index = 0; index < titleNum; ++index )
                  {
                     if( typeof( titleWidth[index] ) == 'string' )
                     {
                        //标题
                        var tmpTdWidth = parseInt( titleWidth[index] ) ;
                        var bodyTdWidth = tmpTdWidth ;
                        if( index + 1 == titleNum )
                        {
                           tmpTdWidth += scrollWidth ;
                        }
                        cursorTitleWidth[index] = ( tmpTdWidth + 'px' ) ;
                        sumWidth += tmpTdWidth ;
                     }
                     else if( typeof( titleWidth[index] ) == 'number' )
                     {
                        lastIndex = index ;
                     }
                  }
                  var lastSumWidth = width - sumWidth ;
                  for( var index = 0; index < titleNum; ++index )
                  {
                     if( typeof( titleWidth[index] ) == 'number' )
                     {
                        //标题
                        var tmpTdWidth = 0 ;
                        tmpTdWidth = parseInt( titleWidth[index] * lastSumWidth * 0.01 ) ;
                        var bodyTdWidth = tmpTdWidth ;
                        if( index == lastIndex )
                        {
                           tmpTdWidth = width - sumWidth - scrollWidth ;
                        }
                        if( tmpTdWidth < 0 ) tmpTdWidth = 0 ;
                        cursorTitleWidth[index] = ( tmpTdWidth + 'px' ) ;
                        sumWidth += tmpTdWidth ;
                     }
                  }
                  for( var c = 0; c < titleNum; ++c )
                  {
                     $( titleTdEle[c] ).width( cursorTitleWidth[c] ) ;
                     if( subTitleTdEle.length == titleNum )
                     {
                        $( subTitleTdEle[c] ).width( cursorTitleWidth[c] ) ;
                     }
                  }
               }
            }
            //设置列宽
            function setColumnWidth( scope, gridEle, isFirefox )
            {
               var bodyEle    = $( '> .GridBody:first', gridEle ) ;
               var titleTdEle = $( '> .GridHeader:first > .GridTr:first > .GridTd', gridEle ) ;
               var bodyTrEle  = $( '> .GridTr', bodyEle ) ;
               var bodyTdEle  = $( '> .GridTd', bodyTrEle ) ;
               var tdBorder   = numberCarry( $( titleTdEle[0] ).outerWidth() - $( titleTdEle[0] ).width() ) ;
               var titleNum   = titleTdEle.length ;
               var rowNum     = bodyTrEle.length ;
               var width      = parseInt( $( gridEle ).outerWidth() - numberCarry( $( gridEle ).outerWidth() - $( gridEle ).width() ) - titleNum * tdBorder ) ;
               var bodyWidth = parseInt( $( bodyEle ).outerWidth() - titleNum * tdBorder ) ;
               if( isFirefox == true  && scope.isFirst_firefox < 2 )
               {
                  ++scope.isFirst_firefox ;
                  bodyWidth -= 17 ;
               }
               var scrollWidth = width - bodyWidth ;
               var titleWidth = scope.data.titleWidth ;
               var sumWidth = 0 ;
               var sumBodyWidth = 0 ;
               var cursorBodyWidth  = [] ;
               if( typeof( titleWidth ) == 'undefined' || titleWidth == 'auto' )
               {
                  var aveWidth = parseInt( width / titleNum ) ;
                  for( var i = 0; i < titleNum; ++i )
                  {
                     //内容
                     tmpTdWidth = aveWidth ;
                     if( i + 1 == titleNum )
                     {
                        tmpTdWidth = bodyWidth - sumBodyWidth ;
                     }
                     cursorBodyWidth.push( tmpTdWidth + 'px' ) ;
                     sumBodyWidth += tmpTdWidth ;
                  }
                  for( var r = 0; r < rowNum; ++r )
                  {
                     for( var c = 0; c < titleNum; ++c )
                     {
                        $( bodyTdEle[ r * titleNum + c ] ).width( cursorBodyWidth[c] ) ;
                     }
                  }
               }
               else
               {
                  var titleWidthLen = titleWidth.length ;
                  for( var index = 0; index < titleWidthLen; ++index )
                  {
                     cursorBodyWidth.push( 0 ) ;
                  }
                  var lastIndex = 0 ;
                  for( var index = 0; index < titleWidthLen; ++index )
                  {
                     if( typeof( titleWidth[index] ) == 'string' )
                     {
                        tmpTdWidth = parseInt( titleWidth[index] ) ;
                        sumBodyWidth += tmpTdWidth ;
                        cursorBodyWidth[index] = tmpTdWidth ;
                        sumWidth += tmpTdWidth ;
                     }
                     else if( typeof( titleWidth[index] ) == 'number' )
                     {
                        lastIndex = index ;
                     }
                  }
                  var lastSumWidth = width - sumWidth ;
                  for( var index = 0; index < titleWidthLen; ++index )
                  {
                     if( typeof( titleWidth[index] ) == 'number' )
                     {
                        tmpTdWidth = parseInt( titleWidth[index] * lastSumWidth * 0.01 ) ;
                        if( index == lastIndex )
                        {
                           tmpTdWidth = bodyWidth - sumBodyWidth ;
                        }
                        if( tmpTdWidth < 0 ) tmpTdWidth = 0 ;
                        cursorBodyWidth[index] = tmpTdWidth ;
                        sumBodyWidth += tmpTdWidth ;
                     }
                  }
                  for( var r = 0; r < rowNum; ++r )
                  {
                     for( var c = 0; c < titleNum; ++c )
                     {
                        $( bodyTdEle[ r * titleNum + c ] ).width( cursorBodyWidth[c] ) ;
                     }
                  }
               }
            }
            //设置行高
            function setRowHeight( scope, gridEle )
            {
               var bodyEle = $( '> .GridBody:first', gridEle ) ;
               var model = scope.data.tdModel ;
               if( typeof( model ) == 'undefined' ) model = 'auto' ;
               scope.Setting.tdHeight = [] ;
               if( model == 'dynamic' )
               {
                  $.each( scope.data['body'], function( index, row ){
                     scope.Setting.grid.tdHeight.push( 'auto' ) ;
                  } ) ;
               }
               else if( model == 'fixed' && typeof( scope.data.tdHeight ) != 'undefined' )
               {
                  if( typeof( scope.data.options.grid.tdHeight ) == 'string' )
                  {
                     $.each( scope.data['body'], function( index, row ){
                        scope.Setting.grid.tdHeight.push( scope.data.options.grid.tdHeight ) ;
                     } ) ;
                  }
                  else if( typeof( scope.data.options.grid.tdHeight ) == 'number' )
                  {
                     $.each( scope.data['body'], function( index, row ){
                        scope.Setting.grid.tdHeight.push( scope.data.options.grid.tdHeight + 'px' ) ;
                     } ) ;
                  }
               }
               else if( model == 'auto' )
               {
                  var titleEle  = $( '> .GridHeader:first > .GridTr:first > .GridTd', gridEle ) ;
                  var bodyTrEle = $( '> .GridTr', bodyEle ) ;
                  var bodyTdEle = $( '> .GridTd', bodyTrEle ) ;
                  var rowNum    = bodyTrEle.length ;
                  var columnNum = titleEle.length ;
                  for( var index = 0; index < rowNum; ++index )
                  {
                     var maxHeight = 0 ;
                     for( var index2 = 0; index2 < columnNum; ++index2 )
                     {
                        $( bodyTdEle[ index * columnNum + index2 ] ).css( 'height', 'auto' ) ;
                        var tdHeight = $( bodyTdEle[ index * columnNum + index2 ] ).height() ;
                        if( tdHeight > maxHeight )
                        {
                           maxHeight = tdHeight ;
                        }
                     }
                     for( var index2 = 0; index2 < columnNum; ++index2 )
                     {
                        $( bodyTdEle[ index * columnNum + index2 ] ).height( maxHeight ) ;
                     }
                  }
               }
            }
            return {
               pre: function preLink( scope, element, attributes ){
                  $( element ).parent().css( 'position', 'relative' ) ;
                  var gridOnResize = function ( width, height ) {
                     if( typeof( height ) == 'number' )
                     {
                        $( element ).css( 'height', height ) ;
                     }
                     //设置表格头
                     setHeader( scope, element, ( browserInfo[0] == 'firefox' ) ) ;
                     //设置列宽
                     setColumnWidth( scope, element, ( browserInfo[0] == 'firefox' ) ) ;
                     //设置行高
                     setRowHeight( scope, element ) ;
                  } ;
                  if( scope.data )
                  {
                     scope.data.onResize = gridOnResize ;
                  }
                  setHeader( scope, element ) ;
                  angular.element( $window ).bind( 'resize', gridOnResize ) ;
                  var listener1 = $rootScope.$watch( 'onResize', function(){
                     setTimeout( gridOnResize ) ;
                  } ) ;
                  var listener2 = scope.$watch( 'data', function(){
                     //清除网格内容
                     if( typeof( scope.data ) == 'object' )
                     {
                        setTimeout( function(){
                           //设置表格头
                           setHeader( scope, element ) ;
                           //设置列宽
                           setColumnWidth( scope, element, ( browserInfo[0] == 'firefox' ) ) ;
                           //设置行高
                           setRowHeight( scope, element ) ;
                           if( scope.data.isShow )
                           {
                              $( '> .GridBody:first', element ).css( 'visibility', 'visible' ) ;
                           }
                        } ) ;
                     }
                  } ) ;
                  var listener3 = scope.$watch( 'data.titleWidth', function(){
                     //清除网格内容
                     if( typeof( scope.data ) == 'object' )
                     {
                        $( element ).css( 'visibility', 'hidden' ) ;
                        setTimeout( function(){
                           //设置表格头
                           setHeader( scope, element ) ;
                           //设置列宽
                           setColumnWidth( scope, element, ( browserInfo[0] == 'firefox' ) ) ;
                           //设置行高
                           setRowHeight( scope, element ) ;
                           if( scope.data.isShow )
                           {
                              $( '> .GridBody:first', element ).css( 'visibility', 'visible' ) ;
                           }
                           $( element ).css( 'visibility', 'visible' ) ;
                        } ) ;
                     }
                  } ) ;
                  scope.$on( '$destroy', function(){
                     angular.element( $window ).unbind( 'resize', gridOnResize ) ;
                     listener1() ;
                     listener2() ;
                     listener3() ;
                  } ) ;
               },
               post: function postLink( scope, element, attributes ){}
            } ;
         }
      } ;
      return dire ;
   });

   /*
   <div class="Grid2" ng-grid2="xxx" ng-container="{}">
      <div class="Grid2Header">
         <table>
            <colgroup>
               <col />
            </colgroup>
            <tbody>
               <tr>
                  <td>
                     <div class="Ellipsis">xxxxx</div>
                  </td>
               </tr>
            </tbody>
         </table>
      </div>
      <div class="Grid2Body">
         <table>
            <colgroup>
               <col />
            </colgroup>
            <tbody>
               <tr>
                  <td>
                     <div class="Ellipsis">
                        xxxx
                     </div>
                  </td>
               </tr>
            </tbody>
         </table>
      </div>
   </div>
   <div class="GridTool">
      <div class="ToolLeft">xxxxx</div>
   </div>
   */
   //创建网格的指令(需要html构造)
   sacApp.directive( 'ngGrid2', function( $window, $rootScope, SdbFunction ){
      var browserInfo = SdbFunction.getBrowserInfo() ;
      var dire = {
         restrict: 'A',
         scope: {
            data: '=ngGrid2'
         },
         replace: false,
         // 专用控制器
         controller: function( $scope, $element ){
         },
         // 编译
         compile: function( element, attributes ){
            //设置表格列宽
            function setColumn( scope, gridEle, isFirefox )
            {
               var bodyDiv        = $( '> .Grid2Body', gridEle ) ;
               var headerTable    = $( '> .Grid2Header > table', gridEle ).css( 'width', 'auto' ) ;
               var headerTd       = $( '> tbody:first > tr:first > td:first', headerTable ) ;
               var bodyTable      = $( '> table', bodyDiv ).hide() ;
               var headerTableCol = $( '> colgroup > col', headerTable ) ;
               var bodyTableCol   = $( '> colgroup > col', bodyTable ) ;
               var width          = bodyDiv.width() ;
               var titleWidth     = scope.data ? scope.data.titleWidth : undefined ;
               var titleNum       = headerTableCol.length ;
               var tdBorder       = numberCarry( $( headerTd ).outerWidth() - $( headerTd ).width() ) ;
               var borderWidth    = parseInt( $( bodyDiv ).outerWidth() - titleNum * tdBorder ) ;
               //设置header表格的宽度
               headerTable.width( width ) ;
               //设置body表格的宽度
               bodyTable.width( width ).show() ;
               width = borderWidth ;
               if( typeof( titleWidth ) == 'undefined' || titleWidth == 'auto' )
               {
                  var aveWidth = parseInt( width / titleNum ) ;
                  $( headerTableCol ).width( aveWidth ) ;
                  $( bodyTableCol ).width( aveWidth ) ;
               }
               else
               {
                  var sumWidth = 0 ;
                  var cursorTitleWidth = [] ;
                  for( var index = 0; index < titleNum; ++index )
                  {
                     cursorTitleWidth.push( 0 ) ;
                  }
                  var lastIndex = 0 ;
                  for( var index = 0; index < titleNum; ++index )
                  {
                     if( typeof( titleWidth[index] ) == 'string' )
                     {
                        //标题
                        var tmpTdWidth = parseInt( titleWidth[index] ) ;
                        var bodyTdWidth = tmpTdWidth ;
                        cursorTitleWidth[index] = ( tmpTdWidth + 'px' ) ;
                        sumWidth += tmpTdWidth ;
                     }
                     else if( typeof( titleWidth[index] ) == 'number' )
                     {
                        lastIndex = index ;
                     }
                  }
                  var lastSumWidth = width - sumWidth ;
                  for( var index = 0; index < titleNum; ++index )
                  {
                     if( typeof( titleWidth[index] ) == 'number' )
                     {
                        //标题
                        var tmpTdWidth = 0 ;
                        tmpTdWidth = parseInt( titleWidth[index] * lastSumWidth * 0.01 ) ;
                        var bodyTdWidth = tmpTdWidth ;
                        if( index == lastIndex )
                        {
                           tmpTdWidth = width - sumWidth ;
                        }
                        if( tmpTdWidth < 0 ) tmpTdWidth = 0 ;
                        cursorTitleWidth[index] = ( tmpTdWidth + 'px' ) ;
                        sumWidth += tmpTdWidth ;
                     }
                  }
                  for( var c = 0; c < titleNum; ++c )
                  {
                     $( headerTableCol[c] ).outerWidth( cursorTitleWidth[c] ) ;
                     $( bodyTableCol[c] ).outerWidth( cursorTitleWidth[c] ) ;
                  }
               }
            }
            return {
               pre: function preLink( scope, element, attributes ){
                  $( element ).parent().css( 'position', 'relative' ) ;
               },
               post: function postLink( scope, element, attributes ){
                  //绑定事件等
                  var gridOnResize = function ( width, height ) {
                     if( typeof( height ) == 'number' )
                     {
                        $( element ).css( 'height', height ) ;
                     }
                     setColumn( scope, element, ( browserInfo[0] == 'firefox' ) ) ;
                  } ;
                  if( scope.data )
                  {
                     scope.data.onResize = gridOnResize ;
                  }
                  setColumn( scope, element ) ;
                  angular.element( $window ).bind( 'resize', gridOnResize ) ;
                  var listener1 = $rootScope.$watch( 'onResize', function(){
                     setTimeout( gridOnResize ) ;
                  } ) ;
                  var listener2 = scope.$watch( 'data', function(){
                     //清除网格内容
                     if( typeof( scope.data ) == 'object' )
                     {
                        setTimeout( function(){
                           setColumn( scope, element ) ;
                        } ) ;
                     }
                  } ) ;
                  var listener3 = scope.$watch( 'data.titleWidth', function(){
                     //清除网格内容
                     if( typeof( scope.data ) == 'object' )
                     {
                        setTimeout( function(){
                           setColumn( scope, element ) ;
                        } ) ;
                     }
                  } ) ;
                  scope.$on( '$destroy', function(){
                     angular.element( $window ).unbind( 'resize', gridOnResize ) ;
                     listener1() ;
                     listener2() ;
                     listener3() ;
                  } ) ;
               }
            } ;
         }
      } ;
      return dire ;
   } ) ;

   //弹窗
   sacApp.directive( 'createModal', function( $compile, $window, $rootScope, Tip ){
      var dire = {
         restrict: 'A',
         scope: {
            data: '=para'
         },
         templateUrl: './app/template/Component/Modal.html',
         replace: false,
         controller: function( $scope, $element ){
            $scope.Setting = {
               //弹窗状态，1是普通，2是最大化
               Status: 1,
               //弹窗样式
               Style: {
                  width: '0px',
                  height: '0px',
                  top: '0px',
                  left: '0px'
               },
               BodyStyle: {
                  width: '0px',
                  height: '0px'
               },
               //文本
               Text: {
                  OK: $rootScope.autoLanguage( '确定' ),
                  Close: $rootScope.autoLanguage( '取消' )
               },
               //遮罩
               Mask: $( '<div></div>' ).attr( 'ng-mousedown', 'prompt()').addClass( 'mask-screen unalpha' ),
               //临时数据
               Temp: {
                  left: '0px',
                  top: '0px',
                  width: '0px',
                  height: '0px',
                  x: 0,
                  y: 0
               }
            } ;
         },
         compile: function( element, attributes ){
            function modalResize( scope )
            {
               if( typeof( scope.data.Style ) == 'function' )
               {
                  function setResize()
                  {
                     scope.Setting.Style = scope.data.Style() ;
                     scope.Setting.BodyStyle.width = ( parseInt( scope.Setting.Style.width ) - 42 ) + 'px' ;
                     scope.Setting.BodyStyle.height = ( parseInt( scope.Setting.Style.height ) - 126 ) + 'px' ;
                     if( typeof( scope.data.onResize ) == 'function' )
                     {
                        scope.data.onResize( parseInt( scope.Setting.BodyStyle.width ), parseInt( scope.Setting.BodyStyle.height ) ) ;
                     }
                  }
                  setResize() ;
                  angular.element( $window ).bind( 'resize', function () {
                     setResize() ;
                  } ) ;
               }
               else
               {
                  function autoResize()
                  {
                     var bodyWidth = $( window ).width() ;
                     var bodyHeight = $( window ).height() ;
                     var width = bodyWidth * 0.5 ;
                     var height = bodyHeight * 0.5 ;
                     if( width < 600 ) width = 600 ;
                     if( height < 450 ) height = 450 ;
                     var left = ( bodyWidth - width ) * 0.5 ;
                     var top = ( bodyHeight - height ) * 0.5 ;
                     scope.Setting.Temp.left = left + 'px' ;
                     scope.Setting.Temp.top = top + 'px' ;
                     scope.Setting.Temp.width = width + 'px' ;
                     scope.Setting.Temp.height = height + 'px' ;
                     scope.Setting.Style.width = width + 'px' ;
                     scope.Setting.Style.height = height + 'px' ;
                     scope.Setting.Style.left = left + 'px' ;
                     scope.Setting.Style.top = top + 'px' ;
                     width -= 42 ;
                     height -= 126 ;
                     scope.Setting.BodyStyle.width = width + 'px' ;
                     scope.Setting.BodyStyle.height = height + 'px' ;
                     if( typeof( scope.data.onResize ) == 'function' )
                     {
                        scope.data.onResize( parseInt( scope.Setting.BodyStyle.width ), parseInt( scope.Setting.BodyStyle.height ) ) ;
                     }
                  }
                  autoResize() ;
               }
            }
            return {
               pre: function preLink( scope, element, attributes ){
                  var onResize = function () {
                     modalResize( scope ) ;
                     if( scope.Setting.Status == 2 )
                     {
                        scope.maximumModal() ;
                     }
                     else
                     {
                        scope.recoveryModal() ;
                     }
                     setTimeout( function(){
                        if( scope.Setting.Status == 2 )
                        {
                           scope.maximumModal() ;
                        }
                        else
                        {
                           scope.recoveryModal() ;
                        }
                     } ) ;
                  }
                  //更新弹窗宽度高度坐标
                  var listener1 = scope.$watch( 'data', function(){
                     if( typeof( scope.data ) == 'object' )
                     {
                        modalResize( scope ) ;
                     }
                  } ) ;
                  angular.element( $window ).bind( 'resize', onResize ) ;
                  var listener2 = $rootScope.$on( '$locationChangeStart', function(){
                     scope.data.isShow = false ;
                  } ) ;
                  scope.$on( '$destroy', function(){
                     angular.element( $window ).unbind( 'resize', onResize ) ;
                     listener1() ;
                     listener2() ;
                  } ) ;
               },
               post: function postLink( scope, element, attributes ){
                  //编译内容
                  scope.$watch( 'data.isShow', function(){
                     if( typeof( scope.data ) == 'object' )
                     {
                        if( scope.data.isShow == true )
                        {
                           scope.data.isClose = false ;
                           scope.Setting.BodyStyle['overflow-y'] = scope.data.isScroll == false ? 'hidden' : 'auto' ;
                           var timer = null ;
                           timer = setTimeout( function(){
                              $( document.body ).append( $compile( scope.Setting.Mask )( scope ) ) ;
                              var bodyEle = $( '> .modal2 > .body', element ) ;
                              var contextType = typeof( scope.data.Context ) ;
                              if( contextType == 'string' )
                              {
                                 bodyEle.html( $compile( scope.data.Context )( scope ) ) ;
                              }
                              else if( contextType == 'object' )
                              {
                                 bodyEle.html( scope.data.Context ) ;
                              }
                              else if( contextType == 'function' )
                              {
                                 $compile( scope.data.Context( bodyEle ) )( scope ) ;
                              }
                              if( typeof( scope.data.onResize ) == 'function' )
                              {
                                 scope.data.onResize( parseInt( scope.Setting.BodyStyle.width ), parseInt( scope.Setting.BodyStyle.height ) ) ;
                              }
                              modalResize( scope ) ;
                              scope.$apply() ;
                              clearTimeout( timer ) ;
                              timer = null ;
                           } ) ;
                        }
                        else
                        {
                           scope.recoveryModal() ;
                           scope.Setting.Mask.detach() ;
                           scope.data.noOK = false ;
                           scope.data.isClose = true ;
                        }
                     }
                  } ) ;

                  scope.$watch( 'data.isRepaint', function(){
                     if( typeof( scope.data ) == 'object' )
                     {
                        if( scope.data.isShow == true && typeof( scope.data.isRepaint ) == 'number' )
                        {
                           scope.data.isRepaint = null ;
                           scope.Setting.BodyStyle['overflow-y'] = scope.data.isScroll == false ? 'hidden' : 'auto' ;
                           $( document.body ).append( $compile( scope.Setting.Mask )( scope ) ) ;
                           var bodyEle = $( '> .modal2 > .body', element ) ;
                           var contextType = typeof( scope.data.Context ) ;
                           if( contextType == 'string' )
                           {
                              bodyEle.html( $compile( scope.data.Context )( scope ) ) ;
                           }
                           else if( contextType == 'object' )
                           {
                              bodyEle.html( scope.data.Context ) ;
                           }
                           else if( contextType == 'function' )
                           {
                              $compile( scope.data.Context( bodyEle ) )( scope ) ;
                           }
                           if( typeof( scope.data.onResize ) == 'function' )
                           {
                              scope.data.onResize( parseInt( scope.Setting.BodyStyle.width ), parseInt( scope.Setting.BodyStyle.height ) ) ;
                           }
                           modalResize( scope ) ;
                        }
                     }
                  } ) ;

                  //闪烁
                  scope.prompt = function(){
                     var counter = 0 ;
                     var bodyEle = $( '> .modal2 ', element ) ;
                     var timer = setInterval( function(){
                        ++counter ;
                        if( counter == 1 )
                        {
                           $( bodyEle ).css( 'box-shadow','none' );
                        }
                        else if( counter == 2 )
                        {
                           $( bodyEle ).css( 'box-shadow','0px 2px 8px rgba(0,0,0,0.5)' );
                        }
                        if( counter == 3 )
                        {
                           $( bodyEle ).css( 'box-shadow','none' );
                        }
                        else if( counter == 4 )
                        {
                           $( bodyEle ).css( 'box-shadow','0px 2px 8px rgba(0,0,0,0.5)' );
                        }
                        if( counter == 5 )
                        {
                           $( bodyEle ).css( 'box-shadow','none' );
                        }
                        else if( counter == 6 )
                        {
                           $( bodyEle ).css( 'box-shadow','0px 2px 8px rgba(0,0,0,0.5)' );
                           counter = 0 ;
                           clearInterval( timer );
                        }
                     }, 90 ) ;
                  }

                  //移动弹窗开始
                  scope.startMove = function( event ){
                     if( scope.Setting.Status == 1 )
                     {
                        var modal = $( '> .modal2', element ) ;
                        var pageX = event['pageX'] ;
                        var pageY = event['pageY'] ;
                        scope.Setting.Temp.x = pageX ;
                        scope.Setting.Temp.y = pageY ;
                        $( document.body ).addClass( 'unselect' ) ;
                        //监听鼠标移动
                        $( document ).on( 'mousemove', function( event2 ){
                           modal.addClass( 'alpha' ) ;
                           scope.moveModal( event2 ) ;
                        } ) ;
                        //监听鼠标松开
                        $( document ).on( 'mouseup', function(){
                           scope.endMove() ;
                        } ) ;
                     }
                  }

                  //移动
                  scope.moveModal = function( event ){
                     var modal = $( '> .modal2', element ) ;
                     if( modal.hasClass( 'alpha' ) )
                     {
                        var bodyWidth = $( window ).width() ;
                        var bodyHeight = $( window ).height() ;
                        var modalWidth = modal.width() ;
                        var modalHeight = modal.height() ;
                        var x = scope.Setting.Temp.x ;
                        var y = scope.Setting.Temp.y ;
                        var pageX = event['pageX'] ;
                        var pageY = event['pageY'] ;
                        scope.Setting.Temp.x = pageX ;
                        scope.Setting.Temp.y = pageY ;
                        var left = parseInt( modal.css( 'left' ) ) ;
                        var top = parseInt( modal.css( 'top' ) ) ;
                        var offsetLeft = pageX - x ;
                        var offsetTop = pageY - y ;
                        top  += offsetTop ;
                        left += offsetLeft ;
                        if( top <= 0 ) top = 0 ;
                        if( left <= 0 ) left = 0 ;
                        if( top + modalHeight + 5 >= bodyHeight ) top = bodyHeight - modalHeight - 5 ;
                        if( left + modalWidth + 5 >= bodyWidth ) left = bodyWidth - modalWidth - 5 ;
                        scope.Setting.Temp.left = left + 'px' ;
                        scope.Setting.Temp.top = top + 'px' ;
                        modal.css( { top: top, left: left } ) ;
                     }
                  }

                  //结束移动
                  scope.endMove = function(){
                     var modal = $( '> .modal2', element ) ;
                     modal.removeClass( 'alpha' ) ;
                     $( document.body ).removeClass( 'unselect' ) ;
                     $( document ).off( 'mousemove' ) ;
                     $( document ).off( 'mouseup' ) ;
                     Tip.auto() ;
                  }

                  //开始调整窗口大小
                  scope.startSetSize = function( event ){
                     var modal = $( '> .modal2', element ) ;
                     $( document.body ).addClass( 'unselect' ) ;
                     //监听鼠标移动
                     $( document ).on( 'mousemove', function( event2 ){
                        modal.addClass( 'alpha' ) ;
                        scope.setSize( event2 ) ;
                     } ) ;
                     //监听鼠标松开
                     $( document ).on( 'mouseup', function(){
                        scope.endSetSize() ;
                     } ) ;
                  }

                  //正在调整窗口大小
                  scope.setSize = function( event ){
                     var bodyWidth = $( window ).width() ;
                     var bodyHeight = $( window ).height() ;
                     var modal = $( '> .modal2', element ) ;
                     var body = $( '> .modal2 > .body', element ) ;
                     var left = parseInt( scope.Setting.Temp.left ) ;
                     var top = parseInt( scope.Setting.Temp.top ) ;
                     var pageX = event['pageX'] + 3 ;
                     var pageY = event['pageY'] + 5 ;
                     var width = pageX - left ;
                     var height = pageY - top ;
                     if( width < 600 ) width = 600 ;
                     if( height < 450 ) height = 450 ;
                     if( top + height + 10 >= bodyHeight ) height = bodyHeight - top - 10 ;
                     if( left + width + 10 >= bodyWidth ) width = bodyWidth - left - 10 ;
                     scope.Setting.Temp.width = width + 'px' ;
                     scope.Setting.Temp.height = height + 'px' ;
                     modal.width( width ).height( height ) ;
                     width -= 42 ;
                     height -= 126 ;
                     body.width( width ).height( height ) ;
                     scope.Setting.BodyStyle.width = width + 'px' ;
                     scope.Setting.BodyStyle.height = height + 'px' ;
                  }

                  //结束调整窗口大小
                  scope.endSetSize = function( id ){
                     var modal = $( '> .modal2', element ) ;
                     modal.removeClass( 'alpha' ) ;
                     $( document.body ).removeClass( 'unselect' ) ;
                     $( document ).off( 'mousemove' ) ;
                     $( document ).off( 'mouseup' ) ;
                     if( typeof( scope.data.onResize ) == 'function' )
                     {
                        scope.data.onResize( parseInt( scope.Setting.BodyStyle.width ), parseInt( scope.Setting.BodyStyle.height ) ) ;
                     }
                     Tip.auto() ;
                  }

                  //关闭弹窗
                  scope.closeModal = function(){
                     scope.recoveryModal() ;
                     scope.data.isShow = false ;
                     //scope.Setting.Mask.detach() ;
                     scope.data.onResize = null ;
                  }

                  //最大化或恢复弹窗
                  scope.switchModalSize = function(){
                     if( scope.Setting.Status == 2 )
                     {
                        scope.recoveryModal() ;
                     }
                     else
                     {
                        scope.maximumModal() ;
                     }
                  }

                  //最大化弹窗
                  scope.maximumModal = function(){
                     scope.Setting.Status = 2 ;
                     var modal = $( '> .modal2', element ) ;
                     var bodyWidth = $( window ).width() ;
                     var bodyHeight = $( window ).height() ;
                     var width = bodyWidth - 12 ;
                     var height = bodyHeight - 16 ;
                     if( width < 600 ) width = 600 ;
                     if( height < 450 ) height = 450 ;
                     var left = 6 ;
                     var top = 6 ;
                     scope.Setting.Style.width = width + 'px' ;
                     scope.Setting.Style.height = height + 'px' ;
                     scope.Setting.Style.left = left + 'px' ;
                     scope.Setting.Style.top = top + 'px' ;
                     width -= 42 ;
                     height -= 126 ;
                     $( '> .modal2 > .body', element ).width( width ).height( height ) ;
                     if( typeof( scope.data.onResize ) == 'function' )
                     {
                        scope.data.onResize( width, height ) ;
                     }
                  }

                  //恢复弹窗
                  scope.recoveryModal = function(){
                     scope.Setting.Status = 1 ;
                     var modal = $( '> .modal2', element ) ;
                     var width = parseInt( scope.Setting.Temp.width ) ;
                     scope.Setting.Style.width = width + 'px' ;
                     scope.Setting.Style.height = scope.Setting.Temp.height  ;
                     scope.Setting.Style.left = scope.Setting.Temp.left  ;
                     scope.Setting.Style.top = scope.Setting.Temp.top  ;
                     width = parseInt( scope.Setting.Style.width ) - 42 ;
                     var height = parseInt( scope.Setting.Style.height ) - 126 ;
                     $( '> .modal2 > .body', element ).width( width ).height( height ) ;
                     if( typeof( scope.data.onResize ) == 'function' )
                     {
                        scope.data.onResize( width, height ) ;
                     }
                  }
                  //确定
                  scope.ok = function(){
                     if( typeof( scope.data.ok ) == 'function' )
                     {
                        if( scope.data.ok() )
                        {
                           scope.closeModal() ;
                        }
                     }
                     else
                     {
                        scope.closeModal() ;
                     }
                  }

                  scope.close = function(){
                     if( typeof( scope.data.close ) == 'function' )
                     {
                        if( scope.data.close() )
                        {
                           scope.closeModal() ;
                        }
                     }
                     else
                     {
                        scope.closeModal() ;
                     }
                  }
               }
            } ;
         }
      } ;
      return dire ;
   });

   //Json编辑器
   sacApp.directive( 'jsonEdit', function( $rootScope ){
      var dire = {
         restrict: 'A',
         scope: {
            data: '=para'
         },
         templateUrl: './app/template/Component/JsonEdit.html',
         replace: false,
         controller: function( $scope, $element ){
            $scope.autoLanguage = $rootScope.autoLanguage ;
            var json = {} ;
            if( $scope.data && typeof( $scope.data['Json'] ) == 'object' )
            {
               json = $scope.data['Json'] ;
            }
            $scope.Setting = {
               Height: 0,
               Type: 1,
               View: json2Array( json ),
               Json: JSON.stringify( json, null, 3 ),
               Search: ''
            } ;
            $scope.Setting.View[0]['isOpen'] = true ;
            var listener = $scope.$watch( 'data.Height', function(){
               var value = parseInt( $scope.data.Height ) ;
               if( !isNaN( value ) )
               {
                  if( value >= 39 )
                  {
                     $scope.Setting.Height = value - 39 ;
                  }
                  else
                  {
                     $scope.Setting.Height = 0 ;
                  }
               }
            } ) ;
            $scope.$on( '$destroy', function(){
               listener() ;
            } ) ;
            $scope.data['Callback'] = {} ;
            $scope.data['Callback']['getJson'] = function(){
               if( $scope.Setting.Type == 1 )
               {
                  return array2Json( $scope.Setting.View ) ;
               }
               else
               {
                  try
                  {
                     return JSON.parse( $scope.Setting.Json ) ;
                  }
                  catch(e)
                  {
                     alert( e.message ) ;
                     throw e.message ;
                  }
               }
            }
         },
         compile: function( element, attributes ){
            return {
               pre: function preLink( scope, element, attributes ){
                  //scope的一些初始化或者运算
               },
               post: function postLink( scope, element, attributes ){
                  //显示类型菜单 
                  scope.showMenu = function( event ){
                     var clickEle = $( event.target ) ;
                     var button = clickEle ;
                     if( clickEle.hasClass( 'fa' ) )
                     {
                        button = clickEle.parent() ;
                     }
                     var menu = $( '~ .treeMenu', button.parent() ) ;
                     var offsetX = button.offset().left - 5 ;
                     var offsetY = button.offset().top + button.height() ;
                     menu.css( { 'left': offsetX, 'top': offsetY } ) ;
                     menu.show( 150, function(){
                        $( document ).on( 'click', function( event2 ){
                           scope.hideMenu( menu ) ;
                        } ) ;
                     } ) ;
                  }
                  //隐藏菜单
                  scope.hideMenu = function( menu ){
                     menu.hide() ;
                     $( document ).off( 'click' ) ;
                  }
                  //显示隐藏子字段
                  scope.toggle = function( field ){
                     field.isOpen = !field.isOpen ;
                  }
                  //添加子字段
                  scope.add = function( field ){
                     if( field.type == 'Array' )
                     {
                        var key = '0' ;
                        var i = 0 ;
                        while( true )
                        {
                           var hasKey = false ;
                           $.each( field['val'], function( index, val ){
                              if( val['key'] == key )
                              {
                                 hasKey = true ;
                                 return false ;
                              }
                           } ) ;
                           if( hasKey == false )
                           {
                              field['val'].push( { key: key, val: 'value', isOpen: true, type: 'Auto', level: field.level + 1 } ) ;
                              break ;
                           }
                           ++i ;
                           key = i + '' ;
                        }
                     }
                     else
                     {
                        var key = 'field' ;
                        var i = 0 ;
                        while( true )
                        {
                           var hasKey = false ;
                           $.each( field['val'], function( index, val ){
                              if( val['key'] == key )
                              {
                                 hasKey = true ;
                                 return false ;
                              }
                           } ) ;
                           if( hasKey == false )
                           {
                              field['val'].push( { key: key, val: 'value', isOpen: true, type: 'Auto', level: field.level + 1 } ) ;
                              break ;
                           }
                           ++i ;
                           key = 'field_' + i ;
                        }
                     }
                  }
                  //删除字段
                  scope.remove = function( fields, field ){
                     var index = fields.indexOf( field ) ;
                     fields.splice( index, 1 );     
                  }
                  //复制
                  scope.copy = function( fields, field ){
                     var index = fields.indexOf( field ) + 1 ;
                     var key = 'field' ;
                     var i = 0 ;
                     while( true )
                     {
                        var hasKey = false ;
                        $.each( fields, function( index, val ){
                           if( val['key'] == key )
                           {
                              hasKey = true ;
                              return false ;
                           }
                        } ) ;
                        if( hasKey == false )
                        {
                           var newField = $.extend( true, {}, field ) ;
                           newField['key'] = key ;
                           newField['isOpen'] = true ;
                           fields.splice( index, 0, newField );
                           break ;
                        }
                        ++i ;
                        key = 'field_' + i ;
                     }
                  }
                  //切换视图
                  scope.switchView = function(){
                     if( scope.Setting.Type == 1 )
                     {
                        //视图 -> Json
                        var json = array2Json( scope.Setting.View ) ;
                        scope.Setting.Json = JSON.stringify( json, null, 3 ) ;
                        scope.Setting.Type = 2 ;
                     }
                     else 
                     {
                        //Json -> 视图
                        try
                        {
                           var json = JSON.parse( scope.Setting.Json ) ;
                           scope.Setting.View = json2Array( json ) ;
                           scope.Setting.View[0]['isOpen'] = true ;
                           scope.Setting.Type = 1 ;
                        }
                        catch(e)
                        {
                           alert( e.message ) ;
                        }
                     }
                  }
                  //展开
                  scope.expand = function(){
                     if( scope.Setting.Type == 1 )
                     {
                        //视图模式
                        function viewExpand( fields )
                        {
                           $.each( fields, function( index, field ){
                              field.isOpen = true ;
                              if( field.type == 'Object' || field.type == 'Array' )
                              {
                                 viewExpand( field.val ) ;
                              }
                           } ) ;
                        }
                        viewExpand( scope.Setting.View ) ;
                     }
                     else
                     {
                        //var json = array2Json( scope.Setting.View ) ;
                        try{
                           var json = JSON.parse( scope.Setting.Json ) ;
                           scope.Setting.Json = JSON.stringify( json, null, 3 ) ;
                        }
                        catch( e )
                        {
                           alert( e.message ) ;
                        }
                     }
                  }
                  //收起
                  scope.collapse = function(){
                     if( scope.Setting.Type == 1 )
                     {
                        //视图模式
                        function viewExpand( fields )
                        {
                           $.each( fields, function( index, field ){
                              field.isOpen = false ;
                              if( field.type == 'Object' || field.type == 'Array' )
                              {
                                 viewExpand( field.val ) ;
                              }
                           } ) ;
                        }
                        viewExpand( scope.Setting.View ) ;
                     }
                     else
                     {
                        //var json = array2Json( scope.Setting.View ) ;
                        try{
                           var json = JSON.parse( scope.Setting.Json ) ;
                           scope.Setting.Json = JSON.stringify( json ) ;
                        }
                        catch( e )
                        {
                           alert( e.message ) ;
                        }
                     }
                  }
                  //修改类型
                  scope.changeType = function( event, field, newType ){
                     var clickEle = $( event.target ) ;
                     var button = clickEle ;
                     if( clickEle.hasClass( 'fa' ) )
                     {
                        button = clickEle.parent() ;
                     }
                     var menu = $( button.parent().parent().parent() ) ;
                     scope.hideMenu( menu ) ;
                     if( newType == 'Auto' )
                     {
                        if( !( field.type == 'Bool' || field.type == 'Number' || field.type == 'Null' || field.type == 'String' ) )
                        {
                           field.val = '' ;
                        }
                     }
                     else if( newType == 'Object' )
                     {
                        if( field.type == 'Object' )
                        {
                           return ;
                        }
                        else if( field.type == 'Array' )
                        {
                           $.each( field.val, function( index, child ){
                              child.key = index + '' ;
                           } ) ;
                           field.isOpen = true ;
                        }
                        else
                        {
                           field.val = [] ;
                           field.isOpen = true ;
                        }
                     }
                     else if( newType == 'Array' )
                     {
                        if( field.type == 'Array' )
                        {
                           return ;
                        }
                        else if( field.type == 'Object' )
                        {
                           $.each( field.val, function( index, child ){
                              child.key = index + '' ;
                           } ) ;
                           field.isOpen = true ;
                        }
                        else
                        {
                           field.val = [] ;
                           field.isOpen = true ;
                        }
                     }
                     else if( newType == 'String' )
                     {
                        if( field.type == 'Object' || field.type == 'Array' )
                        {
                           field.val = '' ;
                        }
                     }
                     else if( newType == 'Timestamp' )
                     {
                        field.val = timeFormat( new Date(), 'yyyy-MM-dd-hh.mm.ss.000000' ) ;
                     }
                     else if( newType == 'Date' )
                     {
                        field.val = timeFormat( new Date(), 'yyyy-MM-dd' ) ;
                     }
                     field.type = newType ;
                  }
               }
            } ;
         }
      } ;
      return dire ;
   });

   //表单
   sacApp.directive( 'formCreate', function( $rootScope, SdbFunction ){
      var dire = {
         restrict: 'A',
         scope: {
            data: '=para'
         },
         templateUrl: './app/template/Component/Form.html',
         replace: false,
         controller: function( $scope, $element ){
            var init = function(){
               if( typeof( $scope.data ) == 'object' )
               {
                  $.each( $scope.data.inputList, function( index ){
                     $scope.data.inputList[index]['isClick'] = false ;
                     if( $scope.data.inputList[index]['type'] == 'list' )
                     {
                        $.each( $scope.data.inputList[index]['child'], function( index2 ){
                           $.each( $scope.data.inputList[index]['child'][index2], function( index3 ){
                              if( typeof( $scope.data.inputList[index]['child'][index2][index3]['default'] ) == 'undefined' )
                              {
                                 $scope.data.inputList[index]['child'][index2][index3]['default'] = $scope.data.inputList[index]['child'][index2][index3]['value'] ;
                              }
                           } ) ;
                        } ) ;
                     }
                  } ) ;
                  $scope.browserInfo = SdbFunction.getBrowserInfo() ;
                  $scope.Setting = {
                     Type: $scope.data.type,
                     GridTitle: $scope.data.gridTitle,
                     Grid: $scope.data.grid,
                     KeyWidth: $scope.data.keyWidth ? $scope.data.keyWidth : '130px',
                     Text: {
                        'string': {
                           min: $rootScope.autoLanguage( '?长度不能小于?。' ),
                           max: $rootScope.autoLanguage( '?长度不能大于?。' ),
                           regex: $rootScope.autoLanguage( '?格式错误。' ),
                           ban: $rootScope.autoLanguage( '?不能有?字符。' )
                        },
                        'int': {
                           min: $rootScope.autoLanguage( '?的值不能小于?。' ),
                           max: $rootScope.autoLanguage( '?的值不能大于?。' ),
                           ban: $rootScope.autoLanguage( '?的值不能取?。' ),
                           step: $rootScope.autoLanguage( '?的值必须是?的倍数。' ),
                           format: $rootScope.autoLanguage( '?的值必须是整数。' )
                        },
                        'double': {
                           min: $rootScope.autoLanguage( '?的值不能小于?。' ),
                           max: $rootScope.autoLanguage( '?的值不能大于?。' ),
                           ban: $rootScope.autoLanguage( '?的值不能取?。' ),
                           step: $rootScope.autoLanguage( '?的值必须是?的倍数。' ),
                           format: $rootScope.autoLanguage( '?的值必须是数字。' )
                        },
                        'port':{
                           min: $rootScope.autoLanguage( '?不能小于?。' ),
                           max: $rootScope.autoLanguage( '?不能大于?。' ),
                           empty: $rootScope.autoLanguage( '?不能为空。' ),
                           format: $rootScope.autoLanguage( '?格式错误。' )
                        },
                        'multiple': {
                           min: $rootScope.autoLanguage( '?至少选择?个。' ),
                           max: $rootScope.autoLanguage( '?不能多于?个。' ),
                           empty: $rootScope.autoLanguage( '?不能为空。' ),
                           candidate: $rootScope.autoLanguage( '待选列表' ),
                           select: $rootScope.autoLanguage( '选中列表' )
                        },
                        'list': $rootScope.autoLanguage( '?参数错误。' )
                     },
                     inputList: $scope.data.inputList,
                     checkString: function( name, value, valid ){
                        var rc = true ;
                        var error = '' ;
                        if( typeof( valid ) == 'object' )
                        {
                           var min = valid.min ;
                           var max = valid.max ;
                           var reg = valid.regex ;
                           var rer = valid.regexError ;
                           var ban = valid.ban ;
                           var len = typeof( value ) == 'string' ? value.length : 0 ;
                           if( typeof( min ) == 'number' && len < min )
                           {
                              error = sprintf( $scope.Setting.Text.string.min, name, min ) ;
                              rc = false ;
                           }
                           else if( typeof( max ) == 'number' && len > max )
                           {
                              error = sprintf( $scope.Setting.Text.string.max, name, max ) ;
                              rc = false ;
                           }
                           else if( typeof( ban ) == 'string' && value.indexOf( ban ) >= 0 )
                           {
                              error = sprintf( $scope.Setting.Text.string.ban, name, ban ) ;
                              rc = false ;
                           }
                           else if( isArray( ban ) )
                           {
                              $.each( ban, function( index, banChar ){
                                 if( value.indexOf( banChar ) >= 0 )
                                 {
                                    error = sprintf( $scope.Setting.Text.string.ban, name, banChar ) ;
                                    rc = false ;
                                    return false ;
                                 }
                              } ) ;
                           }
                           else if( typeof( reg ) == 'string' )
                           {
                              var patt = new RegExp( reg, 'g' ) ;
                              if( patt.test( value ) == false )
                              {
                                 if( typeof( rer ) == 'string' )
                                 {
                                    error = rer ;
                                 }
                                 else
                                 {
                                    error = sprintf( $scope.Setting.Text.string.regex, name ) ;
                                 }
                                 rc = false ;
                              }
                           }
                        }
                        return { rc: rc, error: error } ;
                     },
                     checkInt: function ( name, value, valid ){
                        var rc = true ;
                        var error = '' ;
                        if( value.length == 0 && typeof( valid ) == 'object' && valid.empty == true )
                        {
                           return { rc: rc, error: error } ;
                        }
                        if( isNaN( value ) || parseInt( value ) != value )
                        {
                           error = sprintf( $scope.Setting['Text']['int']['format'], name ) ;
                           rc = false ;
                        }
                        else if( typeof( valid ) == 'object' )
                        {
                           var min = valid.min ;
                           var max = valid.max ;
                           var ban = valid.ban ;
                           var step = valid.step ;
                           if( typeof( min ) == 'number' && value < min )
                           {
                              error = sprintf( $scope.Setting['Text']['int']['min'], name, min ) ;
                              rc = false ;
                           }
                           else if( typeof( max ) == 'number' && value > max )
                           {
                              error = sprintf( $scope.Setting['Text']['int']['max'], name, max ) ;
                              rc = false ;
                           }
                           else if( typeof( ban ) == 'number' && value == ban )
                           {
                              error = sprintf( $scope.Setting['Text']['int']['ban'], name, ban ) ;
                              rc = false ;
                           }
                           else if( isArray( ban ) )
                           {
                              $.each( ban, function( index, banInt ){
                                 if( value == banInt )
                                 {
                                    error = sprintf( $scope.Setting['Text']['int']['ban'], name, banInt ) ;
                                    rc = false ;
                                    return false ;
                                 }
                              } ) ;
                           }
                           else if( typeof( step ) == 'number' && value % step != 0 )
                           {
                              error = sprintf( $scope.Setting['Text']['int']['step'], name, step ) ;
                              rc = false ;
                           }
                        }
                        return { rc: rc, error: error } ;
                     },
                     checkDouble: function ( name, value, valid ){
                        var rc = true ;
                        var error = '' ;
                        if( isNaN( value ) )
                        {
                           error = sprintf( $scope.Setting['Text']['double']['format'], name ) ;
                           rc = false ;
                        }
                        else if( typeof( valid ) == 'object' )
                        {
                           var min = valid.min ;
                           var max = valid.max ;
                           var ban = valid.ban ;
                           var step = valid.step ;
                           if( typeof( min ) == 'number' && value < min )
                           {
                              error = sprintf( $scope.Setting['Text']['double']['min'], name, min ) ;
                              rc = false ;
                           }
                           else if( typeof( max ) == 'number' && value > max )
                           {
                              error = sprintf( $scope.Setting['Text']['double']['max'], name, max ) ;
                              rc = false ;
                           }
                           else if( typeof( ban ) == 'number' && value == ban )
                           {
                              error = sprintf( $scope.Setting['Text']['double']['ban'], name, ban ) ;
                              rc = false ;
                           }
                           else if( isArray( ban ) )
                           {
                              $.each( ban, function( index, banInt ){
                                 if( value == banInt )
                                 {
                                    error = sprintf( $scope.Setting['Text']['double']['ban'], name, banInt ) ;
                                    rc = false ;
                                    return false ;
                                 }
                              } ) ;
                           }
                           else if( typeof( step ) == 'number' && value % step != 0 )
                           {
                              error = sprintf( $scope.Setting['Text']['int']['step'], name, step ) ;
                              rc = false ;
                           }
                        }
                        return { rc: rc, error: error } ;
                     },
                     checkPort: function ( name, value, valid ){
                        var rc = true ;
                        var error = '' ;
                        if( value.length == 0 && typeof( valid ) == 'object' && valid.empty == true )
                        {
                           return { rc: rc, error: error } ;
                        }
                        if( value.length == 0 )
                        {
                           rc = false ;
                           error = sprintf( $scope.Setting.Text.port.empty, name ) ;
                        }
                        else if( checkPort( value ) == false )
                        {
                           rc = false ;
                           error = sprintf( $scope.Setting.Text.port.format, name ) ;
                        }
                        else if( typeof( valid ) == 'object' )
                        {
                           var min = valid.min ;
                           var max = valid.max ;
                           var ban = valid.ban ;
                           var step = valid.step ;
                           if( typeof( min ) == 'number' && value < min )
                           {
                              error = sprintf( $scope.Setting['Text']['port']['min'], name, min ) ;
                              rc = false ;
                           }
                           else if( typeof( max ) == 'number' && value > max )
                           {
                              error = sprintf( $scope.Setting['Text']['port']['max'], name, max ) ;
                              rc = false ;
                           }
                        }
                        return { rc: rc, error: error } ;
                     },
                     checkMultiple: function ( name, value, valid ){
                        var rc = true ;
                        var error = '' ;
                        if( typeof( valid ) == 'object' )
                        {
                           var min = valid.min ;
                           var max = valid.max ;
                           if( value.length == 0 && valid.empty == false )
                           {
                              rc = false ;
                              error = sprintf( $scope.Setting.Text.multiple.empty, name ) ;
                           }
                           else if( typeof( min ) == 'number' && value.length < min )
                           {
                              rc = false ;
                              error = sprintf( $scope.Setting.Text.multiple.min, name, min ) ;
                           }
                           else if( typeof( max ) == 'number' && value.length > max )
                           {
                              rc = false ;
                              error = sprintf( $scope.Setting.Text.multiple.max, name, max ) ;
                           }
                        }
                        return { rc: rc, error: error } ;
                     },
                     checkInput: function( inputList, customCheckFun ){
                        var isAllClear = true ;
                        $.each( inputList, function( index, inputInfo ){
                           inputInfo.error = '' ;
                           var rv = { rc: true, error: '' } ;
                           switch( inputInfo.type )
                           {
                           case 'string':
                              rv = $scope.Setting.checkString( inputInfo.webName, trim( inputInfo.value ), inputInfo.valid ) ;
                              break ;
                           case 'password':
                              rv = $scope.Setting.checkString( inputInfo.webName, inputInfo.value, inputInfo.valid ) ;
                              break ;
                           case 'text':
                              rv = $scope.Setting.checkString( inputInfo.webName, trim( inputInfo.value ), inputInfo.valid ) ;
                              break ;
                           case 'int':
                              rv = $scope.Setting.checkInt( inputInfo.webName, trim( inputInfo.value ), inputInfo.valid ) ;
                              break ;
                           case 'double':
                              rv = $scope.Setting.checkDouble( inputInfo.webName, trim( inputInfo.value ), inputInfo.valid ) ;
                              break ;
                           case 'port':
                              rv = $scope.Setting.checkPort( inputInfo.webName, trim( inputInfo.value ), inputInfo.valid ) ;
                              break ;
                           case 'multiple':
                              rv = $scope.Setting.checkMultiple( inputInfo.webName, trim( inputInfo.value ), inputInfo.valid ) ;
                              break ;
                           case 'group':
                              isAllClear = $scope.Setting.checkInput( inputInfo.child ) ;
                              break ;
                           case 'list':
                              if( inputInfo.valid && inputInfo.valid.min == 0 && inputInfo.child.length == 1 )
                              {
                              }
                              else
                              {
                                 var hasError = false ;
                                 $.each( inputInfo.child, function( index2 ){
                                    var rc = $scope.Setting.checkInput( inputInfo.child[index2] ) ;
                                    if( rc == false )
                                    {
                                       hasError = true ;
                                    }
                                 } ) ;
                                 if( hasError == true )
                                 {
                                    isAllClear = false ;
                                    inputInfo.error = sprintf( $scope.Setting.Text.list, inputInfo.webName ) ;
                                 }
                              }
                              break ;
                           }
                           if( rv.rc == false )
                           {
                              isAllClear = false ;
                              inputInfo.error = rv.error ;
                           }
                        } ) ;
                        if( typeof( customCheckFun ) == 'function' )
                        {
                           var rvs = customCheckFun( $scope.Setting.getValue( $scope.Setting.inputList ) ) ;
                           if( rvs.length > 0 )
                           {
                              $.each( rvs, function( index2, errInfo ){
                                 $.each( inputList, function( index3, inputInfo ){
                                    if( inputInfo.name == errInfo.name )
                                    {
                                       inputInfo.error = errInfo.error ;
                                       return false ;
                                    }
                                 } ) ;
                              } ) ;
                              isAllClear = false ;
                           }
                        }
                        return isAllClear ;
                     },
                     getValue: function( inputList ){
                        var returnValue = {} ;
                        $.each( inputList, function( index, inputInfo ){
                           switch( inputInfo.type )
                           {
                           case 'string':
                              returnValue[ inputInfo.name ] = trim( inputInfo.value ) ;
                              break ;
                           case 'password':
                              returnValue[ inputInfo.name ] = inputInfo.value ;
                              break ;
                           case 'text':
                              returnValue[ inputInfo.name ] = trim( inputInfo.value ) ;
                              break ;
                           case 'int':
                              returnValue[ inputInfo.name ] = parseInt( trim( inputInfo.value ) ) ;
                              break ;
                           case 'double':
                              returnValue[ inputInfo.name ] = parseFloat( trim( inputInfo.value ) ) ;
                              break ;
                           case 'port':
                              returnValue[ inputInfo.name ] = trim( inputInfo.value ) ;
                              break ;
                           case 'multiple':
                              returnValue[ inputInfo.name ] = inputInfo.value ;
                              break ;
                           case 'select':
                              returnValue[ inputInfo.name ] = inputInfo.value ;
                              break ;
                           case 'checkbox':
                              returnValue[ inputInfo.name ] = inputInfo.value ;
                              break ;
                           case 'group':
                              returnValue[ inputInfo.name ] = $scope.Setting.getValue( inputInfo.child ) ;
                              break ;
                           case 'list':
                              var listValue = [] ;
                              $.each( inputInfo.child, function( index2, items ){
                                 listValue.push( $scope.Setting.getValue( items ) ) ;
                              } ) ;
                              returnValue[ inputInfo.name ] = listValue ;
                              break ;
                           }
                        } ) ;
                        return returnValue ;
                     },
                     getValue2: function( inputList ){
                        var returnValue = [] ;
                        $.each( inputList, function( index, inputLine ){
                           var returnLine = [] ;
                           $.each( inputLine, function( index2, inputInfo ){
                              switch( inputInfo.type )
                              {
                              case 'textual':
                              case 'string':
                              case 'select':
                              case 'checkbox':
                                 returnLine.push( inputInfo.value ) ;
                                 break ;
                              }
                           } ) ;
                           returnValue.push( returnLine ) ;
                        } ) ;
                        return returnValue ;
                     }
                  } ;
                  $scope.data.check = function( customCheckFun ){
                     return $scope.Setting.checkInput( $scope.Setting.inputList, customCheckFun ) ;
                  }
                  $scope.data.getValue = function(){
                  if( $scope.Setting.Type == 'grid' )
                  {
                     return $scope.Setting.getValue2( $scope.Setting.inputList ) ;
                  }
                  else
                  {
                     return $scope.Setting.getValue( $scope.Setting.inputList ) ;
                  }
               }
               }
            }
            var listener = $scope.$watch( 'data', function(){
               //清除网格内容
               if( typeof( $scope.data ) == 'object' )
               {
                  init() ;
               }
            } ) ;
            $scope.$on( '$destroy', function(){
               listener() ;
            } ) ;
            init() ;
         },
         compile: function( element, attributes ){
            return {
               pre: function preLink( scope, element, attributes ){
               },
               post: function postLink( scope, element, attributes ){
                  scope.multipleCheck = function( item, list ){
                     list.checked = !list.checked ;
                     if( list.checked )
                     {
                        item.value.push( list.value ) ;
                     }
                     else
                     {
                        var index = item.value.indexOf( list.value ) ;
                        if( index >= 0 )
                        {
                           item.value.splice( index, 1 ) ;
                        }
                     }
                     if( typeof( item.onChange ) == 'function' )
                     {
                        item.onChange( item.name, item.value ) ;
                     }
                  }
                  scope.multipleCheckAll = function( item, status ){
                     if( status )
                     {
                        item.value = [] ;
                        $.each( item.valid.list, function( index ){
                           item.valid.list[index]['checked'] = true ;
                           item.value.push( item.valid.list[index]['value'] ) ;
                        } ) ;
                     }
                     else
                     {
                        $.each( item.valid.list, function( index ){
                           item.valid.list[index]['checked'] = false ;
                        } ) ;
                        item.value = [] ;
                     }
                     if( typeof( item.onChange ) == 'function' )
                     {
                        item.onChange( item.name, item.value ) ;
                     }
                  }
                  scope.listAppend = function( items, item ){
                     var index = items.indexOf( item ) ;
                     var newItem = [] ;
                     $.each( item, function( index2, inputInfo ){
                        var newInputInfo = $.extend( true, {}, inputInfo ) ;
                        newInputInfo['value'] = newInputInfo['default'] ;
                        newItem.push( newInputInfo ) ;
                     } ) ;
                     items.splice( index + 1, 0, newItem ) ;
                  }
                  scope.listRemove = function( items, item ){
                     if( items.length > 1 )
                     {
                        var index = items.indexOf( item ) ;
                        items.splice( index, 1 ) ;
                     }
                  }
                  scope.onChange = function( inputInfo ){
                     if( typeof( inputInfo.onChange ) == 'function' )
                     {
                        if( inputInfo['type'] == 'select' )
                        {
                           var inputKey = '' ;
                           $.each( inputInfo.valid, function( index, item ){
                              if( inputInfo.value == item.value )
                              {
                                 inputKey = item.key ;
                                 return false ;
                              }
                           } ) ;
                           inputInfo.onChange( inputInfo.name, inputKey, inputInfo.value ) ;
                        }
                        else
                        {
                           inputInfo.onChange( inputInfo.name, inputInfo.value ) ;
                        }
                     }
                  }
                  scope.showMenu = function( inputInfo ){
                     if( inputInfo.showMenu != true )
                     {
                        var browser = SdbFunction.getBrowserInfo() ;
                        if( browser[0] == 'ie' && browser[1] == '7' )
                        {
                           return ;
                        }
                        setTimeout( function(){
                           $( document ).on( 'click', function(){
                              inputInfo.showMenu = false ;
                              $( document ).off( 'click' ) ;
                              scope.$digest();
                           } ) ;
                        } ) ;
                        inputInfo.showMenu = true ;
                     }
                  }
                  scope.placeholderClick = function( inputInfo ){
                     inputInfo.isClick = true ;
                  }
               }
            } ;
         }
      } ;
      return dire ;
   });

   //json树 key部分
   sacApp.directive( 'treeKey', function( $window ){
      var img = $( '<img>' ).attr( 'src', './images/tree/object.png' ) ;
      var dire = {
         restrict: 'A',
         scope: {
            data: '=para'
         },
         templateUrl: './app/template/Component/TreeKey.html',
         replace: false,
         controller: function( $scope, $element ){
            $scope.Setting = {
               items: $scope.data.Json,
               index: $scope.data.index,
               width: $scope.data.width
            }
         },
         compile: function( element, attributes ){
            return {
               pre: function preLink( scope, element, attributes ){
                  $( element ).addClass( 'jsonTree' ) ;
                  var listener = scope.$watch( 'data.width', function(){
                     if( scope.data && typeof( scope.data.width ) == 'number' )
                     {
                        scope.Setting.width = scope.data.width ;
                     }
                  } ) ;
                  scope.$on( '$destroy', function(){
                     listener() ;
                  } ) ;
               },
               post: function postLink( scope, element, attributes ){
                  scope.toggle = function( field ){
                     if( field.type == 'Object' || field.type == 'Array' )
                     {
                        field.isOpen = !field.isOpen ;
                     }
                  }
               }
            } ;
         }
      } ;
      return dire ;
   });

   //json树 value部分
   sacApp.directive( 'treeValue', function( $window ){
      var dire = {
         restrict: 'A',
         scope: {
            data: '=para'
         },
         templateUrl: './app/template/Component/TreeValue.html',
         replace: false,
         controller: function( $scope, $element ){
            $scope.Setting = {
               items: $scope.data.Json,
               index: $scope.data.index,
               width: $scope.data.width
            }
         },
         compile: function( element, attributes ){
            return {
               pre: function preLink( scope, element, attributes ){
                  $( element ).addClass( 'jsonTreeValue' ) ;
                  var listener = scope.$watch( 'data.width', function(){
                     if( scope.data && typeof( scope.data.width ) == 'number' )
                     {
                        scope.Setting.width = scope.data.width ;
                     }
                  } ) ;
                  scope.$on( '$destroy', function(){
                     listener() ;
                  } ) ;
               },
               post: function postLink( scope, element, attributes ){
               }
            } ;
         }
      } ;
      return dire ;
   });

   //json树 type部分
   sacApp.directive( 'treeType', function(){
      var dire = {
         restrict: 'A',
         scope: {
            data: '=para'
         },
         templateUrl: './app/template/Component/TreeType.html',
         replace: false,
         controller: function( $scope, $element ){
            $scope.Setting = {
               items: $scope.data.Json,
               index: $scope.data.index
            }
         },
         compile: function( element, attributes ){
            return {
               pre: function preLink( scope, element, attributes ){
                  $( element ).addClass( 'jsonTreeType' ) ;
               },
               post: function postLink( scope, element, attributes ){
               }
            } ;
         }
      } ;
      return dire ;
   });

   //让ng-model支持开启contenteditable的div
   sacApp.directive( 'contenteditable', function(){
      var dire = {
         restrict: 'A',
         require: '?ngModel',
         link: function( scope, element, attr, ngModel ){
            var read ;
            if( !ngModel )
            {
               return ;
            }
            ngModel.$render = function(){
               return element.text( ngModel.$viewValue ) ;
            } ;
            var keyupEvent = function(){
               if( ngModel.$viewValue !== element.text() )
               {
                  return scope.$apply( read ) ;
               }
            }
            element.bind( 'keyup', keyupEvent ) ;
            scope.$on( '$destroy', function(){
               $( element ).unbind( 'keyup', keyupEvent ) ;
            } ) ;
            return read = function() {
               return ngModel.$setViewValue( element.text() ) ;
            } ;
         }
      } ;
      return dire ;
   });

   //执行命令，类似ng-init，区别是ng-init在ng-repeat中只执行一次，但ng-eval执行多次
   sacApp.directive( 'ngEval', function(){
      var dire = {
         restrict: 'A',
         priority: 440,
         compile: function(){
            return {
               pre: function( scope, element, attrs ){
                  scope.$target = element ;
                  var lintener = scope.$watch( attrs.ngEval, function(){
                     scope.$eval( attrs.ngEval ) ;
                  } ) ;
                  scope.$on( '$destroy', function(){
                     lintener() ;
                  } ) ;
               }
            } ;
         }
      } ;
      return dire ;
   });

   //添加placeholder
   sacApp.directive( 'ngPlaceholder', function(){
      var dire = {
         restrict: 'A',
         priority: 1,
         scope: false,
         replace: false,
         controller: function( $scope, $element ){
         },
         compile: function( element, attributes ){
            return {
               pre: function preLink( scope, element, attributes ){
                  var lintener = scope.$watch( attributes.ngPlaceholder, function ngPlaceholderAction( placeholder ){
                     if( typeof( placeholder ) == 'string' )
                     {
                        $( element ).attr( 'placeholder', placeholder ) ;
                     }
                  } ) ;
                  scope.$on( '$destroy', function(){
                     lintener() ;
                  } ) ;
               },
               post: function postLink( scope, element, attributes ){
               }
            } ;
         }
      } ;
      return dire ;
   });

   //创建确认提示框
   sacApp.directive( 'createConfirm', function( $compile, $window, $rootScope ){
      var dire = {
         restrict: 'A',
         scope: {
            data: '=para'
         },
         templateUrl: './app/template/Component/Confirm.html',
         replace: false,
         // 专用控制器
      controller: function( $scope, $element ){
         $scope.Setting = {
            //弹窗样式
            Style: {
               top:'0px' ,
               left:'0px'
            },
            Mask: $( '<div></div>' ).attr( 'ng-mousedown', 'prompt()').addClass( 'mask-screen2 unalpha' ).css( 'opacity', '0.1' )
         } ;
      },

      // 编译
      compile: function( element, attributes ){
         function confirmResize( scope )
            {
               function autoResize()
               {
                  var bodyWidth = $( window ).width() ;
                  var bodyHeight = $( window ).height() ;
                  var left = ( bodyWidth - 450 ) * 0.5 ;
                  var top = ( bodyHeight - 160 ) * 0.5 ;
                  scope.Setting.Style.left = left + 'px' ;
                  scope.Setting.Style.top = top + 'px' ;
               }
               autoResize() ;
            }
         return {
            pre: function preLink( scope, element, attributes ){
               scope.data.okText = $rootScope.autoLanguage( '确定' ) ;
               scope.data.closeText = $rootScope.autoLanguage( '取消' ) ;
               //更新弹窗宽度高度坐标
               var listener1 = scope.$watch( 'data', function(){
                  if( typeof( scope.data ) == 'object' )
                  {
                     confirmResize( scope ) ;
                  }
               } ) ;
               var onResize = function () {
                  confirmResize( scope ) ;
               }
               angular.element( $window ).bind( 'resize', onResize ) ;
               var listener2 = $rootScope.$on( '$locationChangeStart', function(){
                  scope.data.isShow = false ;
               } ) ;
               scope.$on( '$destroy', function(){
                  angular.element( $window ).bind( 'resize', onResize ) ;
                  listener1() ;
                  listener2() ;
               } ) ;
            },
            post: function postLink( scope, element, attributes ){
               //编译内容
               var listener = scope.$watch( 'data.isShow', function(){
                  if( typeof( scope.data ) == 'object' )
                  {
                     if( scope.data.isShow == true )
                     {
                        setTimeout( function(){
                           $( document.body ).append( $compile( scope.Setting.Mask )( scope ) ) ;
                           confirmResize( scope ) ;
                        } ) ;
                     }
                     else
                     {
                        scope.Setting.Mask.detach() ;
                        scope.data.okText = $rootScope.autoLanguage( '确定' ) ;
                        scope.data.closeText = $rootScope.autoLanguage( '取消' ) ;
                        scope.data.ok = null ;
                        scope.data.context = '' ;
                     }
                  }
               } ) ;
               scope.$on( '$destroy', function(){
                  listener() ;
               } ) ;
               //提示框阴影效果
               scope.prompt = function(){
                  var counter = 0 ;
                  var bodyEle = $( '> .confirm ', element ) ;
                  var timer = setInterval( function(){
                     ++counter ;
                     if( counter == 1 )
                     {
                        $( bodyEle ).css( 'box-shadow','none' );
                     }
                     else if( counter == 2 )
                     {
                        $( bodyEle ).css( 'box-shadow',' 0px 2px 8px rgba(0,0,0,0.5)' );
                     }
                     if( counter == 3 )
                     {
                        $( bodyEle ).css( 'box-shadow','none' );
                     }
                     else if( counter == 4 )
                     {
                        $( bodyEle ).css( 'box-shadow',' 0px 2px 8px rgba(0,0,0,0.5)' );
                     }
                     if( counter == 5 )
                     {
                        $( bodyEle ).css( 'box-shadow','none' );
                     }
                     else if( counter == 6 )
                     {
                        $( bodyEle ).css( 'box-shadow',' 0px 2px 8px rgba(0,0,0,0.5)' );
                        counter = 0 ;
                        clearInterval( timer );
                     }
                  }, 90 ) ;
               }

               //确定
               scope.ok = function(){
                  if( typeof( scope.data.ok ) == 'function' )
                  {  
                     if( scope.data.ok() )
                     {
                        scope.close() ;
                     }
                  }
                  else
                  {
                     scope.close() ;
                  }
               } ;
               //关闭
               scope.close = function(){
                  scope.data.isShow = false ;
               } ;
            }
         } ;
      }
   } ;
      return dire ;
   } ) ;

   //取得焦点
   sacApp.directive( 'getFocus', function(){
      var dire = {
         restrict: 'A',
         scope: {
            data: '=getFocus'
         },
         replace: false,
         priority: 3,
         // 专用控制器
         controller: function( $scope, $element ){},
         // 编译
         compile: function( element, attributes ){
            return {
               pre: function preLink( scope, element, attributes ){},
               post: function postLink( scope, element, attributes ){
                  var lintener = scope.$watch( 'data', function(){
                     if( scope.data == true )
                     {
                        $( element ).get(0).focus() ;
                     }
                  } ) ;
                  scope.$on( '$destroy', function(){
                     lintener() ;
                  } ) ;
               }
            } ;
         }
      } ;
      return dire ;
   });

   //容器(自动设置宽高)
   sacApp.directive( 'ngContainer', function( $window, $rootScope ){
      var list = [] ;
      var listSize = 0 ;
      var maxLevel = 0 ;
      function _arratInsert( eleInfo )
      {
         var level = eleInfo[0] ;
         if( level >= maxLevel )
         {
            list.push( eleInfo ) ;
            ++listSize ;
            maxLevel = level ;
         }
         else
         {
            for( var i = 0; i < listSize; ++i )
            {
               if( list[i][0] > level )
               {
                  list.splice( i, 0, eleInfo ) ;
                  ++listSize ;
                  break ;
               }
            }
         }
      }
      function _renderWidth( scope, ele, width )
      {
         var marginLeft  = scope.data.marginLeft ;
         var marginRight = scope.data.marginRight ;
         var offsetX     = scope.data.offsetX ;
         marginLeft  = ( typeof( marginLeft ) != 'number' ? 0 : marginLeft ) ;
         marginRight  = ( typeof( marginRight ) != 'number' ? 0 : marginRight ) ;
         offsetX  = ( typeof( offsetX ) != 'number' ? 0 : offsetX ) ;
         width += offsetX ;
         ele.outerWidth( width ).css( { marginLeft: marginLeft, marginRight: marginRight } ) ;
      }
      function _renderHeight( scope, ele, height )
      {
         var marginTop    = scope.data.marginTop ;
         var marginBottom = scope.data.marginBottom ;
         var offsetY      = scope.data.offsetY ;
         marginTop  = ( typeof( marginTop ) != 'number' ? 0 : marginTop ) ;
         marginBottom  = ( typeof( marginBottom ) != 'number' ? 0 : marginBottom ) ;
         offsetY  = ( typeof( offsetY ) != 'number' ? 0 : offsetY ) ;
         if( height === 'auto' )
         {
            ele.css( { marginTop: marginTop, marginBottom: marginBottom, height: 'auto' } ) ;
         }
         else
         {
            height += offsetY ;
            ele.outerHeight( height ).css( { marginTop: marginTop, marginBottom: marginBottom } ) ;
         }
      }
      function _renderMaxHeight( scope, ele, maxheight )
      {
         var marginTop    = scope.data.marginTop ;
         var marginBottom = scope.data.marginBottom ;
         var offsetY      = scope.data.offsetY ;
         marginTop  = ( typeof( marginTop ) != 'number' ? 0 : marginTop ) ;
         marginBottom  = ( typeof( marginBottom ) != 'number' ? 0 : marginBottom ) ;
         offsetY  = ( typeof( offsetY ) != 'number' ? 0 : offsetY ) ;
         maxheight += offsetY ;
         ele.css( { 'marginTop': marginTop, 'marginBottom': marginBottom, 'maxHeight': maxheight } ) ;
      }
      function _render( scope, ele, parent )
      {
         $( document.body ).css( 'overflow', 'hidden' ) ;
         var width  = scope.data.width ;
         var height = scope.data.height ;
         var maxHeight = scope.data.maxHeight ;
         var widthType = typeof( width ) ;
         var heightType = typeof( height ) ;
         var maxHeightType = typeof( maxHeight ) ;
         if( widthType == 'string' )
         {
            var length = width.length ;
            if( width.charAt( length - 1 ) == '%' )
            {
               width = parseInt( parent.width() * parseInt( width ) * 0.01 ) ;
            }
            else
            {
               width  = parent.width() ;
            }
         }
         else if( widthType != 'number' )
         {
            width  = parent.width() ;
         }
         _renderWidth( scope, ele, width ) ;
         if( maxHeightType == 'string' )
         {
            var length = maxHeight.length ;
            if( maxHeight.charAt( length - 1 ) == '%' )
            {  
               maxHeight = parseInt( parent.height() * parseInt( maxHeight ) * 0.01 ) ;
            }
            _renderMaxHeight( scope, ele, maxHeight ) ;
         }
         else
         {
            if( heightType == 'string' )
            {
               var length = height.length ;
               if( height.charAt( length - 1 ) == '%' )
               {  
                  height = parseInt( parent.height() * parseInt( height ) * 0.01 ) ;
               }
               else if( height.charAt( length - 1 ) == 'w' )
               {
                  height = parseInt( width * parseInt( height ) * 0.01 ) ;
               }
               else if( height === 'auto' )
               {
               }
               else
               {
                  height  = parent.height() ;
               }
            }
            else if( heightType != 'number' )
            {
               height  = parent.height() ;
            }
            _renderHeight( scope, ele, height ) ;
         }
         $( document.body ).css( 'overflow', 'auto' ) ;
      }
      function _renderAll( rootLevel )
      {
         for( var index = ( rootLevel == -1 ? 0 : rootLevel ); index < listSize; ++index ){
            _render( list[index][1], list[index][2], list[index][3] ) ;
         }
      }
      $rootScope.$watch( 'onResize', function(){
         setTimeout( function(){
            _renderAll( -1 ) ;
         } ) ;
      } ) ;
      angular.element( $window ).bind( 'resize', function () {
         _renderAll( -1 ) ;
      } ) ;
      $rootScope.$on( '$locationChangeStart', function( event, newUrl, oldUrl ){
         var newList = [] ;
         var length = listSize ;
         listSize = 0 ;
         for(var i = 0; i < length; ++i )
         {
            var element = list[i][2] ;
            if( $( element ).is( ':hidden' ) == false )
            {
               newList.push( list[i] ) ;
               ++listSize ;
               maxLevel = list[i][0] ;
            }
         }
         list = newList ;
      } ) ;
      function _append( scope, element, parent )
      {
         var level = 0 ;
         var root = element ;
         for( ; ; ++level )
         {
            if( root.get(0) == document.body || root.get(0) == document )
            {
               break ;
            }
            root = root.parent() ;
         }
         scope.level = level ;
         _arratInsert( [ level, scope, element, parent ] ) ;
      }
      var dire = {
         restrict: 'A',
         scope: true,
         replace: false,
         priority: 1,
         // 专用控制器
         controller: function( $scope, $element ){
            $scope.level = -1 ;
         },
         //编译
         compile: function( element, attributes ){
            return {
               pre: function preLink( scope, element, attributes ){},
               post: function postLink( scope, element, attributes ){
                  scope.data = scope.$eval( attributes.ngContainer ) ;
                  var current = $( element ) ;
                  var parent = $( element ).parent() ;
                  _render( scope, current, parent ) ;
                  _append( scope, current, parent ) ;
                  scope.$watch( attributes.ngContainer, function ngContainerAction( data ){
                     scope.data = data ;
                     _renderAll( scope.level ) ;
                  } ) ;
               }
            } ;
         }
      } ;
      return dire ;
   });

   //添加自定义属性
   sacApp.directive( 'ngAttr', function(){
      var dire = {
         restrict: 'A',
         priority: 2,
         scope: false,
         replace: false,
         // 专用控制器
         controller: function( $scope, $element ){},
         // 编译
         compile: function( element, attributes ){
            return {
               pre: function preLink( scope, element, attributes ){
                  var lintener = scope.$watch( attributes.ngAttr, function ngAttrAction( attr ){
                     if( typeof( attr ) != 'undefined' )
                     {
                        $( element ).attr( attr ) ;
                     }
                  }, true ) ;
                  scope.$on( '$destroy', function(){
                     lintener() ;
                  } ) ;
               },
               post: function postLink( scope, element, attributes ){}
            } ;
         }
      } ;
      return dire ;
   });

   //创建圆柱体
   sacApp.directive( 'createCylinder', function(){
      function getAllCoord( $scope )
      {
         function getCoord( $scope, index, height, offsetY ){
            var round   = $scope.Setting.round ;
            var width   = $scope.Setting.width - 100 ;
            var height  = height ;
            var percent = $scope.Setting.data[index]['percent'] ;
            //圆柱主干
            $scope.Setting.data[index]['zg'] = {} ;
            $scope.Setting.data[index]['zg']['x'] = 0 ;
            $scope.Setting.data[index]['zg']['y'] = round + offsetY ;
            $scope.Setting.data[index]['zg']['width'] = width - 1 ;
            $scope.Setting.data[index]['zg']['height'] = height - round * 2 - offsetY ;
            if( $scope.Setting.data[index]['zg']['height'] < 0 ) $scope.Setting.data[index]['zg']['height'] = 0 ;

            //圆柱底部
            $scope.Setting.data[index]['db'] = {} ;
            $scope.Setting.data[index]['db']['cx'] = width / 2 - 1 ;
            $scope.Setting.data[index]['db']['cy'] = height - round - 1 ;
            $scope.Setting.data[index]['db']['rx'] = width / 2 - 1 ;
            $scope.Setting.data[index]['db']['ry'] = round ;

            //圆柱主干2
            $scope.Setting.data[index]['zg2'] = {} ;
            $scope.Setting.data[index]['zg2']['x'] = 1 ;
            $scope.Setting.data[index]['zg2']['y'] = round + offsetY ;
            $scope.Setting.data[index]['zg2']['width'] = width - 3 ;
            $scope.Setting.data[index]['zg2']['height'] = height - round * 2 - offsetY - 2 ;
            if( $scope.Setting.data[index]['zg2']['height'] < 0 ) $scope.Setting.data[index]['zg2']['height'] = 0 ;

            //圆柱头部
            $scope.Setting.data[index]['tb'] = {} ;
            $scope.Setting.data[index]['tb']['cx'] = width / 2 - 1 ;
            $scope.Setting.data[index]['tb']['cy'] = round + offsetY ;
            $scope.Setting.data[index]['tb']['rx'] = width / 2 - 1 ;
            $scope.Setting.data[index]['tb']['ry'] = round ;

            //箭头
            var pointX = width - 2 ;
            var pointY = height / 2 + offsetY / 2 + 5 ;
            $scope.Setting.data[index]['jt'] = {} ;
            $scope.Setting.data[index]['jt']['d'] = 'M' + pointX + ' ' + pointY + 'L' + ( pointX + 25 ) + ' ' + ( pointY - 6 ) ;

            //字体
            $scope.Setting.data[index]['zt'] = {} ;
            $scope.Setting.data[index]['zt']['x'] = width + 29 ;
            $scope.Setting.data[index]['zt']['y'] = height / 2  + offsetY / 2 - 2 ;
         }
         $scope.Setting.data = $scope.data ;
         //圆柱真正的高度
         var height2 = $scope.Setting.height - ( $scope.Setting.round * 2 ) ;
         var height3 = 0 ;
         var len = $scope.Setting.data.length ;
         $.each( $scope.Setting.data, function( index ){
            var tmpHeight = 0 ;
            var thisHeight = height2 * ( 1 - $scope.Setting.data[index]['percent'] ) - height3 ;
            var cyOffsetY = ( index + 1 < len ) ? thisHeight : 0 ;
            if( index > 0 ) tmpHeight = 1 ;
            getCoord( $scope, index, ( $scope.Setting.height - height3 + tmpHeight ), cyOffsetY ) ;
            height3 += height2 * $scope.Setting.data[index]['percent'] ;
            $scope.Setting.data[index]['percentStr'] = parseInt( $scope.Setting.data[index]['percent'] * 100 ) + '%' ;
         } ) ;
      }
      var dire = {
         restrict: 'A',
         scope: {
            data: '=para'
         },
         replace: false,
         templateUrl: './app/template/Component/Cylinder.html',
         controller: function( $scope, $element ){
            $scope.Setting = {
               width: 200,
               height: 200,
               round: 20,
               borderColor: '#F0F0F0',
               data: []
            } ;
            var lintener = $scope.$watch( 'data', function(){
               if( isArray( $scope.data ) && $scope.data.length > 0 )
               {
                  getAllCoord( $scope ) ;
               }
            } ) ;
            scope.$on( '$destroy', function(){
               lintener() ;
            } ) ;
         },
         compile: function( element, attributes ){
            return {
               pre: function preLink( scope, element, attributes ){
                  //scope的一些初始化或者运算
               },
               post: function postLink( scope, element, attributes ){
                  setTimeout( function(){
                     scope.Setting.width = $( element ).width()  - 5 ;
                     scope.Setting.height = $( element ).height() - 5 ;
                     $( '> svg', element ).attr( { width: scope.Setting.width, height: scope.Setting.height } ) ;
                     getAllCoord( scope ) ;
                  } ) ;
               }
            } ;
         }
      } ;
      return dire ;
   } );

   //创建echart图
   sacApp.directive( 'createChart', function( $rootScope, $window ){
      var dire = {
         restrict: 'A',
         scope: {
            data: '=createChart'
         },
         replace: false,
         //专用控制器
         controller: function( $scope, $element ){
            $scope.Setting = {
               'element': null
            } ;
            var lintener1 = $scope.$watch( 'data', function(){
               if( typeof( $scope.data ) == 'object' && typeof( $scope.data.options ) == 'object' )
               {
                  if( $scope.Setting.element == null )
                  {
                     $scope.Setting.element = echarts.init( $( $element ).get(0) ).setOption( $scope.data.options ) ;
                  }
                  else
                  {
                     $scope.Setting.element.setOption( $scope.data.options ) ;
                  }
               }
            } ) ;
            var lintener2 = $scope.$watch( 'data.value', function(){
               if( typeof( $scope.data ) == 'object' && typeof( $scope.data.options ) == 'object' )
               {
                  if( $scope.Setting.element != null && isArray( $scope.data.value ) )
                  {
                     $scope.Setting.element.addData( $scope.data.value ) ;
                  }
               }
            } ) ;
            $scope.$on( '$destroy', function(){
               lintener1() ;
               lintener2() ;
            } ) ;
         },
         //编译
         compile: function( element, attributes ){
            return {
               pre: function preLink( scope, element, attributes ){
                  function resize()
                  {
                     if( scope.Setting.element != null )
                     {
                        scope.Setting.element.resize() ;
                     }
                  }
                  angular.element( $window ).bind( 'resize', resize ) ;
                  var lintener = $rootScope.$watch( 'onResize', function(){
                     setTimeout( function(){
                        resize() ;
                     } ) ;
                  } ) ;
                  scope.$on( '$destroy', function(){
                     angular.element( $window ).unbind( 'resize', resize ) ;
                     lintener() ;
                  } ) ;
               },
               post: function postLink( scope, element, attributes ){}
            } ;
         }
      } ;
      return dire ;
   });

   //创建动态框架
   sacApp.directive( 'createResponse', function( $window, $rootScope ){
      //计算一个的宽度
      function getLineWidth( parentWidth, len, column, min, max )
      {
         //步进
         var step = 0 ;
         //希望得到的列宽
         var dWidth = parseInt( parentWidth / column ) ;
         if( dWidth < min )
         {
            step = -1 ;
         }
         else if( dWidth > max )
         {
            step = 1 ;
         }
         else
         {
            return dWidth ;
         }
         while( true )
         {
            column += step ;
            dWidth = parseInt( parentWidth / column ) ;
            if( step == -1 && dWidth > min )
            {
               break ;
            }
            else if( step == 1 && dWidth < max )
            {
               break ;
            }
            else if( column == 0 )
            {
               dWidth = parentWidth ;
               break ;
            }
            else if( column == len )
            {
               dWidth = max ;
               break ;
            }
         }
         return dWidth ;
      }
      var dire = {
         restrict: 'A',
         scope: {
            data: '=createResponse'
         },
         replace: false,
         priority: 2,
         controller: function( $scope, $element ){
         },
         compile: function( element, attributes ){
            return {
               pre: function preLink( scope, element, attributes ){
                  function onResize()
                  {
                     if( typeof( scope.data.max ) == 'undefined' ) scope.data.max = 0 ;
                     var parent = $( element ).parent() ;
                     if( parent.width() == 0 )
                     {
                        return ;
                     }
                     var newWidth = getLineWidth( parent.width() - parseInt( parent.css( 'padding-left' ) ) - parseInt( parent.css( 'padding-right' ) ),
                                                  scope.data.len,
                                                  scope.data.column,
                                                  scope.data.min,
                                                  scope.data.max ) ;
                     newWidth = newWidth - parseInt( $( element ).css( 'margin-left' ) ) - parseInt( $( element ).css( 'margin-right' ) ) ;
                     $( element ).css( 'float', 'left' ).outerWidth( newWidth ) ;
                  }
                  angular.element( $window ).bind( 'resize', onResize ) ;
                  var listener2 = $rootScope.$watch( 'onResize', function(){
                     setTimeout( onResize ) ;
                  } ) ;
                  var listener3 = scope.$watch( 'data', function(){
                     onResize() ;
                  } ) ;
                  scope.$on( '$destroy', function(){
                     angular.element( $window ).unbind( 'resize', onResize ) ;
                     listener2() ;
                     listener3() ;
                  } ) ;
               },
               post: function postLink( scope, element, attributes ){
               }
            } ;
         }
      } ;
      return dire ;
   });

   //滚动条自动到底部
   sacApp.directive( 'scrollBottom', function( $parse, $window, $timeout, SdbFunction ){
      function createActivationState($parse, attr, scope){
         function unboundState(initValue){
            var activated = initValue;
            return {
                  getValue: function(){
                     return activated;
                  },
                  setValue: function(value){
                     activated = value;
                  }
            };
         }

         function oneWayBindingState(getter, scope){
            return {
                  getValue: function(){
                     return getter(scope);
                  },
                  setValue: function(){}
            }
         }

         function twoWayBindingState(getter, setter, scope){
            return {
                  getValue: function(){
                     return getter(scope);
                  },
                  setValue: function(value){
                     if(value !== getter(scope)){
                        scope.$apply(function(){
                              setter(scope, value);
                        });
                     }
                  }
            };
         }

         if(attr !== ""){
            var getter = $parse(attr);
            if(getter.assign !== undefined){
                  return twoWayBindingState(getter, getter.assign, scope);
            } else {
                  return oneWayBindingState(getter, scope);
            }
         } else {
            return unboundState(true);
         }
      }
      var direction = {
         isAttached: function(el){
            // + 1 catches off by one errors in chrome
            return el.scrollTop + el.clientHeight + 1 >= el.scrollHeight;
         },
         scroll: function(el){
            el.scrollTop = el.scrollHeight;
         }
      } ;
      return {
         priority: 1,
         restrict: 'A',
         link: function(scope, $el, attrs){
            var el = $el[0],
               activationState = createActivationState($parse, attrs['scrollBottom'], scope);

            function scrollIfGlued() {
               if(activationState.getValue() && !direction.isAttached(el)){
                     direction.scroll(el);
               }
            }

            function onScroll() {
               activationState.setValue(direction.isAttached(el));
            }

            scope.$watch( scrollIfGlued ) ;

            $timeout(scrollIfGlued, 0, false);

            var browser = SdbFunction.getBrowserInfo() ;
            if( ( browser[0] == 'ie' && browser[1] >= 9 ) ||  browser[0] != 'ie' )
            {
               //ie7、8不支持addEventListener
               $window.addEventListener('resize', scrollIfGlued, false);
            }

            $el.bind('scroll', onScroll);


            // Remove listeners on directive destroy
            $el.on('$destroy', function() {
               $el.unbind('scroll', onScroll);
            });

            scope.$on('$destroy', function() {
               $window.removeEventListener('resize',scrollIfGlued, false);
            });
         }
      } ;
   } ) ;

   //进度条
   sacApp.directive( 'progressBar', function( $window, $rootScope ){
      var dire = {
         restrict: 'A',
         scope: {
            data: '=progressBar'
         },
         templateUrl: './app/template/Component/ProgressBar.html',
         replace: false,
         controller: function( $scope, $element ){
            $scope.Setting = {
               'style': {
                  'box':      { 'height': '20px', 'background': '#CDD7E1' },
                  'progress': { 'height': '20px', 'background': '#188FF1', 'width': '0%' },
                  'context':  { 'height': '16px', 'padding-top': '4px', 'color': '#fff', 'text-shadow': '#aaa 0 1px 2px' }
               },
               'context': '0%'
            } ;
         },
         compile: function( element, attributes ){
            return {
               pre: function preLink( scope, element, attributes ){
                  var lintener = scope.$watch( 'data', function(){
                     if( typeof( scope.data ) == 'object' )
                     {
                        if( typeof( scope.data.percent ) == 'number' )
                        {
                           scope.Setting.style.progress.width = scope.data.percent + '%' ;
                           if( typeof( scope.data.text ) == 'string' )
                           {
                              scope.Setting.context = scope.data.text ;
                           }
                           else
                           {
                              scope.Setting.context = scope.data.percent + '%' ;
                           }
                        }
                        if( typeof( scope.data.style ) == 'object' )
                        {
                           if( typeof( scope.data.style.box ) == 'object' )
                           {
                              $.each( scope.data.style.box, function( key, val ){
                                 scope.Setting.style.box[key] = val ;
                              } ) ;
                           }
                           if( typeof( scope.data.style.progress ) == 'object' )
                           {
                              $.each( scope.data.style.progress, function( key, val ){
                                 scope.Setting.style.progress[key] = val ;
                              } ) ;
                           }
                           if( typeof( scope.data.style.context ) == 'object' )
                           {
                              $.each( scope.data.style.context, function( key, val ){
                                 scope.Setting.style.context[key] = val ;
                              } ) ;
                           }
                        }
                     }
                  } ) ;
                  scope.$on( '$destroy', function(){
                     lintener() ;
                  } ) ;
               },
               post: function postLink( scope, element, attributes ){
               }
            } ;
         }
      } ;
      return dire ;
   });

   //步骤条
   sacApp.directive( 'stepChart', function( $window, $rootScope ){
      var dire = {
         restrict: 'A',
         scope: {
            data: '=stepChart'
         },
         templateUrl: './app/template/Component/StepChart.html',
         replace: false,
         controller: function( $scope, $element ){
            $scope.Setting = {
               'bar': [],
               'text': []
            } ;
            $scope.onclick = function( index ){
               if( typeof( $scope.data ) == 'object' && typeof( $scope.data['info'][index]['click'] ) == 'function' )
               {
                  $scope.data['info'][index]['click']( index + 1 ) ;
               }
            } ;
         },
         compile: function( element, attributes ){
            return {
               pre: function preLink( scope, element, attributes ){
                  function setChart()
                  {
                     scope.Setting.bar = [] ;
                     if( typeof( scope.data ) == 'object' )
                     {
                        var step = scope.data.step ;
                        var width = $( element ).width() - 170 ;
                        var txtNum = scope.Setting.text.length ;
                        var barNum = txtNum - 1 ;
                        var barWidth = parseInt( width / barNum ) ;
                        for( var i = 0; i < barNum; ++i )
                        {
                           scope.Setting.bar.push( { 'width': barWidth + 'px' } ) ;
                           if( i + 1 >= step )
                           {
                              scope.Setting.bar[i]['background'] = '#ddd' ;
                           }
                        }
                        for( var i = 0; i < txtNum; ++i )
                        {
                           var currentLeft = barWidth * i - 14 ;
                           scope.Setting.text[i]['style'] = { 'left': currentLeft + 'px' } ;
                           if( i + 1 > step )
                           {
                              scope.Setting.text[i]['style']['background'] = '#ddd' ;
                           }
                        }
                     }
                  }
                  var lintener1 = scope.$watch( 'data', function(){
                     if( typeof( scope.data ) == 'object' )
                     {
                        $.each( scope.data.info, function( index, info ){
                           scope.Setting.text.push( { 'text': info.text } ) ;
                        } ) ;
                        setChart() ;
                     }
                  } ) ;
                  angular.element( $window ).bind( 'resize', setChart ) ;
                  var lintener2 = $rootScope.$watch( 'onResize', function(){
                     setTimeout( setChart ) ;
                  } ) ;
                  scope.$on( '$destroy', function(){
                     angular.element( $window ).unbind( 'resize', setChart ) ;
                     lintener1() ;
                     lintener2() ;
                  } ) ;
               },
               post: function postLink( scope, element, attributes ){
               }
            } ;
         }
      } ;
      return dire ;
   });

   //下拉菜单
   sacApp.directive( 'dropdownMenu', function( $window, $rootScope, $compile ){
      var menu = $( '<ul></ul>' ).addClass( 'dropdown-menu' ).appendTo( $( 'body' ) ) ;
      var mask = $( '<div></div>' ).addClass( 'mask-screen unalpha' ).appendTo( $( 'body' ) ).hide().on( 'click', function(){
         $( mask ).hide() ;
         $( menu ).hide().css( { 'overflow-y': 'visible', 'height': 'auto' } ) ;
      } ) ;
      var dire = {
         restrict: 'A',
         scope: {
            data: '=dropdownMenu'
         },
         replace: false,
         controller: function( $scope, $element ){
         },
         compile: function( element, attributes ){
            var menuHide = function(){
               $( menu ).hide().css( { 'overflow-y': 'visible', 'height': 'auto' } ) ;
            }
            var maskHide = function(){
               $( mask ).hide() ;
            }
            var onClickEvent = function( scope, index ){
               if( typeof( scope.data[index]['onClick'] ) == 'function' )
               {
                  scope.data[index]['onClick'](  menuHide, maskHide ) ;
               }
               else
               {
                  $( mask ).hide() ;
                  $( menu ).hide().css( { 'overflow-y': 'visible', 'height': 'auto' } ) ;
               }
            }
            var onMove = function( ele ){
               var bodyWidth  = $( 'body' ).outerWidth() ;
               var bodyHeight = $( 'body' ).outerHeight() ;
               var eleWidth   = $( ele ).outerWidth() ;
               var eleHeight  = $( ele ).outerHeight() ;
               var menuWidth  = $( menu ).outerWidth() ;
               var menuHeight = $( menu ).outerHeight() ;
               var left = $( ele ).offset().left ;
               var top  = $( ele ).offset().top ;
               if( left + menuWidth > bodyWidth )
               {
                  left = bodyWidth - menuWidth ;
               }
               left = left <= 0 ? 0 : left ;
               if( top + eleHeight + menuHeight > bodyHeight )
               {
                  //判断按钮在页面的上面还是下面
                  var isTop = top + parseInt( eleHeight * 0.5 ) <= ( bodyHeight * 0.5 ) ;
                  if( isTop == true )
                  {
                     top = top + eleHeight ;
                     $( menu ).css('overflow-y', 'auto').height( parseInt( ( bodyHeight - top - 20 ) * 0.8 ) ) ;
                  }
                  else
                  {
                     if( top - menuHeight - 3 > 0 )
                     {
                        top = top - menuHeight - 3 ;
                     }
                     else
                     {
                        menuHeight = parseInt( ( top - 20 ) * 0.8 ) ;
                        $( menu ).css('overflow-y', 'auto').height( menuHeight ) ;
                        top = top - menuHeight - 13 ;
                     }
                  }
               }
               else
               {
                  top = top + eleHeight ;
               }
               $( menu ).css( { 'left': left, 'top': top } )  ;
            }
            return {
               pre: function preLink( scope, element, attributes ){
                  $( element ).on( 'click', function(){
                     $( menu ).empty() ;
                     if( typeof( scope.data ) == 'object' )
                     {
                        var rowNum = scope.data.length ;
                        for( var index = 0; index < rowNum; ++index )
                        {
                           if( typeof( scope.data[index]['html'] ) == 'object' )
                           {
                              if( scope.data[index]['disabled'] == true )
                              {
                                 $( '<li></li>' ).addClass( 'drop-disabled' ).append( scope.data[index]['html'] ).appendTo( menu ) ;
                              }
                              else
                              {
                                 (function( index ){
                                    $( '<li></li>' ).addClass( 'event' ).append( scope.data[index]['html'] ).appendTo( menu ).on( 'click', function(){
                                       onClickEvent( scope, index ) ;
                                    } ) ;
                                 })( index ) ;
                              }
                           }
                           else
                           {
                              $( '<li></li>' ).addClass( 'divider' ).appendTo( menu ) ;
                           }
                        }
                        $( mask ).show() ;
                        $( menu ).show().css( { 'left': 0, 'top': 0 } )  ;
                        onMove( element ) ;
                     }
                  } ) ;
                  var onResize = function () {
                     if( $( menu ).is( ':hidden' ) == false )
                     {
                        onMove( element ) ;
                     }
                  }
                  angular.element( $window ).bind( 'resize', onResize ) ;
                  var lintener = $rootScope.$watch( 'onResize', onResize ) ;
                  scope.$on( '$destroy', function(){
                     angular.element( $window ).unbind( 'resize', onResize ) ;
                     lintener() ;
                  } ) ;
               },
               post: function postLink( scope, element, attributes ){
               }
            } ;
         }
      } ;
      return dire ;
   });

   //落地窗
   sacApp.directive( 'frenchWindow', function( $rootScope, $window, $compile ){
      var mask = $( '<div></div>' ).addClass( 'mask-screen unalpha' ) ;
      var boxOffset  = 94 ;
      var bodyOffset = boxOffset + 47 ;
      var dire = {
         restrict: 'A',
         scope: {
            data: '=frenchWindow'
         },
         templateUrl: './app/template/Component/FrenchWindow.html',
         replace: false,
         controller: function( $scope, $element ){
            $( $element ).addClass( 'frenchWindow' ) ;
         },
         compile: function( element, attributes ){
            var headEle = $( '> .header', element ) ;
            var bodyEle = $( '> .body', element ) ;
            var resize = function(){
               var bodyHeight = $( 'body' ).outerHeight() ;
               bodyEle.height( bodyHeight - bodyOffset ) ;
               $( element ).height( bodyHeight - boxOffset ) ;
            }
            return {
               pre: function preLink( scope, element, attributes ){
                  mask.on( 'click', function(){
                     scope.data.isShow = false ;
                     scope.$apply() ;
                  } ) ;
               },
               post: function postLink( scope, element, attributes ){
                  var onResize = function () {
                     if( scope.data.isShow == true )
                     {
                        resize() ;
                     }
                  }
                  var lintener1 = scope.$watch( 'data.isShow', function(){
                     if( typeof( scope.data ) != 'undefined' && scope.data.isShow == true )
                     {
                        headEle.text( scope.data.title ) ;
                        if( scope.data.Context == null )
                        {
                           var bodyHeight = $( 'body' ).outerHeight() ;
                           bodyEle.text( scope.data.empty ) ;
                           bodyEle.css( { 'line-height': ( bodyHeight - bodyOffset ) + 'px', 'text-align': 'center', 'color': '#666' } ) ;
                        }
                        else if( typeof( scope.data.Context ) == 'object' )
                        {
                           bodyEle.css( { 'line-height': 'normal', 'text-align': 'left', 'color': '#000' } ) ;
                           bodyEle.html( scope.data.Context ) ;
                        }
                        else if( typeof( scope.data.Context ) == 'string' )
                        {
                           bodyEle.css( { 'line-height': 'normal', 'text-align': 'left', 'color': '#000' } ) ;
                           bodyEle.html( $compile( scope.data.Context )( scope ) ) ;
                        }
                        resize() ;
                        $( element ).show() ;
                        $( mask ).appendTo( $( 'body' ) ) ;
                     }
                     else
                     {
                        $( element ).hide() ;
                        $( mask ).detach() ;
                     }
                  } ) ;
                  angular.element( $window ).bind( 'resize', onResize ) ;
                  var lintener2 = $rootScope.$watch( 'onResize', onResize ) ;
                  scope.$on( '$destroy', function(){
                     angular.element( $window ).unbind( 'resize', onResize ) ;
                     lintener1() ;
                     lintener2() ;
                  } ) ;
               }
            } ;
         }
      } ;
      return dire ;
   });

   //定时器
   sacApp.directive( 'createTimer', function(){
      var dire = {
         restrict: 'A',
         scope: {
            data: '=createTimer'
         },
         template: '<div ng-style="{\'width\':Setting.options.width,\'background\':Setting.options.backgroundColor,\'height\':Setting.options.height}"></div>',
         replace: true,
         controller: function( $scope, $element ){
            $scope.Setting = {
               options: {
                  width: '0%',
                  height: '2px',
                  backgroundColor: '#00B8E6'
               },
               status: 'stop',
               interval: 5000,
               currentTimer: 0,
               complete: false
            }
         },
         compile: function( element, attributes ){
            return {
               pre: function preLink( scope, element, attributes ){},
               post: function postLink( scope, element, attributes ){
                  var timer = function(){
                     if( scope.Setting.status == 'start' )
                     {
                        scope.Setting.currentTimer += 100 ;
                        var percent = ( scope.Setting.currentTimer / scope.Setting.interval * 100 ) ;
                        scope.Setting.options.width =　percent > 100 ? 100 + '%' : percent + '%' ;
                        if( scope.Setting.currentTimer >= scope.Setting.interval )
                        {
                           scope.$apply() ;
                           if( typeof( scope.data.fn ) == 'function' )
                           {
                              scope.data.fn() ;
                           }
                        }
                        else
                        {
                           setTimeout( timer, 100 ) ;
                        }
                     }
                     else
                     {
                        scope.Setting.options.width = '0%' ;
                        scope.Setting.currentTimer = 0 ;
                     }
                  }
                  var listener1 = scope.$watch( 'data.options', function(){
                     if( typeof( scope.data.options ) == 'object' )
                     {
                        scope.Setting.options = scope.data.options ;
                     }
                  } ) ;
                  var listener2 = scope.$watch( 'data.interval', function(){
                     if( isNaN( scope.data.interval ) == false && scope.data.interval >= 1 )
                     {
                        scope.Setting.interval = scope.data.interval * 1000 ;
                     }
                     else
                     {
                        scope.Setting.interval = 1000 ;
                     }
                  } ) ;
                  var listener3 = scope.$watch( 'data.status', function(){
                     scope.Setting.status = scope.data.status ;
                     if( scope.Setting.status == 'start' )
                     {
                        scope.data.complete = false ;
                        scope.Setting.complete = false ;
                        scope.Setting.currentTimer = 0 ;
                        scope.Setting.options.width = '0%' ;
                        setTimeout( timer, 10 ) ;
                     }
                  } ) ;
                  var listener4 = scope.$watch( 'data.complete', function(){
                     scope.Setting.complete = scope.data.complete ;
                     if( scope.Setting.complete == true && scope.Setting.status == 'start' )
                     {
                        scope.data.complete = false ;
                        scope.Setting.complete = false ;
                        scope.Setting.currentTimer = 0 ;
                        scope.Setting.options.width = '0%' ;
                        setTimeout( timer, 10 ) ;
                     }
                  } ) ;
                  scope.$on( '$destroy', function(){
                     listener1() ;
                     listener2() ;
                     listener3() ;
                     listener4() ;
                  } ) ;
               }
            } ;
         }
      } ;
      return dire ;
   });
}());

/*
模板
sacApp.directive( 'createGrid', function(){
   var dire = {

      // 模式 建议用A 支持 IE6、7
      // E - 元素名称： <my-directive></my-directive>
      // A - 属性名： <div my-directive=”exp”></div>
      // C - class名： <div class=”my-directive:exp;”></div>
      // M - 注释 ： <!-- directive: my-directive exp -->
      restrict: 'A',

      // scope重定义
      scope: {
         data: '=para'
      },

      // 模版 直接写html，也可以用 templateUrl 来异步加载文件
      template: '<p>Hello {{data}}</p>',
      
      // replace - 如果设置为true，那么模版将会替换当前元素，而不是作为子元素添加到当前元素中。（注：为true时，模版必须有一个根节点）
      replace: false,

      // 专用控制器
      controller: function( $scope, $element ){
         $scope.data = $scope.data + "22222 ";
      },

      // 编译
      compile: function( element, attributes ){
         //DOM变形
         //xxx代码xxx
         return {
            pre: function preLink( scope, element, attributes ){
               //scope的一些初始化或者运算
            },
            post: function postLink( scope, element, attributes ){
               //绑定事件等
            }
         } ;
      }
   } ;
   return dire ;
});
*/