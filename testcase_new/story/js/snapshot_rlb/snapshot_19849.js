/******************************************************************************
*@Description : seqDB-19849 : 指定showError和showErrorMode查询节点健康快照 
*@author      : Zhao xiaoni
*@Date        : 2019-09-29
******************************************************************************/
main( test );

function test ()
{
   if( commIsStandalone( db ) )
   {
      return;
   }

   var nodeAddresses = getNodeAddresses();
   stopNodes( nodeAddresses );

   try
   {
      //show/aggr显示节点错误信息
      var count = 0;
      var showError = "show";
      var showErrorMode = "aggr";
      var sdbsnapshotOption = new SdbSnapshotOption().options( { ShowError: showError, ShowErrorMode: showErrorMode } );
      for( var i = 0; i < nodeAddresses.length; i++ )
      {
         var cursor = db.snapshot( SDB_SNAP_HEALTH, sdbsnapshotOption );
         var errNodes = cursor.current().toObj()["ErrNodes"];
         var hostName = nodeAddresses[i]["hostName"];
         var svcName = nodeAddresses[i]["svcName"];
         for( var j = 0; j < errNodes.length; j++ )
         {
            var nodeName = errNodes[j]["NodeName"];
            var flag = errNodes[j]["Flag"];
            if( nodeName === hostName + ":" + svcName )
            {
               if( flag !== -79 )
               {
                  throw new Error( "show/aggr's nodeName is " + nodeName );
               }
               count++;
               break;
            }
         }
      }
      if( count !== nodeAddresses.length )
      {
         throw new Error( "show/aggr's count is " + count );
      }

      //show/flat显示节点错误信息
      count = 0;
      showError = "show";
      showErrorMode = "flat";
      sdbsnapshotOption = new SdbSnapshotOption().options( { ShowError: showError, ShowErrorMode: showErrorMode } );
      for( var i = 0; i < nodeAddresses.length; i++ )
      {
         cursor = db.snapshot( SDB_SNAP_HEALTH, sdbsnapshotOption );
         var hostName = nodeAddresses[i]["hostName"];
         var svcName = nodeAddresses[i]["svcName"];
         while( cursor.next() )
         {
            var nodeName = cursor.current().toObj()["NodeName"];
            var flag = cursor.current().toObj()["Flag"];
            if( nodeName === hostName + ":" + svcName )
            {
               if( flag !== -79 )
               {
                  throw new Error( "nodeName is " + nodeName + ", flag is " + flag );
               }
               count++;
               break;
            }
         }
      }
      if( count !== nodeAddresses.length )
      {
         throw new Error( "show/flat's count is " + count );
      }

      //ignore/[flat, aggr]显示节点错误信息
      showError = "ignore";
      showErrorMode = ["flat", "aggr"];
      for( var i = 0; i < showErrorMode.length; i++ )
      {
         sdbsnapshotOption = new SdbSnapshotOption().options( { ShowError: showError, ShowErrorMode: showErrorMode[i] } );
         cursor = db.snapshot( SDB_SNAP_HEALTH, sdbsnapshotOption );
         errNodes = cursor.current().toObj()["ErrNodes"];
         if( errNodes !== undefined )
         {
            throw new Error( "ignore/aggr's errNodes is " + JSON.stringify( errNodes ) );
         }
      }

      //only/aggr显示节点错误信息
      count = 0;
      var num = 0;
      showError = "only";
      showErrorMode = "aggr";
      sdbsnapshotOption = new SdbSnapshotOption().options( { ShowError: showError, ShowErrorMode: showErrorMode } );
      cursor = db.snapshot( SDB_SNAP_HEALTH, sdbsnapshotOption );
      while( cursor.next() )
      {
         num++;
         errNodes = cursor.current().toObj()["ErrNodes"];
         for( var i = 0; i < nodeAddresses.length; i++ )
         {
            var hostName = nodeAddresses[i]["hostName"];
            var svcName = nodeAddresses[i]["svcName"];
            for( var j = 0; j < errNodes.length; j++ )
            {
               var nodeName = errNodes[j]["NodeName"];
               var flag = errNodes[j]["Flag"];
               if( nodeName === hostName + ":" + svcName )
               {
                  if( flag !== -79 )
                  {
                     throw new Error( "show/aggr's nodeName is " + nodeName );
                  }
                  count++;
                  break;
               }
            }
         }
      }
      if( num !== 1 || count !== nodeAddresses.length )
      {
         throw new Error( "only/aggr's num is " + num + ", count is " + count );
      }

      //only/flat显示节点错误信息
      count = 0;
      showError = "only";
      showErrorMode = "flat";
      sdbsnapshotOption = new SdbSnapshotOption().options( { ShowError: showError, ShowErrorMode: showErrorMode } );
      cursor = db.snapshot( SDB_SNAP_HEALTH, sdbsnapshotOption );
      while( cursor.next() )
      {
         var nodeName = cursor.current().toObj()["NodeName"];
         var flag = cursor.current().toObj()["Flag"];
         for( var i = 0; i < nodeAddresses.length; i++ )
         {
            var hostName = nodeAddresses[i]["hostName"];
            var svcName = nodeAddresses[i]["svcName"];
            if( nodeName === hostName + ":" + svcName )
            {
               if( flag !== -79 )
               {
                  throw new Error( "nodeName is " + nodeName + ", flag is " + flag );
               }
               count++;
            }
         }
      }
      if( count !== nodeAddresses.length )
      {
         throw new Error( "only/flat's count is " + count );
      }
   }
   finally
   {
      startNodes( nodeAddresses );
   }
}
