/******************************************************************************
 * @Description   : seqDB-23648 stp.getSyncHistory()接口验证 
 * @Author        : Siqin Chen
 * @CreateTime    : 2021.03.22
 * @LastEditTime  : 2021.03.22
 * @LastEditors   : Siqin Chen
 ******************************************************************************/
testConf.skipStandAlone = true;
main( test );
function test ()
{
   //1.连接server主节点执行stp.getSyncHistory()查询,自动化不实现切主的情况，清空历史耗时太长，不合适
   var primaryNode = getStpPrimaryNode();
   var primaryStp = new Stp(primaryNode["HostName"], primaryNode["Service"]);
   var primarySyncHistory = primaryStp.getSyncHistory();
   checkSyncHistoryInfo(primarySyncHistory, true);
   
   //2.连接server非主节点执行stp.getSyncHistory()查询
   var spareNode = getStpSpareNode();
   var spareNodeHost = spareNode[0]["HostName"];
   var spareNodePort = spareNode[0]["Service"];
   var spareStp = new Stp(spareNodeHost, spareNodePort);
   var spareSyncHistory = spareStp.getSyncHistory();
   checkSyncHistoryInfo(spareSyncHistory, false);
}

function checkSyncHistoryInfo(info, isPrimary)
{
   var isSuccess = true;
   var primaryNode = getStpPrimaryNode();
   var syncHistory = JSON.parse( info.toString() );
   if(isPrimary)
   {
      if(syncHistory["SyncSources"].length != 0)
      {
         isSuccess = false;
      }
   } else {
      if(syncHistory["SyncSources"][0]["HostName"] != primaryNode["HostName"] || syncHistory["SyncSources"][0]["Service"] != primaryNode["Service"])
      {
         isSuccess = false;
      }
   }

   if (!isSuccess)
   {
      throw new Error( "Error: stp.getSyncHistory return syncHistory is not expected! " + syncHistory["SyncSources"][0]["HostName"]);
   }
}