// --------------------- Data.Database.Index ---------------------
var _DataDatabaseIndex = {} ;

//控制是否显示子集合
_DataDatabaseIndex.isShowSubCl = function( $scope, SdbFunction, event ){
   var clList = $.extend( true, [], $scope.sourceClList ) ;
   var newClList = [] ;
   $scope.isHideSubCl = $( event.target ).is(':checked') ;
   if( $scope.isHideSubCl == true )
   {
      $.each( clList, function( index, clInfo ){
         if( typeof( clInfo['MainCLName'] ) != 'string' )
         {
            newClList.push( clInfo ) ;
         }
      } ) ;
      SdbFunction.LocalData( 'SdbHidePartition', 1 ) ;
   }
   else
   {
      newClList = clList ;
      SdbFunction.LocalData( 'SdbHidePartition', null ) ;
   }
   $scope.showCSInfo( 0 ) ;
   _DataDatabaseIndex.buildClList( $scope, newClList ) ;
   $.each( $scope.csList, function( index, csInfo ){
      csInfo.show = false ;
   } ) ;
}

//构建cl列表信息
_DataDatabaseIndex.buildClList = function( $scope, clList ){
   $scope.partitionCLNum = 0 ;
   $scope.clList = [] ;
   if( clList.length == 1 && clList[0]['Name'] == null ) clList = [] ;
   $.each( $scope.csList, function( index2, csInfo ){
      csInfo['clNum'] = 0 ;
   } ) ;
   $scope.clInfo = clList ;
   $.each( clList, function( index, clInfo ){
      var fullName = clInfo['Name'].split( '.' ) ;
      var csName = fullName[0] ;
      var clName = fullName[1] ;
      var clListIndex = -1 ;
      //查找cl列表是否已经存在该cl
      $.each( $scope.clList, function( index2, clInfo2 ){
         if( clName == clInfo2['Name'] && csName == clInfo2['csName'] )
         {
            clListIndex = index2 ;
            return false ;
         }
      } ) ;
      //存在则累加
      if( clListIndex >= 0 )
      {
         $scope.clList[clListIndex]['Record']                      += clInfo['TotalRecords'] ;
         $scope.clList[clListIndex]['TotalLobs']                   += clInfo['TotalLobs'] ;
         $scope.clList[clListIndex]['Info']['TotalLobs']           += clInfo['TotalLobs'] ;
         $scope.clList[clListIndex]['Info']['TotalRecords']        += clInfo['TotalRecords'] ;
         $scope.clList[clListIndex]['Info']['TotalDataPages']      += clInfo['TotalDataPages'] ;
         $scope.clList[clListIndex]['Info']['TotalIndexPages']     += clInfo['TotalIndexPages'] ;
         $scope.clList[clListIndex]['Info']['TotalLobPages']       += clInfo['TotalLobPages'] ;
         $scope.clList[clListIndex]['Info']['TotalDataFreeSpace']  += clInfo['TotalDataFreeSpace'] ;
         $scope.clList[clListIndex]['Info']['TotalIndexFreeSpace'] += clInfo['TotalIndexFreeSpace'] ;
         if( clInfo['GroupName'] != null )
         {
            $scope.clList[clListIndex]['GroupName'].push( { 'key': clInfo['GroupName'], 'value': index } ) ;
         }
      }
      //不存在则创建
      else
      {
         var shardingType, shardingTypeDesc ;
         if( clInfo['IsMainCL'] == true )
         {
            shardingType = $scope.autoLanguage( '垂直' ) ;
            shardingTypeDesc = $scope.autoLanguage( '垂直分区' ) ;
         }
         else
         {
            if( clInfo['ShardingType'] == 'range' )
            {
               ++$scope.partitionCLNum ;
               shardingType = $scope.autoLanguage( '水平' ) ;
               shardingTypeDesc = $scope.autoLanguage( '水平范围分区' ) ;
            }
            else if( clInfo['ShardingType'] == 'hash' )
            {
               ++$scope.partitionCLNum ;
               shardingType = $scope.autoLanguage( '水平' ) ;
               shardingTypeDesc = $scope.autoLanguage( '水平散列分区' ) ;
            }
            else
            {
               shardingType = $scope.autoLanguage( '普通' ) ;
               shardingTypeDesc = $scope.autoLanguage( '普通' ) ;
            }
         }
         var isHide = false ;
         var pageSize = 0 ;
         var lobPageSize = 0 ;
         //计算cl数量
         $.each( $scope.csList, function( index2, csInfo ){
            if( csInfo['Name'] == csName )
            {
               ++csInfo['clNum'] ;
               if( csInfo['clNum'] > 5 )
               {
                  isHide = true ;
               }
               pageSize = parseInt( csInfo['Info']['PageSize'] ) ;
               lobPageSize = parseInt( csInfo['Info']['LobPageSize'] ) ;
               return false ;
            }
         } ) ;
         $scope.clList.push( {
            'csName': csName,
            'Name': clName,
            'type': shardingType,
            'typeDesc': shardingTypeDesc,
            'GroupName': ( clInfo['GroupName'] == null ? [] : [ { 'value': index, 'key': clInfo['GroupName'] } ] ),
            'Lob': clInfo['IsMainCL'] ? null : 0,
            'Record': clInfo['TotalRecords'],
            'TotalLobs': clInfo['IsMainCL'] ? null : clInfo['TotalLobs'],
            'Index': clInfo['Indexes'],
            'IsMainCL': clInfo['IsMainCL'],
            'hide': isHide,
            'pageSize': pageSize,
            'lobPageSize': lobPageSize,
            'MainCLName': clInfo['MainCLName'],
            'ShardingType': clInfo['ShardingType'],
            'Info': {
               'Name':                clName,
               'IsMainCL':            clInfo['IsMainCL'],
               'MainCLName':          clInfo['MainCLName'],
               'ShardingKey':         clInfo['ShardingKey'],
               'ShardingType':        clInfo['ShardingType'],
               'ID':                  clInfo['ID'],
               'LogicalID':           clInfo['LogicalID'],
               'Sequence':            clInfo['Sequence'],
               'GroupName':           clInfo['GroupName'],
               'Status':              clInfo['Status'],
               'Indexes':             clInfo['Indexes'],
               'TotalRecords':        clInfo['TotalRecords'],
               'TotalLobs':           clInfo['TotalLobs'],
               'TotalDataPages':      clInfo['TotalDataPages'],
               'TotalIndexPages':     clInfo['TotalIndexPages'],
               'TotalLobPages':       clInfo['TotalLobPages'],
               'TotalDataFreeSpace':  clInfo['TotalDataFreeSpace'],
               'TotalIndexFreeSpace': clInfo['TotalIndexFreeSpace'],
               'EnsureShardingIndex': clInfo['EnsureShardingIndex'],
               'ReplSize':            clInfo['ReplSize'],
               'LowBound':            null,
               'UpBound':             null,
               'Metadata Ratio':      '0%',
               'CataInfo':            clInfo['CataInfo'],
               'Attribute':           $scope.moduleMode == 'standalone' ? clInfo['Attribute'] : clInfo['AttributeDesc'],
               'CompressionType':     $scope.moduleMode == 'standalone' ? clInfo['CompressionType'] : clInfo['CompressionTypeDesc'],
               'HasDict':             clInfo['HasDict']
            }
         } ) ;
      }
      clInfo['Name'] = clName ;
      clInfo['TotalDataFreeSpace'] = fixedNumber( clInfo['TotalDataFreeSpace'], 2 ) + 'MB' ;
      clInfo['TotalIndexFreeSpace'] = fixedNumber( clInfo['TotalIndexFreeSpace'], 2 ) + 'MB' ;
   } ) ;
   $.each( $scope.clList, function( index, clInfo ){
      if( typeof( clInfo['Info']['TotalDataFreeSpace'] ) == 'number' )
      {
         clInfo['Info']['TotalDataFreeSpace'] = fixedNumber( clInfo['Info']['TotalDataFreeSpace'], 2 ) + 'MB' ;
      }
      if( typeof( clInfo['Info']['TotalIndexFreeSpace'] ) == 'number' )
      {
         clInfo['Info']['TotalIndexFreeSpace'] = fixedNumber( clInfo['Info']['TotalIndexFreeSpace'], 2 ) + 'MB' ;
      }
      if( typeof( clInfo['Info']['TotalIndexPages'] ) == 'number' && typeof( clInfo['Info']['TotalDataPages'] ) == 'number' && typeof( clInfo['Info']['TotalLobPages'] ) == 'number' && clInfo['Info']['TotalDataPages'] + clInfo['Info']['TotalLobPages'] > 0 )
      {
         clInfo['Info']['Metadata Ratio'] = fixedNumber( ( clInfo['Info']['TotalIndexPages'] * 100 * clInfo['pageSize'] ) / ( clInfo['Info']['TotalDataPages'] * clInfo['pageSize'] + clInfo['Info']['TotalLobPages'] * clInfo['lobPageSize'] ), 2 ) + '%' ;
      }
   } ) ;
}

