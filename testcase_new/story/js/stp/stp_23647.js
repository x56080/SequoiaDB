/******************************************************************************
 * @Description   : seqDB-23647 stp.getSyncStatus()接口验证 
 * @Author        : Siqin Chen
 * @CreateTime    : 2021.03.22
 * @LastEditTime  : 2021.03.22
 * @LastEditors   : Siqin Chen
 ******************************************************************************/
testConf.skipStandAlone = true;
main( test );
function test ()
{
   //1.连接server主节点执行stp.getSyncStatus()查询，检查STP 节点历史同步请求历史信息和当前同步源的同步信息
   var primaryNode = getStpPrimaryNode();
   var primaryStp = new Stp(primaryNode["HostName"], primaryNode["Service"]);
   var primarySyncStatus = primaryStp.getSyncStatus();
   checkSyncStatusInfo(primarySyncStatus, true);
   
   //2.连接server非主节点执行stp.getSyncStatus()查询，检查STP 节点历史同步请求历史信息和当前同步源的同步信息
   var spareNode = getStpSpareNode();
   var spareNodeHost = spareNode[0]["HostName"];
   var spareNodePort = spareNode[0]["Service"];
   var spareStp = new Stp(spareNodeHost, spareNodePort);
   var spareSyncStatus = spareStp.getSyncStatus();
   checkSyncStatusInfo(spareSyncStatus,false);
   
   //3.连接client执行stp.getSyncStatus()查询，检查STP 节点历史同步请求历史信息和当前同步源的同步信息
   var clientNode = getStpClientNode();
   var clientHost = clientNode["HostName"];
   var clientPort = clientNode["Service"];
   var clientStp = new Stp(clientHost, clientPort);
   var clientSyncStatus = clientStp.getSyncStatus();
   checkSyncStatusInfo(clientSyncStatus, false);
}

function checkSyncStatusInfo(info, isPrimary)
{
   var isSuccess = true;
   var syncStatus = JSON.parse( info.toString() );
   if(isPrimary)
   {
      if(syncStatus["Role"] != "server" || syncStatus["IsPrimary"] != true || syncStatus["SyncSource"] != undefined)
      {
         isSuccess = false;
      }
   } else {
      var primaryNode = getStpPrimaryNode();
      if(syncStatus["SyncSource"]["HostName"] != primaryNode["HostName"] || syncStatus["SyncSource"]["Service"] != primaryNode["Service"])
      {
        isSuccess = false;
      }
      if (syncStatus["SyncSource"]["SyncHistory"].length <= 0)
      {
        isSuccess = false;
      }
   }
   println(syncStatus);
   if (!isSuccess)
   {
      throw new Error( "Error: stp.getSyncStatus return syncStatus is not expected: " + syncStatus );
   }
}