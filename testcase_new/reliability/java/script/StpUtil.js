/******************************************************************************
 * @Description   : stp公共方法
 * @Author        : Siqin Chen
 * @CreateTime    : 2021.03.20
 * @LastEditTime  : 2021.03.20
 * @LastEditors   : Siqin Chen
 ******************************************************************************/

if( typeof ( STPHOSTNAME ) == "undefined" ) { STPHOSTNAME = 'localhost'; }
//STP服务端端口号，默认传入9622
if( typeof ( STPSVCNAME ) == "undefined" ) { STPSVCNAME = '9622'; }

if( typeof ( FUNCTION ) == "undefined" ) { FUNCTION = 'getStpPrimaryNode'; }

main();

function main()
{
   if (FUNCTION == "getStpPrimaryNode")
   {
      getStpPrimaryNode();
   }
   if (FUNCTION == "getStpSpareNode")
   {
      getStpSpareNode();
   }
   if (FUNCTION == "getStpClientNode")
   {
      getStpClientNode();
   }
   if (FUNCTION == "getStpServerNodes")
   {
      getStpServerNodes();
   }
   if (FUNCTION == "getStpTimeUsAndTimeError")
   {
      getStpTimeUsAndTimeError();
   }
}
//server小于2时跳过用例

//获取stp主节点
function getStpPrimaryNode()
{
   var stp = new Stp(STPHOSTNAME, STPSVCNAME);
   var serverGroup = stp.getServers();
   
   var serverInfo = JSON.parse( serverGroup.toString() );
   println(serverInfo["PrimaryNode"]["HostName"]+":"+serverInfo["PrimaryNode"]["Service"]);
}
//stp.getServers()
//获取stp备节点
function getStpSpareNode()
{
   var stp = new Stp(STPHOSTNAME, STPSVCNAME);
   var serverGroup = stp.getServers();
   
   var serverInfo = JSON.parse( serverGroup.toString() );
   
   var spareNode = "";
   for(var i = 0; i < serverInfo["Group"].length; i++) 
   {
      if (serverInfo["Group"][i]["HostName"] != serverInfo["PrimaryNode"]["HostName"])
      {
         if (i==0)
         {
            spareNode=serverInfo["Group"][i]["HostName"]+":"+serverInfo["Group"][i]["Service"];
         }else{
           spareNode=spareNode+","+serverInfo["Group"][i]["HostName"]+":"+serverInfo["Group"][i]["Service"];
         }
      }
   }
   println(spareNode);
}
//获取stp client节点
function getStpClientNode()
{
   var stp = new Stp(STPHOSTNAME, STPSVCNAME);
   var syncClients = stp.getSyncClients();
   var stpClientInfo = JSON.parse( syncClients.toString() );
   var client = "";
   for(var i = 0; i < stpClientInfo["SyncClients"].length; i++) 
   {
      if(stpClientInfo["SyncClients"][i]["Role"] == "client")
      {
         client=stpClientInfo["SyncClients"][i]["HostName"]+":"+stpClientInfo["SyncClients"][i]["Service"];
      }
   }
    println(client);
}
//stp.getServers()

function getStpServerNodes()
{
   var stp = new Stp(STPHOSTNAME, STPSVCNAME);
   var serverGroup = stp.getServers();
   
   var serverInfo = JSON.parse( serverGroup.toString() );
   var servers = [];
   for(var i=0; i<serverInfo["Group"].length; i++)
   {
      servers.push(serverInfo["Group"][i]["HostName"]+":"+serverInfo["Group"][i]["Service"]);
   }
   return servers;
}

function getStpTimeUsAndTimeError()
{
   var stp = new Stp(STPHOSTNAME, STPSVCNAME);
   var timeUs = JSON.parse( stp.getTimeUS().toString() );
   println(timeUs["TimeStamp"]+","+timeUs["TimeError"]);
}