//获取所有cs信息
_DataDatabaseIndex.getCSInfo = function( $scope, SdbRest ){
   var sql ;
   if( $scope.moduleMode == 'standalone' )
   {
      sql = 'SELECT Name, PageSize/1024, LobPageSize/1024, GroupName, TotalRecords, FreeDataSize/1048576, FreeIndexSize/1048576, FreeLobSize/1048576, FreeSize/1048576, MaxDataCapSize/1073741824, MaxIndexCapSize/1073741824, MaxLobCapSize/1073741824, TotalDataSize/1048576, TotalIndexSize/1048576, TotalLobSize/1048576, TotalSize/1048576 FROM $SNAPSHOT_CS ORDER BY Name' ;
   }
   else
   {
      sql = 'SELECT Name, PageSize/1024, LobPageSize/1024, GroupName, TotalRecords, FreeDataSize/1048576, FreeIndexSize/1048576, FreeLobSize/1048576, FreeSize/1048576, MaxDataCapSize/1073741824, MaxIndexCapSize/1073741824, MaxLobCapSize/1073741824, TotalDataSize/1048576, TotalIndexSize/1048576, TotalLobSize/1048576, TotalSize/1048576 FROM $SNAPSHOT_CS WHERE NodeSelect="master" ORDER BY Name' ;
   }
   function dataSizeFmt( value, str )
   {
      if( typeof( value ) == 'number' )
      {
         return fixedNumber( value, 2 ) + str ;
      }
      else
      {
         return value ;
      }
   }
   function dataPlus( value )
   {
      if( typeof( value ) == 'number' )
      {
         return value ;
      }
      else
      {
         return 0 ;
      }
   }
   //获取cs的信息
   SdbRest.Exec( sql, function( csList ){
      if( csList.length == 1 && csList[0]['Name'] == null ) csList = [] ;
      var data = { 'cmd': 'list collectionspaces', 'sort': JSON.stringify( { 'Name': 1 } ) } ;
      SdbRest.DataOperation( data, function( csList2 ){
         $.each( csList2, function( index2, csInfo2 ){
            var hasCS = false ;
            $.each( csList, function( index, csInfo ){
               if( csInfo['Name'] == csInfo2['Name'] )
               {
                  hasCS = true ;
                  return false ;
               }
            } ) ;
            if( hasCS == false )
            {
               csList.push( csInfo2 ) ;
            }
         } ) ;
         $scope.csList = [] ;
         $scope.csInfo = csList ;
         $.each( csList, function( index, csInfo ){
            var csListIndex = -1 ;
            //查找cs列表是否已经存在该cs
            $.each( $scope.csList, function( index2, csInfo2 ){
               if( csInfo['Name'] == csInfo2['Name'] )
               {
                  csListIndex = index2 ;
                  return false ;
               }
            } ) ;
            //存在则累加
            if( csListIndex >= 0 )
            {
               $scope.csList[csListIndex]['Info']['TotalRecords']    += dataPlus( csInfo['TotalRecords'] ) ;
               $scope.csList[csListIndex]['Info']['TotalDataSize']   += dataPlus( csInfo['TotalDataSize'] ) ;
               $scope.csList[csListIndex]['Info']['FreeDataSize']    += dataPlus( csInfo['FreeDataSize'] ) ;
               $scope.csList[csListIndex]['Info']['TotalIndexSize']  += dataPlus( csInfo['TotalIndexSize'] ) ;
               $scope.csList[csListIndex]['Info']['FreeIndexSize']   += dataPlus( csInfo['FreeIndexSize'] ) ;
               $scope.csList[csListIndex]['Info']['TotalLobSize']    += dataPlus( csInfo['TotalLobSize'] ) ;
               $scope.csList[csListIndex]['Info']['FreeLobSize']     += dataPlus( csInfo['FreeLobSize'] ) ;
               $scope.csList[csListIndex]['Info']['MaxDataCapSize']  += dataPlus( csInfo['MaxDataCapSize'] ) ;
               $scope.csList[csListIndex]['Info']['MaxIndexCapSize'] += dataPlus( csInfo['MaxIndexCapSize'] ) ;
               $scope.csList[csListIndex]['Info']['MaxLobCapSize']   += dataPlus( csInfo['MaxLobCapSize'] ) ;
               $scope.csList[csListIndex]['Info']['TotalSize']       += dataPlus( csInfo['TotalSize'] ) ;
               $scope.csList[csListIndex]['Info']['FreeSize']        += dataPlus( csInfo['FreeSize'] ) ;
               if( csInfo['GroupName'] != null )
               {
                  $scope.csList[csListIndex]['GroupName'].push( { 'key': csInfo['GroupName'], 'value': index } ) ;
               }
            }
            //不存在则创建
            else
            {
               var color = '' ;
               var i = $scope.csList.length ;
               if( i > 3 ) i = i % 4 ;
               if( i == 0 ) color = 'green' ;
               if( i == 1 ) color = 'yellow' ;
               if( i == 2 ) color = 'blue' ;
               if( i == 3 ) color = 'violet' ;
               $scope.csList.push( {
                  'Name': csInfo['Name'],
                  'clNum': 0,
                  'GroupName': ( csInfo['GroupName'] == null ? [] : [ { 'value': index, 'key': csInfo['GroupName'] } ] ),
                  'hide': false,
                  'color': color,
                  'show': false,
                  'Info': {
                     'Name':            csInfo['Name'],
                     'PageSize':        csInfo['PageSize'],
                     'LobPageSize':     csInfo['LobPageSize'],
                     'TotalRecords':    csInfo['TotalRecords'],
                     'FreeDataSize':    csInfo['FreeDataSize'],
                     'FreeIndexSize':   csInfo['FreeIndexSize'],
                     'FreeLobSize':     csInfo['FreeLobSize'],
                     'FreeSize':        csInfo['FreeSize'],
                     'MaxDataCapSize':  csInfo['MaxDataCapSize'],
                     'MaxIndexCapSize': csInfo['MaxIndexCapSize'],
                     'MaxLobCapSize':   csInfo['MaxLobCapSize'],
                     'TotalDataSize':   csInfo['TotalDataSize'],
                     'TotalIndexSize':  csInfo['TotalIndexSize'],
                     'TotalLobSize':    csInfo['TotalLobSize'],
                     'TotalSize':       csInfo['TotalSize']
                  }
               } ) ;
            }
            csInfo['PageSize']        = dataSizeFmt( csInfo['PageSize'], 'KB' ) ;
            csInfo['LobPageSize']     = dataSizeFmt( csInfo['LobPageSize'], 'KB' ) ;
            csInfo['TotalRecords']    = dataSizeFmt( csInfo['TotalRecords'], '' ) ;
            csInfo['TotalDataSize']   = dataSizeFmt( csInfo['TotalDataSize'], 'MB' ) ;
            csInfo['FreeDataSize']    = dataSizeFmt( csInfo['FreeDataSize'], 'MB' ) ;
            csInfo['TotalIndexSize']  = dataSizeFmt( csInfo['TotalIndexSize'], 'MB' ) ;
            csInfo['FreeIndexSize']   = dataSizeFmt( csInfo['FreeIndexSize'], 'MB' ) ;
            csInfo['TotalLobSize']    = dataSizeFmt( csInfo['TotalLobSize'], 'MB' ) ;
            csInfo['FreeLobSize']     = dataSizeFmt( csInfo['FreeLobSize'], 'MB' ) ;
            csInfo['TotalSize']       = dataSizeFmt( csInfo['TotalSize'], 'MB' ) ;
            csInfo['FreeSize']        = dataSizeFmt( csInfo['FreeSize'], 'MB' ) ;
            csInfo['MaxDataCapSize']  = dataSizeFmt( csInfo['MaxDataCapSize'], 'GB' ) ;
            csInfo['MaxIndexCapSize'] = dataSizeFmt( csInfo['MaxIndexCapSize'], 'GB' ) ;
            csInfo['MaxLobCapSize']   = dataSizeFmt( csInfo['MaxLobCapSize'], 'GB' ) ;
         } ) ;

         $.each( $scope.csList, function( index, csInfo ){
            csInfo['Info']['PageSize']        = dataSizeFmt( csInfo['Info']['PageSize'], 'KB' ) ;
            csInfo['Info']['LobPageSize']     = dataSizeFmt( csInfo['Info']['LobPageSize'], 'KB' ) ;
            csInfo['Info']['TotalRecords']    = dataSizeFmt( csInfo['Info']['TotalRecords'], '' ) ;
            csInfo['Info']['TotalDataSize']   = dataSizeFmt( csInfo['Info']['TotalDataSize'], 'MB' ) ;
            csInfo['Info']['FreeDataSize']    = dataSizeFmt( csInfo['Info']['FreeDataSize'], 'MB' ) ;
            csInfo['Info']['TotalIndexSize']  = dataSizeFmt( csInfo['Info']['TotalIndexSize'], 'MB' ) ;
            csInfo['Info']['FreeIndexSize']   = dataSizeFmt( csInfo['Info']['FreeIndexSize'], 'MB' ) ;
            csInfo['Info']['TotalLobSize']    = dataSizeFmt( csInfo['Info']['TotalLobSize'], 'MB' ) ;
            csInfo['Info']['FreeLobSize']     = dataSizeFmt( csInfo['Info']['FreeLobSize'], 'MB' ) ;
            csInfo['Info']['TotalSize']       = dataSizeFmt( csInfo['Info']['TotalSize'], 'MB' ) ;
            csInfo['Info']['FreeSize']        = dataSizeFmt( csInfo['Info']['FreeSize'], 'MB' ) ;
            csInfo['Info']['MaxDataCapSize']  = dataSizeFmt( csInfo['Info']['MaxDataCapSize'], 'GB' ) ;
            csInfo['Info']['MaxIndexCapSize'] = dataSizeFmt( csInfo['Info']['MaxIndexCapSize'], 'GB' ) ;
            csInfo['Info']['MaxLobCapSize']   = dataSizeFmt( csInfo['Info']['MaxLobCapSize'], 'GB' ) ;
         } ) ;
         $scope.showCSInfo( 0 ) ;
         $scope.$apply() ;
         _DataDatabaseIndex.getCLInfo( $scope, SdbRest ) ;
      }, function( errorInfo ){
         _IndexPublic.createRetryModel( $scope, errorInfo, function(){
            _DataDatabaseIndex.getCSInfo( $scope, SdbRest ) ;
            return true ;
         } ) ;
      }, function(){
         _IndexPublic.createErrorModel( $scope, $scope.autoLanguage( '网络连接错误，请尝试按F5刷新浏览器。' ) ) ;
      } ) ;
   }, function( errorInfo ){
      _IndexPublic.createRetryModel( $scope, errorInfo, function(){
         _DataDatabaseIndex.getCSInfo( $scope, SdbRest ) ;
         return true ;
      } ) ;
   }, function(){
      _IndexPublic.createErrorModel( $scope, $scope.autoLanguage( '网络连接错误，请尝试按F5刷新浏览器。' ) ) ;
   } ) ;
}

//获取所有cl信息
_DataDatabaseIndex.getCLInfo = function( $scope, SdbRest )
{
   $scope.subCLNum = 0 ;
   $scope.mainCLNum = 0 ;
   var sql ;
   if( $scope.moduleMode == 'standalone' )
   {
      sql = 'SELECT t1.Name, t1.Details.ID, t1.Details.LogicalID, t1.Details.Sequence, t1.Details.Status,t1.Details.Attribute, t1.Details.Status,t1.Details.CompressionType, t1.Details.CompressionType, t1.Details.Indexes, t1.Details.TotalRecords, t1.Details.TotalLobs, t1.Details.TotalDataPages, t1.Details.HasDict, t1.Details.TotalIndexPages, t1.Details.TotalLobPages, t1.Details.TotalDataFreeSpace/1048576, t1.Details.TotalIndexFreeSpace/1048576 FROM (SELECT * FROM $SNAPSHOT_CL split BY Details) AS t1 ORDER BY t1.Name' ;
   }
   else
   {
      sql = 'SELECT t1.Name, t1.Details.ID, t1.Details.LogicalID, t1.Details.Sequence, t1.Details.GroupName, t1.Details.Status, t1.Details.Indexes, t1.Details.TotalRecords, t1.Details.TotalLobs, t1.Details.TotalDataPages, t1.Details.HasDict, t1.Details.TotalIndexPages, t1.Details.TotalLobPages, t1.Details.TotalDataFreeSpace/1048576, t1.Details.TotalIndexFreeSpace/1048576, t1.IsMainCL, t1.MainCLName, t1.ShardingKey, t1.ShardingType FROM (SELECT * FROM $SNAPSHOT_CL WHERE NodeSelect="master" split BY Details) AS t1 ORDER BY t1.Name' ;
   }

   //合并cl数据
   function mergedData( clList, cataList ){
      $.each( cataList, function( index, cataInfo ){
         if( cataInfo['IsMainCL'] == true )
         {
            ++$scope.mainCLNum ;
            clList.push( cataInfo ) ;
            cataInfo['TotalRecords'] = 0 ;
            $.each( cataInfo['CataInfo'], function( index2, rangeInfo ){
               $.each( clList, function( index3, clInfo ){
                  if( clInfo['Name'] == rangeInfo['SubCLName'] )
                  {
                     cataInfo['TotalRecords'] += clInfo['TotalRecords'] ;
                  }
               } ) ;
               rangeInfo['LowBound'] = JSON.stringify( rangeInfo['LowBound'] ) ;
               rangeInfo['UpBound'] = JSON.stringify( rangeInfo['UpBound'] ) ;
            } ) ;
         }
         else
         {
            $.each( clList, function( index2, clInfo ){
               if( clInfo['Name'] == cataInfo['Name'] )
               {
                  if( typeof( cataInfo['MainCLName'] ) == 'string' )
                  {
                     ++$scope.subCLNum ;
                     clInfo['MainCLName'] = cataInfo['MainCLName'] ;
                  }
                  if( typeof( cataInfo['ShardingType'] ) == 'string' )
                  {
                     clInfo['ShardingType'] = cataInfo['ShardingType'] ;
                     clInfo['ShardingKey'] = cataInfo['ShardingKey'] ;
                     clInfo['LowBound'] = [] ;
                     clInfo['UpBound'] = [] ;
                     clInfo['ReplSize'] = cataInfo['ReplSize'] ;
                     clInfo['EnsureShardingIndex'] = cataInfo['EnsureShardingIndex'] ;
                     $.each( cataInfo['CataInfo'], function( index3, groupInfo ){
                        if( groupInfo['GroupName'] == clInfo['GroupName'] )
                        {
                           clInfo['LowBound'].push( JSON.stringify( groupInfo['LowBound'] ) ) ;
                           clInfo['UpBound'].push( JSON.stringify( groupInfo['UpBound'] ) ) ;
                        }
                     } ) ;
                  }
                  if( typeof( cataInfo['ReplSize'] ) != 'undefined' )
                  {
                     clInfo['ReplSize'] = cataInfo['ReplSize'] ;
                  }
                  if( typeof( cataInfo['AttributeDesc'] ) != 'undefined' )
                  {
                     clInfo['AttributeDesc'] = cataInfo['AttributeDesc'] ;
                  }
                  if( typeof( cataInfo['CompressionTypeDesc'] ) != 'undefined' )
                  {
                     clInfo['CompressionTypeDesc'] = cataInfo['CompressionTypeDesc'] ;
                  }
               }
            } ) ;
         }
      } ) ;
      $scope.sourceClList = $.extend( true, [], clList ) ;
      var newClList = [] ;
      if( $scope.isHideSubCl == true )
      {
         $.each( clList, function( index, clInfo ){
            if( typeof( clInfo['MainCLName'] ) != 'string' )
            {
               newClList.push( clInfo ) ;
            }
         } ) ;
      }
      else
      {
         newClList = clList ;
      }
      return newClList ;
   }

   //查询成功执行的
   function success( clList ){
      _DataDatabaseIndex.buildClList( $scope, clList ) ;
      var clNum = $scope.clList.length ;
      var csNum = $scope.csList.length ;
      var test = 0 ;
      var boxHeight = 0 ;
      if( csNum > 0 && $scope.clList.length > 0 && $scope.moduleMode == 'distribution' && $scope.partitionCLNum > 0 && $scope.clList.length > $scope.mainCLNum + $scope.subCLNum && $scope.mainCLNum > 0 )
      {
         boxHeight = -263 ;
      }
      else if( csNum > 0 && $scope.clList.length > 0 && $scope.moduleMode == 'distribution' && $scope.partitionCLNum > 0 )
      {
         boxHeight = -235 ;
      }
      else if( csNum > 0 && $scope.clList.length > 0 && $scope.moduleMode == 'distribution' && $scope.clList.length > $scope.mainCLNum + $scope.subCLNum && $scope.mainCLNum > 0 )
      {
         boxHeight = -235 ;
      }
      else if(  csNum > 0 && clNum > 0  )
      {
         boxHeight = -207 ;
      }
      else if( csNum > 0 )
      {
         boxHeight = -123 ;
      }
      else if( csNum == 0 )
      {
         boxHeight = -67 ;
      }
      $scope.boxHeight = {'offsetY': boxHeight } ;
      
   }

   //获取cl信息
   SdbRest.Exec( sql, function( clList ){
      if( $scope.moduleMode == 'standalone' )
      {
         success( clList ) ;
      }
      else
      {
         sql = 'select * from $SNAPSHOT_CATA' ;
         SdbRest.Exec( sql, function( cataList ){
            var newList = mergedData( clList, cataList ) ;
            success( newList ) ;
         }, function( errorInfo ){
            _IndexPublic.createRetryModel( $scope, errorInfo, function(){
               _DataDatabaseIndex.getCLInfo( $scope, SdbRest ) ;
               return true ;
            } ) ;
         }, function(){
            _IndexPublic.createErrorModel( $scope, $scope.autoLanguage( '网络连接错误，请尝试按F5刷新浏览器。' ) ) ;
         } ) ;
      }
   }, function( errorInfo ){
      _IndexPublic.createRetryModel( $scope, errorInfo, function(){
         _DataDatabaseIndex.getCLInfo( $scope, SdbRest ) ;
         return true ;
      } ) ;
   }, function(){
      _IndexPublic.createErrorModel( $scope, $scope.autoLanguage( '网络连接错误，请尝试按F5刷新浏览器。' ) ) ;
   } ) ;
}

