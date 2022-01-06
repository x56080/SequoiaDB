/******************************************************************************
 * @Description   : seqDB-23646 stp.testGetSyncClients23646()接口验证 
 * @Author        : Siqin Chen
 * @CreateTime    : 2021.03.22
 * @LastEditTime  : 2021.03.22
 * @LastEditors   : Siqin Chen
 ******************************************************************************/
testConf.skipStandAlone = true;

main( test );
function test ()
{
   //1.连接server主节点执行stp.getSyncClients()查询，检查STP 节点所在 STP 集群的时间同步信息 
   var primaryNode = getStpPrimaryNode();
   var primaryStp = new Stp(primaryNode["HostName"], primaryNode["Service"]);
   var primarySyncClients = primaryStp.getSyncClients();
   checkSyncClientsInfo(primarySyncClients, primaryNode);
   
   //2.连接server非主节点执行stp.getSyncClients()查询，检查STP 节点所在 STP 集群的时间同步信息 
   var spareNode = getStpSpareNode();
   var spareNodeHost = spareNode[0]["HostName"];
   var spareNodePort = spareNode[0]["Service"];
   var spareStp = new Stp(spareNodeHost, spareNodePort);
   var spareSyncClients = spareStp.getSyncClients();
   checkSyncClientsInfo(spareSyncClients, primaryNode);
   
   //3.连接client执行stp.getSyncClients()查询，检查STP 节点所在 STP 集群的时间同步信息
   var clientNode = getStpClientNode();
   var clientHost = clientNode["HostName"];
   var clientPort = clientNode["Service"];
   var clientStp = new Stp(clientHost, clientPort);
   var clientSyncClients = clientStp.getSyncClients();
   checkSyncClientsInfo(clientSyncClients, primaryNode);
}

function checkSyncClientsInfo(info, primaryNode)
{
   var isSuccess = true;
   var syncClients = JSON.parse( info.toString() );
   if(syncClients["SyncSource"]["HostName"] != primaryNode["HostName"] || syncClients["SyncSource"]["Service"] != primaryNode["Service"])
   {
      isSuccess = false;
   }
   if (syncClients["SyncClients"].length <= 0)
   {
      isSuccess = false;
   }
   if (!isSuccess)
   {
      throw new Error( "Error: stp.getSyncClients return SyncClients is not expected: " + syncClients );
   }
}