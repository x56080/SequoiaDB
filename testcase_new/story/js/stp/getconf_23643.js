/******************************************************************************
 * @Description   : seqDB-23643 stp.getConf()接口验证 
 * @Author        : Siqin Chen
 * @CreateTime    : 2021.03.22
 * @LastEditTime  : 2021.03.22
 * @LastEditors   : Siqin Chen
 ******************************************************************************/
testConf.skipStandAlone = true;
main( test );
function test ()
{
   testGetConf23643();
}

function testGetConf23643()
{
   //1.分别在server/client上执行stp.getConf()获取当前stp节点配置信息
   var primaryNode = getStpPrimaryNode(STPHOSTNAME,STPSVCNAME);
   var clientNode = getStpClientNode(STPHOSTNAME,STPSVCNAME);
   var clientHost = clientNode["HostName"];
   var clientPort = clientNode["Service"];
   
   var primaryStp = new Stp(primaryNode["HostName"], primaryNode["Service"]);
   var clientStp = new Stp(clientHost, clientPort);
   
   var primaryConf = primaryStp.getConf();
   var rc = checkeConfInfo(JSON.parse( primaryConf.toString()), "server");
   if ( !rc )
   {
      throw new Error( "Error: stp.getConf in server return conf is not expected!" );
   }
   
   var clientConf = clientStp.getConf();
   rc = checkeConfInfo(JSON.parse( clientConf.toString()), "client");
   if ( !rc )
   {
      throw new Error( "Error: stp.getConf in client return conf is not expected!" );
   }
   //2.分别在server/client上修改stp节点所有配置项（port除外）该测试点在updaconf接口中验证
   //primaryStp.updateConf(serverlist:stpConf["serverlist"]+","+clientNode["HostName"]+":"+clientNode["Service"],role:"client",syncinterval:50,maxtimeerror:60000,diaglevel:2);
   //clientStp.updateConf(serverlist:stpConf["serverlist"]+","+clientNode["HostName"]+":"+clientNode["Service"],role:"server",syncinterval:50,maxtimeerror:60000,diaglevel:2);
   //println(primaryNode);
   //执行stp.getConf()获取节点配置信息 
}

function checkeConfInfo(stpConf, roleType)
{
   var isSuccess = true
   if (stpConf["role"] != roleType)
   {
      isSuccess = false;
   }
   var serverGroup = getStpServerNodes(STPHOSTNAME,STPSVCNAME);
   serverGroup.sort();
   var serverList = stpConf["serverlist"].split(",");
   serverList.sort();
   if(serverGroup.length != serverList.length)
   {
      isSuccess = false;
   }
   for(var i=0; i<serverGroup.length; i++)
   {
      if(serverGroup[i] != serverList[i])
      {
         isSuccess = false;
      }
   }
   return isSuccess;
}