//初始化参数
_DataDatabaseIndex.init = function( $scope, clusterName, moduleName, moduleMode, isHideSubCl ){
   //分区组列表
   $scope.GroupList = [] ;
   //集群名
   $scope.clusterName = clusterName ;
   //业务名
   $scope.moduleName = moduleName ;
   //业务模式
   $scope.moduleMode = moduleMode ;
   //收起cl列表时，最大显示cl数
   $scope.maxShowCLNum = 5 ;
   //当前选中的cs的ID
   $scope.csID = 0 ;
   //当前选中的cl的ID
   $scope.clID = 0 ;
   //cs列表
   $scope.csList = [] ;
   //cs详细信息
   $scope.csInfo = [] ;
   //原始cl列表
   $scope.sourceClList = [] ;
   //cl列表
   $scope.clList = [] ;
   //cl详细信息
   $scope.clInfo = [] ;
   //参数列表
   $scope.attr = {} ;
   //当前所选的group
   $scope.selectGroup = null ;
   //搜索内容
   $scope.find = '' ;
   //符合搜索的数量
   $scope.findNum = 0 ;
   //是否隐藏子集合
   $scope.isHideSubCl = isHideSubCl ;
   //主表数量
   $scope.mainCLNum = 0 ;
   //子表数量
   $scope.subCLNum = 0 ;
   //分区表数量
   $scope.partitionCLNum = 0 ;
   //右侧高度偏移量
   $scope.boxHeight = {'offsetY': -263 } ;
}

//获取分区组列表
_DataDatabaseIndex.getGroupList = function( $scope, SdbRest ){
   if( $scope.moduleMode == 'distribution' )
   {
      //获取分区组列表
      var exec = function(){
         var data = { 'cmd': 'list groups', 'sort': JSON.stringify( { 'GroupName': 1 } ) } ;
         SdbRest.DataOperation( data, function( groups ){
            $.each( groups, function( index, groupInfo ){
               if( groupInfo['Role'] == 0 )
               {
                  $scope.GroupList.push( groupInfo ) ;
               }
            } ) ;
         }, function( errorInfo ){
            _IndexPublic.createRetryModel( $scope, errorInfo, function(){
               exec() ;
               return true ;
            } ) ;
         }, function(){
            _IndexPublic.createErrorModel( $scope, $scope.autoLanguage( '网络连接错误，请尝试按F5刷新浏览器。' ) ) ;
         } ) ;
      } ;
      exec() ;
   }
}

//跳到记录页面
_DataDatabaseIndex.gotoRecord = function( $scope, $location, SdbFunction, csIndex, clIndex ){
   var csName = $scope.csList[csIndex]['Name'] ;
   var clName = $scope.clList[clIndex]['Info']['Name'] ;
   var clType = '' ;
   if( $scope.clList[clIndex]['Info']['IsMainCL'] == true )
   {
      clType = 'main' ;
   }
   SdbFunction.LocalData( 'SdbCsName', csName ) ;
   SdbFunction.LocalData( 'SdbClName', clName ) ;
   SdbFunction.LocalData( 'SdbClType', clType ) ;
   $location.path( 'Data/SDB-Operate/Record' ) ;
}

//跳到lob页面
_DataDatabaseIndex.gotoLob = function( $scope, $location, SdbFunction, csIndex, clIndex ){
   var csName = $scope.csList[csIndex]['Name'] ;
   var clName = $scope.clList[clIndex]['Info']['Name'] ;
   var clType = '' ;
   if( $scope.clList[clIndex]['Info']['IsMainCL'] == true )
   {
      clType = 'main' ;
   }
   SdbFunction.LocalData( 'SdbCsName', csName ) ;
   SdbFunction.LocalData( 'SdbClName', clName ) ;
   SdbFunction.LocalData( 'SdbClType', clType ) ;
   $location.path( 'Data/SDB-Operate/Lobs' ) ;
}

//搜索cs和cl
_DataDatabaseIndex.searchCSAndCL = function( $scope, fullName ){
   $scope.findNum = 0 ;
   $scope.find = fullName ;
   var isNumber = !isNaN( fullName ) ;
   var len = fullName.length ;
   var pointIndex = fullName.indexOf( '.' ) ;
   fullName = trim( fullName ) ;
   if( len > 0 )
   {
      //xxx.???
      if( pointIndex > 0 )
      {
         //xxx.xxx
         if( pointIndex < len - 1 )
         {
            var csName = '', clName = '' ;
            var tmp = fullName.split( '.' ) ;
            csName = tmp[0] ;
            clName = tmp[1] ;
            $.each( $scope.csList, function( index, csInfo ){
               csInfo.hide = !( csInfo['Name'] == csName ) ;
            } ) ;
            $.each( $scope.clList, function( index, clInfo ){
               clInfo.hide = !( clInfo['Name'] == clName && clInfo['csName'] == csName ) ;
            } ) ;
         }
         //xxx.
         else
         {
            var csName = '' ;
            var tmp = fullName.split( '.' ) ;
            csName = tmp[0] ;
            $.each( $scope.csList, function( index, csInfo ){
               csInfo.hide = !( csInfo['Name'] == csName ) ;
            } ) ;
            $.each( $scope.clList, function( index, clInfo ){
               clInfo.hide = !( clInfo['csName'] == csName ) ;
            } ) ;
         }
      }
      //.xxx
      else if( pointIndex == 0 )
      {
         //.
         if( len == 0 )
         {
            $.each( $scope.clList, function( index, clInfo ){
               clInfo.hide = false ;
            } ) ;
            $.each( $scope.csList, function( index, csInfo ){
               csInfo.hide = false ;
            } ) ;
         }
         //.xx
         else
         {
            var clName = '' ;
            var tmp = fullName.split( '.' ) ;
            clName = tmp[1] ;
            $.each( $scope.csList, function( index, csInfo ){
               csInfo.hide = true ;
            } ) ;
            $.each( $scope.clList, function( index, clInfo ){
               clInfo.hide = true
               if( clInfo['Name'] == clName )
               {
                  clInfo.hide = false ;
                  $.each( $scope.csList, function( index, csInfo ){
                     if( csInfo['Name'] == clInfo['csName'] )
                     {
                        csInfo.hide = false ;
                        return false ;
                     }
                  } ) ;
               }
            } ) ;
         }
      }
      //xxx
      else
      {
         $.each( $scope.csList, function( index, csInfo ){
            csInfo.hide = true ;
         } ) ;
         $.each( $scope.clList, function( index, clInfo ){
            clInfo.hide = true ;
            if( clInfo['Name'].indexOf( fullName ) >= 0 )
            {
               clInfo.hide = false ;
               $.each( $scope.csList, function( index, csInfo ){
                  if( csInfo['Name'] == clInfo['csName'] )
                  {
                     csInfo.hide = false ;
                     return false ;
                  }
               } ) ;
            }
         } ) ;
         $.each( $scope.csList, function( index, csInfo ){
            if( csInfo['Name'].indexOf( fullName ) >= 0 )
            {
               csInfo.hide = false ;
               $.each( $scope.clList, function( index, clInfo ){
                  if( clInfo['csName'] == csInfo['Name'] )
                  {
                     clInfo.hide = false ;
                  }
               } ) ;
            }
         } ) ;
         if( isNumber )
         {
            var num = parseInt( fullName ) ;
            $.each( $scope.clList, function( index, clInfo ){
               if( clInfo['TotalLobs'] == num || clInfo['Record'] == num || clInfo['Index'] == num )
               {
                  clInfo.hide = false ;
                  $.each( $scope.csList, function( index, csInfo ){
                     if( csInfo['Name'] == clInfo['csName'] )
                     {
                        csInfo.hide = false ;
                        return false ;
                     }
                  } ) ;
               }
            } ) ;
         }
      }
   }
   else
   {
      $.each( $scope.clList, function( index, clInfo ){
         clInfo.hide = false ;
      } ) ;
      $.each( $scope.csList, function( index, csInfo ){
         csInfo.hide = false ;
         $scope.clTableHide( csInfo['Name'] ) ;
      } ) ;
   }
   $.each( $scope.csList, function( index, csInfo ){
      if( csInfo.hide == false )
      {
         ++$scope.findNum ;
      }
   } ) ;
}

//展开cs下所有的cl
_DataDatabaseIndex.clTableShow = function( $scope, csName ){
   $.each( $scope.clList, function( index, clInfo ){
      if( clInfo.csName == csName )
      {
         clInfo.hide = false ;
      }
   } ) ;
   $.each( $scope.csList, function( index, csInfo ){
      if( csInfo.Name == csName )
      {
         csInfo.show = true ;
         return false ;
      }
   } ) ;
}

//收起cs下所有的cl
_DataDatabaseIndex.clTableHide = function( $scope, csName ){
   var showNum = 0 ;
   $.each( $scope.clList, function( index, clInfo ){
      if( clInfo.csName == csName )
      {
         ++showNum ;
         clInfo.hide = ( showNum > $scope.maxShowCLNum ) ;
         if( $scope.showType == 'cl' && $scope.clID == index )
         {
            clInfo.hide = false ;
         }
      }
   } ) ;
   $.each( $scope.csList, function( index, csInfo ){
      if( csInfo.Name == csName )
      {
         csInfo.show = false ;
         return false ;
      }
   } ) ;
}

//展示CS属性
_DataDatabaseIndex.showCSInfo = function( $scope, index ){
   $scope.showType = 'cs' ;
   $scope.csID = index ;
   $scope.selectGroup = null ;
   $scope.attr = $.extend( true, {}, $scope.csList[index] ) ;
}

//展示CL属性
_DataDatabaseIndex.showCLInfo = function( $scope, csIndex, clIndex ){
   $scope.showType = 'cl' ;
   $scope.csID = csIndex ;
   $scope.clID = clIndex ;
   $scope.selectGroup = null ;
   $scope.attr = $.extend( true, {}, $scope.clList[clIndex] ) ;
}

//打开 创建集合空间 的窗口
_DataDatabaseIndex.showCreateCS = function( $scope, SdbRest ){
   if( _IndexPublic.checkEditionAndSupport( $scope, 'sequoiadb', 'Metadata' ) == false )
   {
      return ;
   }
   $scope.Components.Modal.icon = 'fa-plus' ;
   $scope.Components.Modal.title = $scope.autoLanguage( '创建集合空间' ) ;
   $scope.Components.Modal.isShow = true ;
   $scope.Components.Modal.form = {
      inputList: [
         {
            "name": "name",
            "webName": $scope.autoLanguage( '集合空间名' ),
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
            "name": "PageSize",
            "webName": $scope.autoLanguage( '数据页大小' ),
            "type": "select",
            "desc": $scope.autoLanguage( '数据页大小创建后不可更改。' ),
            "value": "65536",
            "valid": [
               { "key": "4KB", "value": "4096" },
               { "key": "8KB", "value": "8192" },
               { "key": "16KB", "value": "16384" },
               { "key": "32KB", "value": "32768" },
               { "key": "64KB", "value": "65536" }
            ]
         },
         {
            "name": "Domain",
            "webName": $scope.autoLanguage( '所属域' ),
            "type": "string",
            "desc": $scope.autoLanguage( '所属域必须已经存在。' ),
            "value": "",
            "valid": {
               "min": 0,
               "ban": "SYSDOMAIN"
            }
         },
         {
            "name": "LobPageSize",
            "webName": $scope.autoLanguage( 'Lob数据页大小' ),
            "type": "select",
            "desc": $scope.autoLanguage( 'Lob数据页大小创建后不可更改。' ),
            "value": "262144",
            "valid": [
               { "key": "4KB", "value": "4096" },
               { "key": "8KB", "value": "8192" },
               { "key": "16KB", "value": "16384" },
               { "key": "32KB", "value": "32768" },
               { "key": "64KB", "value": "65536" },
               { "key": "128KB", "value": "131072" },
               { "key": "256KB", "value": "262144" },
               { "key": "512KB", "value": "524288" }
            ]
         }
      ]
   } ;
   $scope.Components.Modal.Context = '<div form-create para="data.form"></div>' ;
   $scope.Components.Modal.ok = function(){
      var isAllClear = $scope.Components.Modal.form.check() ;
      if( isAllClear )
      {
         var value = $scope.Components.Modal.form.getValue() ;
         var options = { 'PageSize': autoTypeConvert( value['PageSize'] ), 'LobPageSize': autoTypeConvert( value['LobPageSize'] ) } ;
         if( value['Domain'].length > 0 )
         {
            options['Domain'] = value['Domain'] ;
         }
         var data = { 'cmd': 'create collectionspace', 'name': value['name'], 'options': JSON.stringify( options ) } ;
         var exec = function(){
            SdbRest.DataOperation( data, function( json ){
               _DataDatabaseIndex.getCSInfo( $scope, SdbRest ) ;
            }, function( errorInfo ){
               _IndexPublic.createRetryModel( $scope, errorInfo, function(){
                  exec() ;
                  return true ;
               } ) ;
            }, function(){
               _IndexPublic.createErrorModel( $scope, $scope.autoLanguage( '网络连接错误，请尝试按F5刷新浏览器。' ) ) ;
            } ) ;
         } ;
         exec() ;
      }
      return isAllClear ;
   }
}

