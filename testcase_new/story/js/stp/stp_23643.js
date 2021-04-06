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
   //1.分别在server/client上执行stp.getConf()获取当前stp节点配置信息
   var primaryNode = getStpPrimaryNode();
   var clientNode = getStpClientNode();
   var clientHost = clientNode["HostName"];
   var clientPort = clientNode["Service"];
   
   var primaryStp = new Stp(primaryNode["HostName"], primaryNode["Service"]);
   var clientStp = new Stp(clientHost, clientPort);
   
   var primaryConf = primaryStp.getConf();
   var conf = JSON.parse( primaryConf.toString());
   checkeConfInfo(conf, "server");
   
   var clientConf = clientStp.getConf();
   conf = JSON.parse( clientConf.toString());
   checkeConfInfo(conf, "client");

   //2.分别在server/client上修改stp节点所有配置项（port除外）该测试点在updaconf接口中验证
}

function checkeConfInfo(stpConf, roleType)
{
   var isSuccess = true
   if (stpConf["role"] != roleType)
   {
       throw new Error("fail to check role, expected: "+roleType+",actual: "+stpConf["role"]);
   }
   var expectedResult = getStpServerNodes();
   expectedResult.sort();
   var serverList = stpConf["serverlist"].split(",");
   serverList.sort();
   commCompareObject(expectedResult, serverList);
}