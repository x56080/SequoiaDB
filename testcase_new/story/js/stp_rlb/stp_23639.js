/******************************************************************************
 * @Description   : seqDB-23639 stp.updateConf()接口验证 
 * @Author        : Siqin Chen
 * @CreateTime    : 2021.03.23
 * @LastEditTime  : 2021.03.23
 * @LastEditors   : Siqin Chen
 ******************************************************************************/
testConf.skipStandAlone = true;
main( test );
function test ()
{
   //SEQUOIADBMAINSTREAM-6901
   //SEQUOIADBMAINSTREAM-6877
   //testPrimaryUpdateConf23639();
   //testSpareUpdateConf23639();
   //testClientUpdateConf23639();
}

function testPrimaryUpdateConf23639 ()
{
   var stp = new Stp( STPHOSTNAME, STPSVCNAME );
   //2.分别在server/client主备节点上执行stp.updateConf()修改端口号port
   var primaryNode = getStpPrimaryNode( STPHOSTNAME, STPSVCNAME );
   var primaryStp = new Stp( primaryNode["HostName"], primaryNode["Service"] );
   assert.tryThrow( SDB_RTN_CONF_NOT_TAKE_EFFECT, function()
   {
      primaryStp.updateConf( { port: 1111 } );
   } )

   //SEQUOIADBMAINSTREAM-6901
   //3.分别在server/client主备节点上执行stp.updateConf()修改其他参数，覆盖合法值和非法值（包含参数值非法和参数名非法，参数名非法时被忽略，和db一样不限制参数名非法）

   //先保存一份conf，update后方便还原，stp.getConf()
   var conf = JSON.parse( primaryStp.getConf().toString() );

   var clientNode = getStpClientNode( STPHOSTNAME, STPSVCNAME );
   primaryStp.updateConf( { serverlist: conf["serverlist"] + "," + clientNode["HostName"] + ":" + clientNode["Service"], role: "client", syncinterval: 50, maxtimeerror: 60000, diaglevel: 2 } );

   var updateInfo = JSON.parse( primaryStp.getConf() );
   var expInfo = { serverlist: conf["serverlist"] + "," + clientNode["HostName"] + ":" + clientNode["Service"], role: "client", syncinterval: 50, maxtimeerror: 60000, diaglevel: 2 };
   commCompareObject( updateInfo, expInfo );

   //还原
   primaryStp.updateConf( { serverlist: conf["serverlist"], role: conf["role"], syncinterval: conf["syncinterval"], maxtimeerror: conf["maxtimeerror"], diaglevel: conf["diaglevel"] } );
   updateInfo = JSON.parse( primaryStp.getConf() );
   commCompareObject( updateInfo, conf );
}


function testSpareUpdateConf23639 ()
{
   //备节点
   var spareNode = getStpSpareNode( STPHOSTNAME, STPSVCNAME );
   var spareNodeHost = spareNode[0]["HostName"];
   var spareNodePort = spareNode[0]["Service"];
   var spareStp = new Stp( spareNodeHost, spareNodePort );
   assert.tryThrow( SDB_RTN_CONF_NOT_TAKE_EFFECT, function()
   {
      spareStp.updateConf( { port: 1111 } );
   } )

   //先保存一份conf，update后方便还原，stp.getConf()
   var conf = JSON.parse( spareStp.getConf().toString() );

   var clientNode = getStpClientNode( STPHOSTNAME, STPSVCNAME );
   spareStp.updateConf( { serverlist: conf["serverlist"] + "," + clientNode["HostName"] + ":" + clientNode["Service"], role: "client", syncinterval: 50, maxtimeerror: 60000, diaglevel: 2 } );

   var updateInfo = JSON.parse( spareStp.getConf() );
   var expInfo = { serverlist: conf["serverlist"] + "," + clientNode["HostName"] + ":" + clientNode["Service"], role: "client", syncinterval: 50, maxtimeerror: 60000, diaglevel: 2 };
   commCompareObject( updateInfo, expInfo );

   //还原
   spareStp.updateConf( { serverlist: conf["serverlist"], role: conf["role"], syncinterval: conf["syncinterval"], maxtimeerror: conf["maxtimeerror"], diaglevel: conf["diaglevel"] } );
   updateInfo = JSON.parse( spareStp.getConf() );
   commCompareObject( updateInfo, conf );
}

function testClientUpdateConf23639 ()
{
   var clientNode = getStpClientNode( STPHOSTNAME, STPSVCNAME );
   var clientHost = clientNode["HostName"];
   var clientPort = clientNode["Service"];
   var clientStp = new Stp( clientHost, clientPort );
   assert.tryThrow( SDB_RTN_CONF_NOT_TAKE_EFFECT, function()
   {
      clientStp.updateConf( { port: 1111 } );
   } )
   clientStp.updateConf( { serverlist: conf["serverlist"] + "," + clientNode["HostName"] + ":" + clientNode["Service"], role: "client", syncinterval: 50, maxtimeerror: 60000, diaglevel: 2 } );

   var updateInfo = JSON.parse( clientStp.getConf() );
   var expInfo = { serverlist: conf["serverlist"] + "," + clientNode["HostName"] + ":" + clientNode["Service"], role: "client", syncinterval: 50, maxtimeerror: 60000, diaglevel: 2 };
   commCompareObject( updateInfo, expInfo );

   //还原
   clientStp.updateConf( { serverlist: conf["serverlist"], role: conf["role"], syncinterval: conf["syncinterval"], maxtimeerror: conf["maxtimeerror"], diaglevel: conf["diaglevel"] } );
   updateInfo = JSON.parse( clientStp.getConf() );
   commCompareObject( updateInfo, conf );
}