//打开 删除集合空间 的窗口
_DataDatabaseIndex.showRemoveCS = function( $scope, SdbRest ){
   if( _IndexPublic.checkEditionAndSupport( $scope, 'sequoiadb', 'Metadata' ) == false )
   {
      return ;
   }
   var csValid = [] ;
   //获取集合空间名显示在列表中
   $.each( $scope.csList, function( index, csInfo ){
      csValid.push( { 'key': csInfo['Name'], 'value': index } ) ;
   } ) ;
   $scope.Components.Modal.icon = 'fa-trash-o' ;
   $scope.Components.Modal.title = $scope.autoLanguage( '删除集合空间' ) ;
   $scope.Components.Modal.isShow = true ;
   $scope.Components.Modal.form = {
      inputList: [
         {
            "name": "name",
            "webName": $scope.autoLanguage( '集合空间名' ),
            "type": "select",
            "value": csValid[ $scope.csID ]['value'],
            "desc": $scope.autoLanguage( '选择集合空间' ),
            "valid": csValid,
            "onChange": function( name, key, value ){
               $scope.Components.Modal.Table = $scope.csList[value]['Info'] ;
            }
         }
      ]
   } ;
   $scope.Components.Modal.Table = $scope.csList[ $scope.csID ]['Info'] ;
   $scope.Components.Modal.Context = '\
<div form-create para="data.form"></div>\
<table class="table loosen border">\
<tr>\
<td style="width:40%;background-color:#F1F4F5;"><b>Key</b></td>\
<td style="width:60%;background-color:#F1F4F5;"><b>Value</b></td>\
</tr>\
<tr ng-repeat="(key, value) in data.Table track by $index" ng-if="key != \'Name\'">\
<td>{{key}}</td>\
<td>{{value}}</td>\
</tr>\
</table>' ;
   $scope.Components.Modal.ok = function(){
      var isAllClear = $scope.Components.Modal.form.check() ;
      if( isAllClear )
      {
         var value = $scope.Components.Modal.form.getValue() ;
         var csName = $scope.csList[ value['name'] ]['Name'] ;
         var data = { 'cmd': 'drop collectionspace', 'name': csName } ;
         var exec = function(){
            SdbRest.DataOperation( data, function( json ){
               if( $scope.csID == value['name'] )
               {
                  $scope.showCSInfo( 0 ) ;
               }
               _DataDatabaseIndex.getCSInfo( $scope, SdbRest ) ;
            }, function( errorInfo ){
               _IndexPublic.createRetryModel( $scope, errorInfo, function(){
                  exec() ;
                  return true ;
               } ) ;
            }, function(){
               _IndexPublic.createErrorModel( $scope, $scope.autoLanguage( '网络连接错误，请尝试按F5刷新浏览器。' ) ) ;
            } ) ;
         } ;
         exec() ;
      }
      return isAllClear ;
   }
}

