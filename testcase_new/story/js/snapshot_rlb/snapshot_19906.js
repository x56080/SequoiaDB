/******************************************************************************
*@Description : seqDB-19906 : 指定showError和showErrorMode查询数据库快照 
*@author      : Zhao xiaoni
*@Date        : 2019-10-15
******************************************************************************/
try
{
   main() ;
}
catch(e)
{
   if(e.constructor === Error)
   {
      println(e.stack);
   }
   throw e;
}

function main()
{
   if( commIsStandalone( db ) )
   {
      println("The environment is standalone!");
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
      var snapshotOption = "/*+use_option(ShowError, " + showError + ") use_option(ShowErrorMode, " + showErrorMode + ")*/";
      for(var j = 0; j < nodeAddresses.length; j++)
      {
         var cursor = db.exec("select * from $SNAPSHOT_DB " + snapshotOption);
         var errNodes = cursor.current().toObj()["ErrNodes"];
         var hostName = nodeAddresses[j]["hostName"];
         var svcName = nodeAddresses[j]["svcName"];
         for(var k = 0; k < errNodes.length; k++)
         {
            var nodeName = errNodes[k]["NodeName"];
            var flag = errNodes[k]["Flag"];
            if(nodeName === hostName+":"+svcName)
            {
               if(flag !== -79)
               {
                  throw new Error("show/aggr's nodeName is " + nodeName);
               }
               count++;
               break;
            }
         } 
      }
      if(count !== nodeAddresses.length)
      {
         throw new Error("show/aggr's count is " + count);
      }

      //show/flat显示节点错误信息
      count = 0;
      showError = "show";
      showErrorMode = "flat";
      snapshotOption = "/*+use_option(ShowError, " + showError + ") use_option(ShowErrorMode, " + showErrorMode + ")*/";
      for(var i = 0; i < nodeAddresses.length; i++)
      {
         cursor = db.exec("select * from $SNAPSHOT_DB " + snapshotOption);
         var hostName = nodeAddresses[i]["hostName"];
         var svcName = nodeAddresses[i]["svcName"];
         while(cursor.next())
         {
            var nodeName = cursor.current().toObj()["NodeName"];
            var flag = cursor.current().toObj()["Flag"];
            if(nodeName === hostName+":"+svcName)
            {
               if(flag !== -79)
               {
                  throw new Error("nodeName is " + nodeName + ", flag is " + flag);
               }
               count++;
               break;
            }
         }
      }
      if(count !== nodeAddresses.length)
      {
         throw new Error("show/flat's count is " + count);
      }
   
      //ignore/[flat, aggr]显示节点错误信息
      showError = "ignore";
      showErrorMode = ["flat", "aggr"];
      for(var i = 0; i < showErrorMode.length; i++)
      {
         var snapshotOption = "/*+use_option(ShowError, " + showError + ") use_option(ShowErrorMode, " + showErrorMode[i] + ")*/";
         var cursor = db.exec("select * from $SNAPSHOT_DB " + snapshotOption);
         errNodes = cursor.current().toObj()["ErrNodes"];
         if(errNodes !== undefined)
         {
            throw new Error("ignore/aggr's errNodes is " + JSON.stringify(errNodes));
         }
      }
   
      //only/aggr显示节点错误信息
      count = 0;
      showError = "only";
      showErrorMode = "aggr";
      var snapshotOption = "/*+use_option(ShowError, " + showError + ") use_option(ShowErrorMode, " + showErrorMode + ")*/";
      var cursor = db.exec("select * from $SNAPSHOT_DB " + snapshotOption);
      while(cursor.next())
      {
         errNodes = cursor.current().toObj()["ErrNodes"];
         for(var i = 0; i < nodeAddresses.length; i++)
         {
            var hostName = nodeAddresses[i]["hostName"];
            var svcName = nodeAddresses[i]["svcName"];
            for(var j = 0; j < errNodes.length; j++)
            {
               var nodeName = errNodes[j]["NodeName"];
               var flag = errNodes[j]["Flag"];
               if(nodeName === hostName+":"+svcName)
               {
                  if(flag !== -79)
                  {
                     throw new Error("show/aggr's nodeName is " + nodeName);
                  }
                  count++;
                  break;
               }
            } 
         }
      }
      if(count !== nodeAddresses.length)
      {
         throw new Error("only/aggr's count is " + count);
      }
    
     //only/flat显示节点错误信息
      count = 0;
      showError = "only";
      showErrorMode = "flat";
      snapshotOption = "/*+use_option(ShowError, " + showError + ") use_option(ShowErrorMode, " + showErrorMode + ")*/";
      cursor = db.exec("select * from $SNAPSHOT_DB " + snapshotOption);
      while(cursor.next())
      {
         var nodeName = cursor.current().toObj()["NodeName"];
         var flag = cursor.current().toObj()["Flag"];
         for(var i = 0; i < nodeAddresses.length; i++)
         {
            var hostName = nodeAddresses[i]["hostName"];
            var svcName = nodeAddresses[i]["svcName"];
            if(nodeName === hostName+":"+svcName)
            {
               if(flag !== -79)
               {
                  throw new Error("nodeName is " + nodeName + ", flag is " + flag);
               }
               count++;
            }
         }
      }
      if(count !== nodeAddresses.length)
      {
         throw new Error("only/flat's count is " + count);
      }

   }
   finally
   {
      startNodes( nodeAddresses );
   }
}