//打开 创建集合 的窗口
_DataDatabaseIndex.showCreateCL = function( $scope, SdbRest ){
   if( _IndexPublic.checkEditionAndSupport( $scope, 'sequoiadb', 'Metadata' ) == false )
   {
      return ;
   }
   //把获取表单的值转换成请求参数
   var modalValue2Create = function( valueJson ){
      var fullName = $scope.csList[ valueJson['csName'] ]['Name'] + '.' + valueJson['clName'] ;
      var rv = { 'name': fullName, 'options': {} } ;
      if( $scope.moduleMode == 'distribution' )
      {
         if( valueJson['type'] == '' )
         {
            rv['options']['Compressed'] = valueJson['Compressed'] == 0 ? false : true ;
            if( rv['options']['Compressed'] == true )
            {
               if( valueJson['Compressed'] == 1 )
               {
                  rv['options']['CompressionType'] = 'snappy' ;
               }
               else if( valueJson['Compressed'] == 2 )
               {
                  rv['options']['CompressionType'] = 'lzw' ;
               }
            }
            rv['options']['ReplSize'] = valueJson['ReplSize'] ;
            if( valueJson['Group'] > 0 )
            {
               rv['options']['Group'] = $scope.GroupList[ valueJson['Group'] - 1 ]['GroupName'] ;
            }
         }
         else if( valueJson['type'] == 'range' )
         {
            rv['options']['ShardingType'] = 'range' ;
            rv['options']['ShardingKey'] = {} ;
            $.each( valueJson['ShardingKey'], function( index, field ){
               rv['options']['ShardingKey'][ field['field'] ] = field['sort'] ;
            } ) ;
            rv['options']['ReplSize'] = valueJson['ReplSize'] ;
            rv['options']['Compressed'] = valueJson['Compressed'] == 0 ? false : true ;
            if( rv['options']['Compressed'] == true )
            {
               if( valueJson['Compressed'] == 1 )
               {
                  rv['options']['CompressionType'] = 'snappy' ;
               }
               else if( valueJson['Compressed'] == 2 )
               {
                  rv['options']['CompressionType'] = 'lzw' ;
               }
            }
            if( valueJson['Group'] > 0 )
            {
               rv['options']['Group'] = $scope.GroupList[ valueJson['Group'] - 1 ]['GroupName'] ;
            }
            rv['options']['EnsureShardingIndex'] = valueJson['EnsureShardingIndex'] ;
         }
         else if( valueJson['type'] == 'hash' )
         {
            rv['options']['ShardingType'] = 'hash' ;
            rv['options']['ShardingKey'] = {} ;
            $.each( valueJson['ShardingKey'], function( index, field ){
               rv['options']['ShardingKey'][ field['field'] ] = field['sort'] ;
            } ) ;
            rv['options']['Partition'] = valueJson['Partition'] ;
            rv['options']['ReplSize'] = valueJson['ReplSize'] ;
            rv['options']['Compressed'] = valueJson['Compressed'] == 0 ? false : true ;
            if( rv['options']['Compressed'] == true )
            {
               if( valueJson['Compressed'] == 1 )
               {
                  rv['options']['CompressionType'] = 'snappy' ;
               }
               else if( valueJson['Compressed'] == 2 )
               {
                  rv['options']['CompressionType'] = 'lzw' ;
               }
            }
            rv['options']['AutoSplit'] = valueJson['AutoSplit'] ;
            if( valueJson['Group'] > 0 )
            {
               rv['options']['Group'] = $scope.GroupList[ valueJson['Group'] - 1 ]['GroupName'] ;
            }
            rv['options']['EnsureShardingIndex'] = valueJson['EnsureShardingIndex'] ;
         }
         else if( valueJson['type'] == 'main' )
         {
            rv['options']['IsMainCL'] = true ;
            rv['options']['ShardingKey'] = {} ;
            $.each( valueJson['ShardingKey'], function( index, field ){
               rv['options']['ShardingKey'][ field['field'] ] = field['sort'] ;
            } ) ;
            rv['options']['ReplSize'] = valueJson['ReplSize'] ;
         }
      }
      else if( $scope.moduleMode == 'standalone' )
      {
         rv['options']['Compressed'] = valueJson['Compressed'] == 0 ? false : true ;
         if( rv['options']['Compressed'] == true )
            {
               if( valueJson['Compressed'] == 1 )
               {
                  rv['options']['CompressionType'] = 'snappy' ;
               }
               else if( valueJson['Compressed'] == 2 )
               {
                  rv['options']['CompressionType'] = 'lzw' ;
               }
            }
      }
      return rv ;
   }
   var createCLExec = function( data ){
      SdbRest.DataOperation( data, function( json ){
         _DataDatabaseIndex.getCSInfo( $scope, SdbRest ) ;
      }, function( errorInfo ){
         _IndexPublic.createRetryModel( $scope, errorInfo, function(){
            createCLExec( data ) ;
            return true ;
         } ) ;
         _DataDatabaseIndex.getCLInfo( $scope, SdbRest ) ;
      }, function(){
         _IndexPublic.createErrorModel( $scope, $scope.autoLanguage( '网络连接错误，请尝试按F5刷新浏览器。' ) ) ;
      } ) ;
   }
   var csValid = [] ;
   //获取集合空间名显示在列表中
   $.each( $scope.csList, function( index, csInfo ){
      csValid.push( { 'key': csInfo['Name'], 'value': index } ) ;
   } ) ;
   var groupValid = [ { "key": "Auto", "value": 0 } ] ;
   //获取分区组列表
   $.each( $scope.GroupList, function( index, groupInfo ){
      groupValid.push( { 'key': groupInfo['GroupName'], 'value': index + 1 } ) ;
   } ) ;
   $scope.Components.Modal.icon = 'fa-plus' ;
   $scope.Components.Modal.title = $scope.autoLanguage( '创建集合' ) ;
   $scope.Components.Modal.isShow = true ;
   if( $scope.moduleMode == 'distribution' )
   {
      $scope.Components.Modal.formShow = 1 ;
      $scope.Components.Modal.form1 = {
         inputList: [
            {
                 
               "name": "csName",
               "webName": $scope.autoLanguage( '集合空间' ),
               "type": "select",
               "value": $scope.csID,
               'valid': csValid
            },
            {
                 
               "name": "type",
               "webName": $scope.autoLanguage( '集合类型' ),
               "type": "select",
               "value": "",
               "valid": [
                  { "key": $scope.autoLanguage( '普通' ), "value": "" },
                  { "key": $scope.autoLanguage( '水平范围分区' ), "value": "range" },
                  { "key": $scope.autoLanguage( '水平散列分区' ), "value": "hash" },
                  { "key": $scope.autoLanguage( '垂直分区' ), "value": "main" }         
               ],
               "onChange": function( name, key, value ){
                  if( value == '' )
                  {
                     $scope.Components.Modal.formShow = 1 ;
                  }
                  else if( value == 'range' )
                  {
                     $scope.Components.Modal.formShow = 2 ;
                  }
                  else if( value == 'hash' )
                  {
                     $scope.Components.Modal.formShow = 3 ;
                  }
                  else if( value == 'main' )
                  {
                     $scope.Components.Modal.formShow = 4 ;
                  }
                  $scope.Components.Modal.form1.inputList[1]['value'] = '' ;
                  $scope.Components.Modal.form2.inputList[0]['value'] = $scope.Components.Modal.form1.inputList[0]['value'] ;
                  $scope.Components.Modal.form3.inputList[0]['value'] = $scope.Components.Modal.form1.inputList[0]['value'] ;
                  $scope.Components.Modal.form4.inputList[0]['value'] = $scope.Components.Modal.form1.inputList[0]['value'] ;
                  $scope.Components.Modal.form2.inputList[2]['value'] = $scope.Components.Modal.form1.inputList[2]['value'] ;
                  $scope.Components.Modal.form3.inputList[2]['value'] = $scope.Components.Modal.form1.inputList[2]['value'] ;
                  $scope.Components.Modal.form4.inputList[2]['value'] = $scope.Components.Modal.form1.inputList[2]['value'] ;
               }
            },
            {
               "name": "clName",
               "webName": $scope.autoLanguage( '集合名' ),
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
               "name": "ReplSize",
               "webName":  $scope.autoLanguage( '副本数' ),
               "type": "int",
               "required": true,
               "value": 1,
               "valid": {
                  "min": -1,
                  "max": 7
               }
            },
            {
               "name": "Compressed",
               "webName":  $scope.autoLanguage( '数据压缩' ),
               "type": "select",
               "required": false,
               "value": 0,
               "valid": [
                  { "key": $scope.autoLanguage( '关' ), "value": 0 },
                  { "key": $scope.autoLanguage( 'Snappy' ), "value": 1 },
                  { "key": $scope.autoLanguage( 'LZW' ), "value": 2 }
               ]
            },
            {
               "name": "Group",
               "webName":  $scope.autoLanguage( '分区组' ),
               "type": "select",
               "value": 0,
               "valid": groupValid
            }
         ]
      } ;
      $scope.Components.Modal.form2 = {
         inputList: [
            {
               "name": "csName",
               "webName": $scope.autoLanguage( '集合空间' ),
               "type": "select",
               "value": $scope.csID,
               'valid': csValid
            },
            {
               "name": "type",
               "webName": $scope.autoLanguage( '集合类型' ),
               "type": "select",
               "value": "range",
               "valid": [
                  { "key": $scope.autoLanguage( '普通' ), "value": "" },
                  { "key": $scope.autoLanguage( '水平范围分区' ), "value": "range" },
                  { "key": $scope.autoLanguage( '水平散列分区' ), "value": "hash" },
                  { "key": $scope.autoLanguage( '垂直分区' ), "value": "main" }         
               ],
               "onChange": function( name, key, value ){
                  if( value == '' )
                  {
                     $scope.Components.Modal.formShow = 1 ;
                  }
                  else if( value == 'range' )
                  {
                     $scope.Components.Modal.formShow = 2 ;
                  }
                  else if( value == 'hash' )
                  {
                     $scope.Components.Modal.formShow = 3 ;
                  }
                  else if( value == 'main' )
                  {
                     $scope.Components.Modal.formShow = 4 ;
                  }
                  $scope.Components.Modal.form2.inputList[1]['value'] = 'range' ;
                  $scope.Components.Modal.form1.inputList[0]['value'] = $scope.Components.Modal.form2.inputList[0]['value'] ;
                  $scope.Components.Modal.form3.inputList[0]['value'] = $scope.Components.Modal.form2.inputList[0]['value'] ;
                  $scope.Components.Modal.form4.inputList[0]['value'] = $scope.Components.Modal.form2.inputList[0]['value'] ;
                  $scope.Components.Modal.form1.inputList[2]['value'] = $scope.Components.Modal.form2.inputList[2]['value'] ;
                  $scope.Components.Modal.form3.inputList[2]['value'] = $scope.Components.Modal.form2.inputList[2]['value'] ;
                  $scope.Components.Modal.form4.inputList[2]['value'] = $scope.Components.Modal.form2.inputList[2]['value'] ;
               }
            },
            {
               "name": "clName",
               "webName": $scope.autoLanguage( '集合名' ),
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
               "name": "ShardingKey",
               "webName":  $scope.autoLanguage( '分区键' ),
               "required": true,
               "desc": $scope.autoLanguage( '分区键是一个或多个集合列的有续集和，其中的值用来确定每个集合所述的数据分区。' ),
               "type": "list",
               "valid": {
                  "min": 1
               },
               "child":[
                  [
                     {
                        "name": "field",
                        "webName": $scope.autoLanguage( "字段名" ),
                        "type": "string",
                        "value": "",
                        "valid": {
                           "min": 1,
                           "regex": "^[^/$].*",
                           "ban": "."
                        }
                     },
                     {
                        "name": "sort",
                        "type": "select",
                        "value": 1,
                        "valid": [
                           { "key": $scope.autoLanguage( '升序' ), "value": 1 },
                           { "key": $scope.autoLanguage( '降序' ), "value": -1 }
                        ]
                     }
                  ]
               ]
            },
            {
               "name": "ReplSize",
               "webName":  $scope.autoLanguage( '副本数' ),
               "type": "int",
               "required": true,
               "value": 1,
               "valid": {
                  "min": -1,
                  "max": 7
               }
            },
            {
               "name": "Compressed",
               "webName":  $scope.autoLanguage( '数据压缩' ),
               "type": "select",
               "required": false,
               "value": 0,
               "valid": [
                  { "key": $scope.autoLanguage( '关' ), "value": 0 },
                  { "key": $scope.autoLanguage( 'Snappy' ), "value": 1 },
                  { "key": $scope.autoLanguage( 'LZW' ), "value": 2 }
               ]
            },
            {
               "name": "Group",
               "webName":  $scope.autoLanguage( '分区组' ),
               "type": "select",
               "value": 0,
               "valid": groupValid
            },
            {
               "name": "EnsureShardingIndex",
               "webName":  $scope.autoLanguage( '分区索引' ),
               "type": "select",
               "required": false,
               "desc": "",
               "value": true,
               "valid": [
                  { "key": $scope.autoLanguage( '开' ), "value": true },
                  { "key": $scope.autoLanguage( '关' ), "value": false }
               ]
            }
         ]
      } ;
      $scope.Components.Modal.form3 = {
         inputList: [
            {
               "name": "csName",
               "webName": $scope.autoLanguage( '集合空间' ),
               "type": "select",
               "value": $scope.csID,
               'valid': csValid
            },
            {
               "name": "type",
               "webName": $scope.autoLanguage( '集合类型' ),
               "type": "select",
               "value": "hash",
               "valid": [
                  { "key": $scope.autoLanguage( '普通' ), "value": "" },
                  { "key": $scope.autoLanguage( '水平范围分区' ), "value": "range" },
                  { "key": $scope.autoLanguage( '水平散列分区' ), "value": "hash" },
                  { "key": $scope.autoLanguage( '垂直分区' ), "value": "main" }         
               ],
               "onChange": function( name, key, value ){
                  if( value == '' )
                  {
                     $scope.Components.Modal.formShow = 1 ;
                  }
                  else if( value == 'range' )
                  {
                     $scope.Components.Modal.formShow = 2 ;
                  }
                  else if( value == 'hash' )
                  {
                     $scope.Components.Modal.formShow = 3 ;
                  }
                  else if( value == 'main' )
                  {
                     $scope.Components.Modal.formShow = 4 ;
                  }
                  $scope.Components.Modal.form3.inputList[1]['value'] = 'hash' ;
                  $scope.Components.Modal.form1.inputList[0]['value'] = $scope.Components.Modal.form3.inputList[0]['value'] ;
                  $scope.Components.Modal.form2.inputList[0]['value'] = $scope.Components.Modal.form3.inputList[0]['value'] ;
                  $scope.Components.Modal.form4.inputList[0]['value'] = $scope.Components.Modal.form3.inputList[0]['value'] ;
                  $scope.Components.Modal.form1.inputList[2]['value'] = $scope.Components.Modal.form3.inputList[2]['value'] ;
                  $scope.Components.Modal.form2.inputList[2]['value'] = $scope.Components.Modal.form3.inputList[2]['value'] ;
                  $scope.Components.Modal.form4.inputList[2]['value'] = $scope.Components.Modal.form3.inputList[2]['value'] ;
               }
            },
            {
               "name": "clName",
               "webName": $scope.autoLanguage( '集合名' ),
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
               "name": "ShardingKey",
               "webName":  $scope.autoLanguage( '分区键' ),
               "required": true,
               "desc": $scope.autoLanguage( '分区键是一个或多个集合列的有续集和，其中的值用来确定每个集合所述的数据分区。' ),
               "type": "list",
               "valid": {
                  "min": 1
               },
               "child":[
                  [
                     {
                        "name": "field",
                        "webName": $scope.autoLanguage( "字段名" ),
                        "type": "string",
                        "value": "",
                        "valid": {
                           "min": 1,
                           "regex": "^[^/$].*",
                           "ban": "."
                        }
                     },
                     {
                        "name": "sort",
                        "type": "select",
                        "value": 1,
                        "valid": [
                           { "key": $scope.autoLanguage( '升序' ), "value": 1 },
                           { "key": $scope.autoLanguage( '降序' ), "value": -1 }
                        ]
                     }
                  ]
               ]
            },
            {
               "name": "Partition",
               "webName":  $scope.autoLanguage( '分区数' ),
               "type": "int",
               "required": true,
               "value": 4096,
               "valid": {
                  "min": 8,
                  "max": 1048576,
                  "step": 2
               }
            },
            {
               "name": "ReplSize",
               "webName":  $scope.autoLanguage( '副本数' ),
               "type": "int",
               "required": true,
               "value": 1,
               "valid": {
                  "min": -1,
                  "max": 7
               }
            },
            {
               "name": "Compressed",
               "webName":  $scope.autoLanguage( '数据压缩' ),
               "type": "select",
               "required": false,
               "value": 0,
               "valid": [
                  { "key": $scope.autoLanguage( '关' ), "value": 0 },
                  { "key": $scope.autoLanguage( 'Snappy' ), "value": 1 },
                  { "key": $scope.autoLanguage( 'LZW' ), "value": 2 }
               ]
            },
            {
               "name": "AutoSplit",
               "webName":  $scope.autoLanguage( '自动切分' ),
               "type": "select",
               "value": false,
               "valid": [
                  { "key": $scope.autoLanguage( '开' ), "value": true },
                  { "key": $scope.autoLanguage( '关' ), "value": false }
               ]
            },
            {
               "name": "Group",
               "webName":  $scope.autoLanguage( '分区组' ),
               "type": "select",
               "value": 0,
               "valid": groupValid
            },
            {
               "name": "EnsureShardingIndex",
               "webName":  $scope.autoLanguage( '分区索引' ),
               "type": "select",
               "required": false,
               "desc": "",
               "value": true,
               "valid": [
                  { "key": $scope.autoLanguage( '开' ), "value": true },
                  { "key": $scope.autoLanguage( '关' ), "value": false }
               ]
            }
         ]
      } ;
      $scope.Components.Modal.form4 = {
         inputList: [
            {
                 
               "name": "csName",
               "webName": $scope.autoLanguage( '集合空间' ),
               "type": "select",
               "value": $scope.csID,
               'valid': csValid
            },
            {
                 
               "name": "type",
               "webName": $scope.autoLanguage( '集合类型' ),
               "type": "select",
               "value": "main",
               "valid": [
                  { "key": $scope.autoLanguage( '普通' ), "value": "" },
                  { "key": $scope.autoLanguage( '水平范围分区' ), "value": "range" },
                  { "key": $scope.autoLanguage( '水平散列分区' ), "value": "hash" },
                  { "key": $scope.autoLanguage( '垂直分区' ), "value": "main" }         
               ],
               "onChange": function( name, key, value ){
                  if( value == '' )
                  {
                     $scope.Components.Modal.formShow = 1 ;
                  }
                  else if( value == 'range' )
                  {
                     $scope.Components.Modal.formShow = 2 ;
                  }
                  else if( value == 'hash' )
                  {
                     $scope.Components.Modal.formShow = 3 ;
                  }
                  else if( value == 'main' )
                  {
                     $scope.Components.Modal.formShow = 4 ;
                  }
                  $scope.Components.Modal.form4.inputList[1]['value'] = '' ;
                  $scope.Components.Modal.form1.inputList[0]['value'] = $scope.Components.Modal.form4.inputList[0]['value'] ;
                  $scope.Components.Modal.form2.inputList[0]['value'] = $scope.Components.Modal.form4.inputList[0]['value'] ;
                  $scope.Components.Modal.form3.inputList[0]['value'] = $scope.Components.Modal.form4.inputList[0]['value'] ;
                  $scope.Components.Modal.form1.inputList[2]['value'] = $scope.Components.Modal.form4.inputList[2]['value'] ;
                  $scope.Components.Modal.form2.inputList[2]['value'] = $scope.Components.Modal.form4.inputList[2]['value'] ;
                  $scope.Components.Modal.form3.inputList[2]['value'] = $scope.Components.Modal.form4.inputList[2]['value'] ;
               }
            },
            {
               "name": "clName",
               "webName": $scope.autoLanguage( '集合名' ),
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
               "name": "ShardingKey",
               "webName":  $scope.autoLanguage( '分区键' ),
               "required": true,
               "desc": $scope.autoLanguage( '分区键是一个或多个集合列的有续集和，其中的值用来确定每个集合所述的数据分区。' ),
               "type": "list",
               "valid": {
                  "min": 1
               },
               "child":[
                  [
                     {
                        "name": "field",
                        "webName": $scope.autoLanguage( "字段名" ),
                        "type": "string",
                        "value": "",
                        "valid": {
                           "min": 1,
                           "regex": "^[^/$].*",
                           "ban": "."
                        }
                     },
                     {
                        "name": "sort",
                        "type": "select",
                        "value": 1,
                        "valid": [
                           { "key": $scope.autoLanguage( '升序' ), "value": 1 },
                           { "key": $scope.autoLanguage( '降序' ), "value": -1 }
                        ]
                     }
                  ]
               ]
            },
            {
               "name": "ReplSize",
               "webName":  $scope.autoLanguage( '副本数' ),
               "type": "int",
               "required": true,
               "value": 1,
               "valid": {
                  "min": -1,
                  "max": 7
               }
            }
         ]
      } ;
      $scope.Components.Modal.Context = '\
<div ng-if="data.formShow == 1" form-create para="data.form1"></div>\
<div ng-if="data.formShow == 2" form-create para="data.form2"></div>\
<div ng-if="data.formShow == 3" form-create para="data.form3"></div>\
<div ng-if="data.formShow == 4" form-create para="data.form4"></div>' ;
      $scope.Components.Modal.ok = function(){
         if( $scope.Components.Modal.formShow == 1 )
         {
            var isAllClear = $scope.Components.Modal.form1.check() ;
            if( isAllClear )
            {
               var value = $scope.Components.Modal.form1.getValue() ;
               var data = modalValue2Create( value ) ;
               data['cmd'] = 'create collection' ;
               data['options'] = JSON.stringify( data['options'] ) ;
               createCLExec( data ) ;
            }
         }
         else if( $scope.Components.Modal.formShow == 2 )
         {
            var isAllClear = $scope.Components.Modal.form2.check() ;
            if( isAllClear )
            {
               var value = $scope.Components.Modal.form2.getValue() ;
               var data = modalValue2Create( value ) ;
               data['cmd'] = 'create collection' ;
               data['options'] = JSON.stringify( data['options'] ) ;
               createCLExec( data ) ;
            }
         }
         else if( $scope.Components.Modal.formShow == 3 )
         {
            var isAllClear = $scope.Components.Modal.form3.check( function( valueJson ){
               var rv = [] ;
               if( valueJson['AutoSplit'] == true && valueJson['Group'] > 0 )
               {
                  rv.push( { 'name': 'AutoSplit', 'error': sprintf( $scope.autoLanguage( '指定了?，不能开启?。' ), $scope.autoLanguage( '分区组' ), $scope.autoLanguage( '自动切分' ) ) } ) ;
                  rv.push( { 'name': 'Group', 'error': sprintf( $scope.autoLanguage( '开启了?，不能指定?。' ), $scope.autoLanguage( '自动切分' ), $scope.autoLanguage( '分区组' ) ) } ) ;
               }
               return rv ;
            } ) ;
            if( isAllClear )
            {
               var value = $scope.Components.Modal.form3.getValue() ;
               var data = modalValue2Create( value ) ;
               data['cmd'] = 'create collection' ;
               data['options'] = JSON.stringify( data['options'] ) ;
               createCLExec( data ) ;
            }
         }
         else if( $scope.Components.Modal.formShow == 4 )
         {
            var isAllClear = $scope.Components.Modal.form4.check() ;
            if( isAllClear )
            {
               var value = $scope.Components.Modal.form4.getValue() ;
               var data = modalValue2Create( value ) ;
               data['cmd'] = 'create collection' ;
               data['options'] = JSON.stringify( data['options'] ) ;
               createCLExec( data ) ;
            }
         }
         return isAllClear ;
      }
   }
   else if( $scope.moduleMode == 'standalone' )
   {
      $scope.Components.Modal.form = {
         inputList: [
            {
                 
               "name": "csName",
               "webName": $scope.autoLanguage( '集合空间' ),
               "type": "select",
               "value": $scope.csID,
               'valid': csValid
            },
            {
               "name": "clName",
               "webName": $scope.autoLanguage( '集合名' ),
               "type": "string",
               "required": true,
               "value": "",
               "valid": {
                  "min": 1,
                  "ban": "."
               }
            },
            {
               "name": "Compressed",
               "webName":  $scope.autoLanguage( '数据压缩' ),
               "type": "select",
               "required": false,
               "value": 0,
               "valid": [
                  { "key": $scope.autoLanguage( '关' ), "value": 0 },
                  { "key": $scope.autoLanguage( 'Snappy' ), "value": 1 },
                  { "key": $scope.autoLanguage( 'LZW' ), "value": 2 }
               ]
            }
         ]
      } ;
      $scope.Components.Modal.Context = '<div form-create para="data.form"></div>' ;
      $scope.Components.Modal.ok = function(){
         var isAllClear = $scope.Components.Modal.form.check() ;
         if( isAllClear )
         {
            var value = $scope.Components.Modal.form.getValue() ;
            var data = modalValue2Create( value ) ;
            data['cmd'] = 'create collection' ;
            data['options'] = JSON.stringify( data['options'] ) ;
            createCLExec( data ) ;
         }
         return isAllClear ;
      }
   }
}

//打开 删除集合 的窗口
_DataDatabaseIndex.showRemoveCL = function( $scope, SdbRest ){
   if( _IndexPublic.checkEditionAndSupport( $scope, 'sequoiadb', 'Metadata' ) == false )
   {
      return ;
   }
   var clValid = [] ;
   var clIndex = -1 ;
   $.each( $scope.clList, function( index, clInfo ){
      if( $scope.showType == 'cs' )
      {
         if( $scope.csList[ $scope.csID ]['Name'] == clInfo['csName'] && clIndex < 0 )
         {
            clIndex = index ;
         }
      }
      else
      {
         if( index == $scope.clID )
         {
            clIndex = index ;
         }
      }
      clValid.push( { 'key' : clInfo['csName'] + '.' + clInfo['Name'] , 'value' : index } );
   } ) ;
   $scope.Components.Modal.icon = 'fa-trash-o' ;
   $scope.Components.Modal.title = $scope.autoLanguage( '删除集合' ) ;
   $scope.Components.Modal.isShow = true ;
   $scope.Components.Modal.form = {
      inputList: [
         {
            "name": "name",
            "webName": $scope.autoLanguage( '集合名' ),
            "type": "select",
            "value": clIndex,
            "valid": clValid,
            "onChange": function( name, key, value ){
               $scope.Components.Modal.Table = $scope.clList[value]['Info'] ;
            }
         }
      ]
   } ;
   $scope.Components.Modal.Table = $scope.clList[ clIndex ]['Info'] ;
   var maskContext = '\
<div form-create para="data.form"></div>\
<table class="table loosen border">\
<tr>\
<td style="width:40%;background-color:#F1F4F5;"><b>Key</b></td>\
<td style="width:60%;background-color:#F1F4F5;"><b>Value</b></td>\
</tr>\
<tr ng-repeat="(key, value) in data.Table track by $index" ng-if="key != \'Name\' && key != \'CataInfo\' && key != \'GroupName\' && value != undefined">\
<td>{{key}}</td>\
<td>{{value}}</td>\
</tr>\
</table>';
   $scope.Components.Modal.Context = maskContext ;
   $scope.Components.Modal.ok = function(){
      var isAllClear = $scope.Components.Modal.form.check() ;
      if( isAllClear )
      {
         var value = $scope.Components.Modal.form.getValue() ;
         var fullName = $scope.clList[ value['name'] ]['csName'] + '.' + $scope.clList[ value['name'] ]['Name'] ;
         var data = { 'cmd': 'drop collection', 'name': fullName } ;
         var exec = function(){
            SdbRest.DataOperation( data, function( json ){
               if( $scope.clID == value['name'] )
               {
                  $scope.clID = 0 ;
                  $scope.showCSInfo( $scope.csID ) ;
               }
               _DataDatabaseIndex.getCSInfo( $scope, SdbRest ) ;
            }, function( errorInfo ){
               _IndexPublic.createRetryModel( $scope, errorInfo, function(){
                  exec() ;
                  return true ;
               } ) ;
            }, function(){
               _IndexPublic.createErrorModel( $scope, $scope.autoLanguage( '网络连接错误，请尝试按F5刷新浏览器。' ) ) ;
            } ) ;
         } ;
         exec() ;
      }
      return isAllClear ;
   }
}

//打开 挂载集合 的窗口
_DataDatabaseIndex.showAttachCL = function( $scope, SdbRest ){
   if( _IndexPublic.checkEditionAndSupport( $scope, 'sequoiadb', 'Metadata' ) == false )
   {
      return ;
   }
   var mainCL = [] ;
   var childCL = [] ;
   //获取主分区集合
   $.each( $scope.clList, function( index, clInfo ){
      if( clInfo['IsMainCL'] == true )
      {
         var id = mainCL.length ;
         mainCL.push( { 'key' : clInfo['csName'] + '.' + clInfo['Name'] , 'value' : id } ) ;
      }
      else
      {
         var id = childCL.length ;
         if( typeof( clInfo['MainCLName'] ) != 'string' )
         {
            childCL.push( { 'key' : clInfo['csName'] + '.' + clInfo['Name'] , 'value' : id } ) ;
         }
      }
   } ) ;
   $scope.Components.Modal.icon = 'fa-paperclip' ;
   $scope.Components.Modal.title = $scope.autoLanguage( '挂载集合' ) ;
   $scope.Components.Modal.isShow = true ;
   $scope.Components.Modal.form = {
      inputList: [
         {
            "name": "name",
            "webName": $scope.autoLanguage( '集合' ),
            "type": "select",
            "value": 0,
            "valid": mainCL
         },
         {
            "name": "attachName",
            "webName": $scope.autoLanguage( '分区' ),
            "type": "select",
            "value": 0,
            "desc": $scope.autoLanguage( '选择在主分区集合下挂载的分区。' ),
            "valid": childCL
         },
         {
            "name": "range",
            "webName":  $scope.autoLanguage( '分区范围' ),
            "required": true,
            "desc": $scope.autoLanguage( '分区范围，包含两个字段“LowBound”（区间左值，填$minKey为负无穷）以及“UpBound”（区间右值，填$maxKey为正无穷），例如：{LowBound:{a:0},UpBound:{a:100}表示取字段“a”的范围区间：[0, 100)。' ),
            "type": "list",
            "valid": {
               "min": 1
            },
            "child":[
               [
                  {
                     "name": "field",
                     "webName": $scope.autoLanguage( "字段名" ),
                     "placeholder": $scope.autoLanguage( "字段名" ),
                     "type": "string",
                     "value": "",
                     "valid": {
                        "min": 1,
                        "regex": "^[^/$].*",
                        "ban": "."
                     }
                  },
                  {
                     "name": "type",
                     "webName": $scope.autoLanguage( "类型" ),
                     "placeholder": $scope.autoLanguage( "类型" ),
                     "type": "select",
                     "value": "Auto",
                     "valid": [
                        { "key": "Auto",      "value": "Auto" },
                        { "key": "Bool",      "value": "Bool" },
                        { "key": "Number",    "value": "Number" },
                        { "key": "Decimal",   "value": "Decimal" },
                        { "key": "String",    "value": "String" },
                        { "key": "ObjectId",  "value": "ObjectId" },
                        { "key": "Regex",     "value": "Regex" },
                        { "key": "Binary",    "value": "Binary" },
                        { "key": "Timestamp", "value": "Timestamp" },
                        { "key": "Date",      "value": "Date" }
                     ]
                  },
                  {
                     "name": "LowBound",
                     "webName": $scope.autoLanguage( "区间左值" ),
                     "placeholder": $scope.autoLanguage( "区间左值" ),
                     "type": "string",
                     "value": "",
                     "selectList": [ '$minKey' ]
                  },
                  {
                     "name": "UpBound",
                     "webName": $scope.autoLanguage( "区间右值" ),
                     "placeholder": $scope.autoLanguage( "区间右值" ),
                     "type": "string",
                     "value": "",
                     "selectList": [ '$maxKey' ]
                  }
               ]
            ]
         }
      ]
   } ;
   $scope.Components.Modal.Context = '<div form-create para="data.form"></div>' ;
   $scope.Components.Modal.ok = function(){
      var isAllClear = $scope.Components.Modal.form.check() ;
      if( isAllClear )
      {
         var value = $scope.Components.Modal.form.getValue() ;
         var lowbound = {} ;
         var upbound = {} ;
         $.each( value['range'], function( index, fieldInfo ){
            var fieldName = fieldInfo['field'] ;

            if( fieldInfo['type'] == 'Auto' )
            {
               lowbound[ fieldName ] = autoTypeConvert( fieldInfo['LowBound'], true ) ;
               upbound[ fieldName ]  = autoTypeConvert( fieldInfo['UpBound'], true ) ;
            }
            else
            {
               if( fieldInfo['LowBound'].toLowerCase() == '$minkey' )
               {
                  lowbound[ fieldName ] = { '$minKey': 1 } ;
               }
               else
               {
                  lowbound[ fieldName ] = specifyTypeConvert( fieldInfo['LowBound'], fieldInfo['type'] ) ;
               }
               if( fieldInfo['UpBound'].toLowerCase() == '$maxkey' )
               {
                  upbound[ fieldName ] = { '$maxKey': 1 } ;
               }
               else
               {
                  upbound[ fieldName ] = specifyTypeConvert( fieldInfo['UpBound'], fieldInfo['type'] ) ;
               }
            }
         } ) ;
         var data = { 'cmd': 'attach collection',
                      'collectionname': mainCL[ value['name'] ]['key'],
                      'subclname': childCL[ value['attachName'] ]['key'],
                      'lowbound': JSON.stringify( lowbound ),
                      'upbound': JSON.stringify( upbound )
                    } ;
         var exec = function(){
            SdbRest.DataOperation( data, function( json ){
               _DataDatabaseIndex.getCSInfo( $scope, SdbRest ) ;
            }, function( errorInfo ){
               _IndexPublic.createRetryModel( $scope, errorInfo, function(){
                  exec() ;
                  return true ;
               } ) ;
            }, function(){
               _IndexPublic.createErrorModel( $scope, $scope.autoLanguage( '网络连接错误，请尝试按F5刷新浏览器。' ) ) ;
            } ) ;
         } ;
         exec() ;
      }
      return isAllClear ;
   }
}

//打开 切分数据 的窗口
_DataDatabaseIndex.showSplit = function( $scope, SdbRest ){
   if( _IndexPublic.checkEditionAndSupport( $scope, 'sequoiadb', 'Metadata' ) == false )
   {
      return ;
   }
   var clValid = [] ;
   var clIndex = -1 ;
   var sourceGroupValid = [] ;
   var groupValid = [] ;
   $.each( $scope.GroupList, function( index, groupInfo ){
      groupValid.push( { 'key': groupInfo['GroupName'], 'value': index } ) ;
   } ) ;
   $.each( $scope.clList, function( index, clInfo ){
      if( $scope.showType == 'cs' )
      {
         if( $scope.csList[ $scope.csID ]['Name'] == clInfo['csName'] && clIndex < 0 )
         {
            if( clInfo['IsMainCL'] != true && ( clInfo['ShardingType'] == 'hash' || clInfo['ShardingType'] == 'range' ) )
            {
               clIndex = index ;
               $.each( $scope.clList[ clIndex ]['GroupName'], function( index2, groupInfo ){
                  sourceGroupValid.push( { 'key': groupInfo['key'], 'value': index2 } ) ;
               } ) ;
            }
         }
      }
      else
      {
         if( index == $scope.clID && clIndex < 0 && clInfo['IsMainCL'] != true && ( clInfo['ShardingType'] == 'hash' || clInfo['ShardingType'] == 'range' ) )
         {
            clIndex = index ;
            $.each( $scope.clList[ clIndex ]['GroupName'], function( index2, groupInfo ){
               sourceGroupValid.push( { 'key': groupInfo['key'], 'value': index2 } ) ;
            } ) ;
         }
      }
      if( clInfo['IsMainCL'] != true && ( clInfo['ShardingType'] == 'hash' || clInfo['ShardingType'] == 'range' ) )
      {
         clValid.push( { 'key' : clInfo['csName'] + '.' + clInfo['Name'] , 'value' : index } ) ;
      }
   } ) ;
   if( clIndex < 0 && clValid.length > 0 )
   {
      clIndex = clValid[0]['value'] ;
      $.each( $scope.clList[ clIndex ]['GroupName'], function( index2, groupInfo ){
         sourceGroupValid.push( { 'key': groupInfo['key'], 'value': index2 } ) ;
      } ) ;
   }
   $scope.Components.Modal.icon = 'fa-scissors' ;
   $scope.Components.Modal.title = $scope.autoLanguage( '切分数据' ) ;
   $scope.Components.Modal.isShow = true ;
   $scope.Components.Modal.formShow = 0 ;
   $scope.Components.Modal.form1 = {
      inputList: [
         {
            "name": "type",
            "webName": $scope.autoLanguage( '切分方式' ),
            "type": "select",
            "value": 0,
            "valid": [
               { 'key': $scope.autoLanguage( '百分比切分' ), 'value': 0 },
               { 'key': $scope.autoLanguage( '条件切分' ), 'value': 1 }
            ],
            "onChange": function( name, key, value ){
               if( value == 0 )
               {
                  $scope.Components.Modal.formShow = 0 ;
               }
               else if( value == 1 )
               {
                  $scope.Components.Modal.formShow = 1 ;
               }
               $scope.Components.Modal.form1.inputList[0]['value'] = 0 ;
               $scope.Components.Modal.form2.inputList[1]['value'] = $scope.Components.Modal.form1.inputList[1]['value'] ;
               $scope.Components.Modal.form2.inputList[2]['valid'] = $scope.Components.Modal.form1.inputList[2]['valid'] ;
               $scope.Components.Modal.form2.inputList[2]['value'] = $scope.Components.Modal.form1.inputList[2]['value'] ;
               $scope.Components.Modal.form2.inputList[3]['value'] = $scope.Components.Modal.form1.inputList[3]['value'] ;
            }
         },
         {
            "name": "name",
            "webName": $scope.autoLanguage( '集合名' ),
            "type": "select",
            "value": clIndex,
            "valid": clValid,
            "onChange": function( name, key, value ){
               $.each( $scope.clList, function( index, clInfo ){
                  if( key == ( clInfo['csName'] + '.' + clInfo['Name'] ) )
                  {
                     sourceGroupValid = [] ;
                     var sourceIndex = -1 ;
                     $.each( $scope.clList[ index ]['GroupName'], function( index2, groupInfo ){
                        if( sourceIndex < 0 )
                        {
                           sourceIndex = index2 ;
                        }
                        sourceGroupValid.push( { 'key': groupInfo['key'], 'value': index2 } ) ;
                     } ) ;
                     $scope.Components.Modal.form1['inputList'][2]['value'] = sourceIndex ;
                     $scope.Components.Modal.form1['inputList'][2]['valid'] = sourceGroupValid ;
                     return false;
                  }
               } ) ;
            }
         },
         {
            "name": "source",
            "webName": $scope.autoLanguage( '源分区组' ),
            "type": "select",
            "value": 0,
            "valid": sourceGroupValid
         },
         {
            "name": "target",
            "webName": $scope.autoLanguage( '目标分区组' ),
            "type": "select",
            "value": 0,
            "valid": groupValid
         },
         {
            "name": "percent",
            "webName": $scope.autoLanguage( '百分比切分' ),
            "type": "double",
            "required": true,
            "value": 50,
            "valid": {
               'min': 1,
               'max': 100
            }
         }
      ]
   } ;
   $scope.Components.Modal.form2 = {
      inputList: [
         {
            "name": "type",
            "webName": $scope.autoLanguage( '切分方式' ),
            "type": "select",
            "value": 1,
            "valid": [
               { 'key': $scope.autoLanguage( '百分比切分' ), 'value': 0 },
               { 'key': $scope.autoLanguage( '条件切分' ), 'value': 1 }
            ],
            "onChange": function( name, key, value ){
               if( value == 0 )
               {
                  $scope.Components.Modal.formShow = 0 ;
               }
               else if( value == 1 )
               {
                  $scope.Components.Modal.formShow = 1 ;
               }
               $scope.Components.Modal.form2.inputList[0]['value'] = 1 ;
               $scope.Components.Modal.form1.inputList[1]['value'] = $scope.Components.Modal.form2.inputList[1]['value'] ;
               $scope.Components.Modal.form1.inputList[2]['valid'] = $scope.Components.Modal.form2.inputList[2]['valid'] ;
               $scope.Components.Modal.form1.inputList[2]['value'] = $scope.Components.Modal.form2.inputList[2]['value'] ;
               $scope.Components.Modal.form1.inputList[3]['value'] = $scope.Components.Modal.form2.inputList[3]['value'] ;
            }
         },
         {
            "name": "name",
            "webName": $scope.autoLanguage( '集合名' ),
            "type": "select",
            "value": clIndex,
            "valid": clValid,
            "onChange": function( name, key, value ){
               $.each( $scope.clList, function( index, clInfo ){
                  if( key == ( clInfo['csName'] + '.' + clInfo['Name'] ) )
                  {
                     var sourceIndex = -1 ;
                     sourceGroupValid = [] ;
                     $.each( $scope.clList[ index ]['GroupName'], function( index2, groupInfo ){
                        if( sourceIndex < 0 )
                        {
                           sourceIndex = index2 ;
                        }
                        sourceGroupValid.push( { 'key': groupInfo['key'], 'value': index2 } ) ;
                     } ) ;
                     $scope.Components.Modal.form2['inputList'][2]['value'] = sourceIndex ;
                     $scope.Components.Modal.form2['inputList'][2]['valid'] = sourceGroupValid ;
                     return false;
                  }
               } ) ;
            }
         },
         {
            "name": "source",
            "webName": $scope.autoLanguage( '源分区组' ),
            "type": "select",
            "value": 0,
            "valid": sourceGroupValid
         },
         {
            "name": "target",
            "webName": $scope.autoLanguage( '目标分区组' ),
            "type": "select",
            "value": 0,
            "valid": groupValid
         },
         {
            "name": "condition",
            "webName":  $scope.autoLanguage( '切分条件' ),
            "required": true,
            "type": "list",
            "desc": $scope.autoLanguage( '起始范围填$minKey为负无穷，结束范围填$maxKey为正无穷。') ,
            "valid": {
               "min": 1
            },
            "child":[
               [
                  {
                     "name": "field",
                     "webName": $scope.autoLanguage( "字段名" ),
                     "placeholder": $scope.autoLanguage( "字段名" ),
                     "type": "string",
                     "value": "",
                     "valid": {
                        "min": 1,
                        "regex": "^[^/$].*",
                        "ban": "."
                     }
                  },
                  {
                     "name": "type",
                     "webName": $scope.autoLanguage( "类型" ),
                     "placeholder": $scope.autoLanguage( "类型" ),
                     "type": "select",
                     "value": "Auto",
                     "valid": [
                        { "key": "Auto",      "value": "Auto" },
                        { "key": "Bool",      "value": "Bool" },
                        { "key": "Number",    "value": "Number" },
                        { "key": "Decimal",   "value": "Decimal" },
                        { "key": "String",    "value": "String" },
                        { "key": "ObjectId",  "value": "ObjectId" },
                        { "key": "Regex",     "value": "Regex" },
                        { "key": "Binary",    "value": "Binary" },
                        { "key": "Timestamp", "value": "Timestamp" },
                        { "key": "Date",      "value": "Date" }
                     ]
                  },
                  {
                     "name": "start",
                     "webName": $scope.autoLanguage( "起始范围" ),
                     "placeholder": $scope.autoLanguage( "起始范围" ),
                     "type": "string",
                     "value": "",
                     "selectList": [ '$minKey' ]
                  },
                  {
                     "name": "end",
                     "webName": $scope.autoLanguage( "结束范围" ),
                     "placeholder": $scope.autoLanguage( "结束范围" ),
                     "type": "string",
                     "value": "",
                     "selectList": [ '$maxKey' ]
                  }
               ]
            ]
         }
      ]
   } ;
   $scope.Components.Modal.Context = '\
<div ng-if="data.formShow == 0" form-create para="data.form1"></div>\
<div ng-if="data.formShow == 1" form-create para="data.form2"></div>' ;
   $scope.Components.Modal.ok = function(){
      var isAllClear = true ;
      if( $scope.Components.Modal.formShow == 0 )
      {
         isAllClear = $scope.Components.Modal.form1.check() ;
      }
      else
      {
         isAllClear = $scope.Components.Modal.form2.check() ;
      }
      if( isAllClear )
      {
         var value = {} ;
         if( $scope.Components.Modal.formShow == 0 )
         {
            value = $scope.Components.Modal.form1.getValue() ;
         }
         else
         {
            value = $scope.Components.Modal.form2.getValue() ;
         }
         var fullname = '' ;
         $.each( clValid, function( index, clValidInfo ){
            if( clValidInfo['value'] == value['name'] )
            {
               fullname = clValidInfo['key'] ;
               return false ;
            }
         } ) ;
         var data = {
            'cmd': 'split',
            'name': fullname,
            'source': sourceGroupValid[ value['source'] ]['key'],
            'target': groupValid[ value['target'] ]['key']
         } ;
         if( value['type'] == 0 )
         {
            //百分比
            data['splitpercent'] = value['percent'] ;
         }
         else
         {
            //条件
            var splitquery = {} ;
            var splitendquery = {} ;
            $.each( value['condition'], function( index, conditionInfo ){
               var fieldName = conditionInfo['field'] ;
               if( conditionInfo['type'] == 'Auto' )
               {
                  splitquery[ fieldName ]    = autoTypeConvert( conditionInfo['start'], true ) ;
                  splitendquery[ fieldName ] = autoTypeConvert( conditionInfo['end'], true ) ;
               }
               else
               {
                  if( conditionInfo['start'].toLowerCase() == '$minkey' )
                  {
                     splitquery[ fieldName ] = { '$minKey': 1 } ;
                  }
                  else
                  {
                     splitquery[ fieldName ]    = specifyTypeConvert( conditionInfo['start'], conditionInfo['type'] ) ;
                  }
                  if( conditionInfo['end'].toLowerCase() == '$maxkey' )
                  {
                     splitendquery[ fieldName ] = { '$maxKey': 1 } ;
                  }
                  else
                  {
                     splitendquery[ fieldName ] = specifyTypeConvert( conditionInfo['end'], conditionInfo['type'] ) ;
                  }
               }
            } ) ;
            data['splitquery'] = JSON.stringify( splitquery ) ;
            data['splitendquery'] = JSON.stringify( splitendquery ) ;
         }
         var exec = function(){
            SdbRest.DataOperation( data, function( json ){
               _DataDatabaseIndex.getCSInfo( $scope, SdbRest ) ;
            }, function( errorInfo ){
               _IndexPublic.createRetryModel( $scope, errorInfo, function(){
                  exec() ;
                  return true ;
               } ) ;
            }, function(){
               _IndexPublic.createErrorModel( $scope, $scope.autoLanguage( '网络连接错误，请尝试按F5刷新浏览器。' ) ) ;
            } ) ;
         } ;
         exec() ;
      }
      return isAllClear ;
   }
}

//显示不同分区组下的CS或CL信息
_DataDatabaseIndex.showGroupInfo = function( $scope, index ){
   $scope.selectGroup = index ;
   if( $scope.showType == 'cs' )
   {
      if( index == null )
      {
         $scope.attr['Info'] = $scope.csList[ $scope.csID ]['Info'] ;
      }
      else
      {
         $scope.attr['Info'] = $scope.csInfo[ index ] ;
      }
   }
   else
   {
      if( index == null )
      {
         $scope.attr['Info'] = $scope.clList[ $scope.clID ]['Info'] ;
      }
      else
      {
         $scope.attr['Info'] = $scope.clInfo[ index ] ;
      }
   }
}

//打开 创建索引 的窗口
_DataDatabaseIndex.showCreateIndex = function( $scope, SdbRest ){
   if( _IndexPublic.checkEditionAndSupport( $scope, 'sequoiadb', 'Metadata' ) == false )
   {
      return ;
   }
   var clValid = [] ;
   $.each( $scope.clList, function( index, clInfo ){
      clValid.push( { 'key': clInfo['csName'] + '.' + clInfo['Name'], 'value': index } ) ;
   } ) ;
   $scope.Components.Modal.icon = 'fa-plus' ;
   $scope.Components.Modal.title = $scope.autoLanguage( '创建索引' ) ;
   $scope.Components.Modal.isShow = true ;
   $scope.Components.Modal.form = {
      inputList: [
         _operate.selectCL( $scope, clValid ),
         _operate.indexName( $scope ),
         _operate.indexDef( $scope ),
         _operate.unique( $scope ),
         _operate.enforced( $scope ),
         _operate.sortbuffersize( $scope )
      ]
   } ;
   $scope.Components.Modal.Context = '<div form-create para="data.form"></div>' ;
   $scope.Components.Modal.ok = function(){
      var isAllClear = $scope.Components.Modal.form.check() ;
      if( isAllClear )
      {
         function modalValue2CreateIndex( valueJson )
         {
            var indexdef = parseIndexDefValue( valueJson['indexKey'] ) ;
            //组装
            var returnJson = {} ;
            returnJson['collectionname'] = clValid[ valueJson['clName'] ]['key'] ;
            returnJson['indexname'] = valueJson['indexName'] ;
            returnJson['indexdef'] = JSON.stringify( indexdef ) ;
            returnJson['unique'] = valueJson['isUnique'] ;
            returnJson['enforced'] = valueJson['enforced'] ;
            returnJson['sortbuffersize'] = valueJson['sortbuffersize'] ;
            return returnJson ;
         }
         var value = $scope.Components.Modal.form.getValue() ;
         var data = modalValue2CreateIndex( value ) ;
         data['cmd'] = 'create index' ;
         var exec = function(){
            SdbRest.DataOperation( data, function( json ){
               _DataDatabaseIndex.getCLInfo( $scope, SdbRest ) ;
            }, function( errorInfo ){
               _IndexPublic.createRetryModel( $scope, errorInfo, function(){
                  exec() ;
                  return true ;
               } ) ;
            }, function(){
               _IndexPublic.createErrorModel( $scope, $scope.autoLanguage( '网络连接错误，请尝试按F5刷新浏览器。' ) ) ;
            } ) ;
         } ;
         exec() ;
      }
      return isAllClear ;
   }
}

//打开 删除索引 的窗口
_DataDatabaseIndex.showRemoveIndex = function( $scope, SdbRest ){
   if( _IndexPublic.checkEditionAndSupport( $scope, 'sequoiadb', 'Metadata' ) == false )
   {
      return ;
   }
   var clValid = [] ;
   var indexValid = [] ; 
   var fullName = '' ;
   var indexesInfoList = [] ;
   $.each( $scope.clList, function( index, clInfo ){
      clValid.push( { 'key': clInfo['csName'] + '.' + clInfo['Name'], 'value': index } ) ;
      if( $scope.clID == index )
      {
         fullName = clInfo['csName'] + '.' + clInfo['Name'] ;
      }
   } ) ;

   $scope.Components.Modal.indexList = {} ;
   $scope.Components.Modal.icon = 'fa-trash-o' ;
   $scope.Components.Modal.title =  $scope.autoLanguage( '删除索引' ) ;
   $scope.Components.Modal.ok = function(){
      return false ;
   }    
   var data = { 'cmd': 'list indexes', 'collectionname': fullName } ;
   SdbRest.DataOperation( data, function( indexList ){
      indexesInfoList = indexList ;
      $.each( indexList, function( index, indexInfo ){
         indexValid.push( { 'key': indexInfo['IndexDef']['name'], 'value': index } ) ;
      } ) ;
      $scope.Components.Modal.form = {
         inputList: [
            {
               "name": "clName",
               "webName": $scope.autoLanguage( '集合' ),
               "type": "select",
               "value": $scope.clID,
               "valid": clValid,
               "onChange": function( name, key, value ){
                  var data = { 'cmd': 'list indexes', 'collectionname': key } ;
                  SdbRest.DataOperation( data, function( indexList ){
                     indexesInfoList = indexList ;
                     $scope.Components.Modal.indexList = indexesInfoList[0] ;
                     indexValid = [] ;
                     $.each( indexList, function( index, indexInfo ){
                        indexValid.push( { 'key': indexInfo['IndexDef']['name'], 'value': index } ) ;
                     } ) ;
                     $scope.Components.Modal.form['inputList'][1]['value'] = 0 ;
                     $scope.Components.Modal.form['inputList'][1]['valid'] = indexValid ;
                  }, function( errorInfo ){
                     _IndexPublic.createRetryModel( $scope, errorInfo, function(){
                        exec() ;
                        return true ;
                     } ) ;
                  }, function(){
                     _IndexPublic.createErrorModel( $scope, $scope.autoLanguage( '网络连接错误，请尝试按F5刷新浏览器。' ) ) ;
                  } ) ;
               }
            },
            {
               "name": "indexName",
               "webName":  $scope.autoLanguage( '索引名' ),
               "type": "select",
               "value": 0,
               "valid": indexValid,
               "onChange": function( name, key, value ){
                  $scope.Components.Modal.indexList = indexesInfoList[ value ] ;
               }
            }
         ]
      } ;
      $scope.Components.Modal.indexList = indexesInfoList[0] ;
      $scope.Components.Modal.Context = '\
<div form-create para="data.form"></div>\
<table class="table loosen border" ng-if="data.indexList">\
<tr>\
<td style="width:40%;background-color:#F1F4F5;"><b>Key</b></td>\
<td style="width:60%;background-color:#F1F4F5;"><b>Value</b></td>\
</tr>\
<tr>\
<td>Name</td>\
<td>{{data.indexList.IndexDef.name}}</td>\
</tr>\
<tr ng-repeat="(key, value) in data.indexList.IndexDef track by $index" ng-if="key != \'name\'&&key != \'_id\'">\
<td>{{key}}</td>\
<td>{{value}}</td>\
</tr>\
<tr>\
<td>IndexFlag</td>\
<td>{{data.indexList.IndexFlag}}</td>\
</tr>\
</table>' ;
      $scope.Components.Modal.isShow = true ;
      $scope.Components.Modal.ok = function(){
         var isAllClear = $scope.Components.Modal.form.check() ;
         if( isAllClear )
         {
            var value = $scope.Components.Modal.form.getValue() ;
            var data = { 'cmd': 'drop index' } ;
            data['collectionname'] = clValid[ value['clName'] ]['key'] ;
            data['indexname'] = indexValid[ value['indexName'] ]['key'] ;
            var exec = function(){
               SdbRest.DataOperation( data, function( json ){
                  _DataDatabaseIndex.getCLInfo( $scope, SdbRest ) ;
               }, function( errorInfo ){
                  _IndexPublic.createRetryModel( $scope, errorInfo, function(){
                     exec() ;
                     return true ;
                  } ) ;
               }, function(){
                  _IndexPublic.createErrorModel( $scope, $scope.autoLanguage( '网络连接错误，请尝试按F5刷新浏览器。' ) ) ;
               } ) ;
            } ;
            exec() ;
         }
         return isAllClear ;
      }   
      $scope.$apply() ;
   }, function( errorInfo ){
      _IndexPublic.createRetryModel( $scope, errorInfo, function(){
         exec() ;
         return true ;
      } ) ;
   }, function(){
      _IndexPublic.createErrorModel( $scope, $scope.autoLanguage( '网络连接错误，请尝试按F5刷新浏览器。' ) ) ;
   } ) ;
}

//打开 索引详细 的窗口
_DataDatabaseIndex.showIndex = function( $scope, SdbRest, csIndex, clIndex ){
   var fullName = $scope.clList[clIndex]['csName'] + '.' + $scope.clList[clIndex]['Name'] ;
   var data = { 'cmd': 'list indexes', 'collectionname': fullName } ;
   SdbRest.DataOperation( data, function( indexList ){
      var indexName = [] ; 
      var indexContent = [] ;
      $.each( indexList, function( index, indexInfo ){
         indexName.push( { 'key': indexInfo['IndexDef']['name'] , 'value': index } ) ;
         indexContent.push( indexInfo ) ;
      } ) ;
      $scope.Components.Modal.icon = '' ;
      $scope.Components.Modal.title = $scope.autoLanguage( '索引信息' ) ;
      $scope.Components.Modal.noOK = true ;
      $scope.Components.Modal.isShow = true ;
      $scope.Components.Modal.form = {
         inputList: [
            {
               "name": "index",
               "webName": $scope.autoLanguage( "索引名" ),
               "type": "select",
               "value":indexName[0]['value'] ,
               "valid": indexName,
               "onChange": function( name, key, value ){
                  $scope.Components.Modal.indexList = indexContent[ value ] ;
               }
            }
         ]
      } ;
      $scope.Components.Modal.indexList = indexContent[0] ;
      $scope.Components.Modal.Context = '\
<div form-create para="data.form"></div>\
<table class="table loosen border">\
<tr>\
<td style="width:40%;background-color:#F1F4F5;"><b>Key</b></td>\
<td style="width:60%;background-color:#F1F4F5;"><b>Value</b></td>\
</tr>\
<tr>\
<td>Name</td>\
<td>{{data.indexList.IndexDef.name}}</td>\
</tr>\
<tr ng-repeat="(key, value) in data.indexList.IndexDef track by $index" ng-if="key != \'name\'&&key != \'_id\'">\
<td>{{key}}</td>\
<td>{{value}}</td>\
</tr>\
<tr>\
<td>IndexFlag</td>\
<td>{{data.indexList.IndexFlag}}</td>\
</tr>\
</table>' ;
   }, function( errorInfo ){
      _IndexPublic.createRetryModel( $scope, errorInfo, function(){
         exec() ;
         return true ;
      }, $scope.autoLanguage( '获取索引信息失败' ) ) ;
   }, function(){
      _IndexPublic.createErrorModel( $scope, $scope.autoLanguage( '网络连接错误，请尝试按F5刷新浏览器。' ) ) ;
   } ) ;
}

//打开 分区信息 的窗口
_DataDatabaseIndex.showPartitions = function( $scope ){
   $scope.Components.Modal.icon = '' ;
   $scope.Components.Modal.title = $scope.autoLanguage( '分区信息' ) ;
   $scope.Components.Modal.noOK = true ;
   $scope.Components.Modal.isShow = true ;
   var partitionInfo = [] ;
   if( $scope.clList[ $scope.clID ]['IsMainCL'] == true )
   {
      $.each( $scope.clList[ $scope.clID ]['Info']['CataInfo'], function( index, info ){
         partitionInfo.push( {
            'name'     : info['SubCLName'],
            'LowBound' : info['LowBound'],
            'UpBound'  : info['UpBound']
         } ) ;
      } ) ;
      $scope.Components.Modal.partitionInfo = partitionInfo ;
      $scope.Components.Modal.Context = '\
<table class="table loosen border">\
   <tr>\
      <td style="width:30%;background-color:#F1F4F5;"><b>Partition</b></td>\
      <td style="width:35%;background-color:#F1F4F5;"><b>LowBound</b></td>\
      <td style="width:35%;background-color:#F1F4F5;"><b>UpBound</b></td>\
   </tr>\
   <tr ng-repeat="(key, value) in data.partitionInfo track by $index">\
      <td>{{value["name"]}}</td>\
      <td>{{value["LowBound"]}}</td>\
      <td>{{value["UpBound"]}}</td>\
   </tr>\
</table>' ;
   }
   else
   {
      if( $scope.selectGroup == null )
      {
         $.each( $scope.clList[ $scope.clID ]['GroupName'], function( index, groupInfo ){
            $.each( $scope.clInfo[ groupInfo['value'] ]['LowBound'], function( index2 ){
               partitionInfo.push( {
                  'GroupName': $scope.clInfo[ groupInfo['value'] ]['GroupName'],
                  'LowBound' : $scope.clInfo[ groupInfo['value'] ]['LowBound'][index2],
                  'UpBound'  : $scope.clInfo[ groupInfo['value'] ]['UpBound'][index2]
               } ) ;
            } ) ;
         } ) ;
      }
      else
      {
         $.each( $scope.clInfo[ $scope.selectGroup ]['LowBound'], function( index2 ){
            partitionInfo.push( {
               'GroupName': $scope.clInfo[ $scope.selectGroup ]['GroupName'],
               'LowBound' : $scope.clInfo[ $scope.selectGroup ]['LowBound'][index2],
               'UpBound'  : $scope.clInfo[ $scope.selectGroup ]['UpBound'][index2]
            } ) ;
         } ) ;
      }
      $scope.Components.Modal.partitionInfo = partitionInfo ;
      $scope.Components.Modal.Context = '\
<table class="table loosen border">\
   <tr>\
      <td style="width:30%;background-color:#F1F4F5;"><b>Group</b></td>\
      <td style="width:35%;background-color:#F1F4F5;"><b>LowBound</b></td>\
      <td style="width:35%;background-color:#F1F4F5;"><b>UpBound</b></td>\
   </tr>\
   <tr ng-repeat="(key, value) in data.partitionInfo track by $index">\
      <td>{{value["GroupName"]}}</td>\
      <td>{{value["LowBound"]}}</td>\
      <td>{{value["UpBound"]}}</td>\
   </tr>\
</table>' ;
   }
}