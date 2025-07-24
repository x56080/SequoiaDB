/******************************************************************************
 * @Description   : seqDB-23623 oma.createStp( [config] )接口验证 
                    seqDB-23624 oma.getStp()接口验证 
                    seqDB-23626 oma.startStp()/oma.stopStp()接口验证
 * @Author        : Siqin Chen
 * @CreateTime    : 2021.03.23
 * @LastEditTime  : 2021.03.23
 * @LastEditors   : Siqin Chen
 ******************************************************************************/
testConf.skipStandAlone = true;
// main( test );
function test ()
{
   testCreateStp23623();
   testOmaGetStp23624();
}

function testCreateStp23623()
{
   var oma = new Oma(COORDHOSTNAME, CMSVCNAME);
   //SEQUOIADBMAINSTREAM-6905 oma不能删除role为server的stp
   assert.tryThrow( SDB_OPTION_NOT_SUPPORT, function(){
            oma.removeStp();
         })
   
   var stpConf = null;
   try
   {
      var conf = oma.getStp().getConf();
      //当前机器存在stp时，先备份该机器的stp节点信息，然后再删除
      stpConf = JSON.parse(conf.toString());
      var stp = new Stp();
      stp.updateConf({"role": "client"});
      oma.removeStp();
   }
   catch( e )
   {
      if( e != -146 )
      {
         throw new Error( "oma.getStp() " + e );
      }
   }
   //1.不传入config值，检查默认值正确性； 
   oma.createStp();
   oma.startStp();
   //seqDB-23626 1.启动stp节点、重复启动stp节点
   //SEQUOIADBMAINSTREAM-6960
   oma.startStp();
   var localStp = new Stp();
   var localConf = JSON.parse( localStp.getConf().toString());
   delete localConf["serverlist"];
   var expConf = {port:9622,role:"server",syncinterval:60,maxtimeerror:50000,diaglevel:3};
   commCompareObject(localConf, expConf);
   localStp.updateConf({"role": "client"});
   
   //seqDB-23626 oma.stopStp() 2.停止stp节点，重复停止相同节点
   oma.stopStp();
   oma.stopStp();
   
   
   oma.removeStp();
   var hostName = System.getHostName();
   //SEQUOIADBMAINSTREAM-6892
   //2.传入所有参数的非默认值/非法配置项/非法配置值，检查stp节点信息正确性；
   var configsA = {serverlist:hostName+":9622",port:0,role:"server"};
   assert.tryThrow( SDB_INVALIDARG, function(){
            oma.createStp(configsA);
         })
   
   //5.创建8个server节点，该测试点报错，验证报错信息
   var configsB = {serverlist:"localhost1:9622,localhost2:9622,localhost3:9622,localhost4:9622,localhost5:9622,localhost6:9622,localhost7:9622,localhost8:9622"}
   assert.tryThrow( SDB_INVALIDARG, function(){
            oma.createStp(configsB);
         })
   
   var configs = {serverlist:hostName+":9623",port:9633,role:"server",syncinterval:50,maxtimeerror:60000,diaglevel:2};
   oma.createStp(configs);
   oma.startStp();
   localStp = new Stp(STPHOSTNAME,9633);
   localConf = JSON.parse( localStp.getConf().toString());
   commCompareObject(localConf, configs);
   localStp.updateConf({"role": "client"});
   
   //3.创建已存在stp节点，错误信息正确; 
   assert.tryThrow( SDBCM_NODE_EXISTED, function(){
            oma.createStp(configs);
         })
   
   oma.removeStp();
   
   if(stpConf != null)
   {
      oma.createStp(stpConf);
      oma.startStp();
   }
}

function testOmaGetStp23624()
{
   var oma = new Oma(COORDHOSTNAME, CMSVCNAME);
   var stp = oma.getStp();
   stp.stop();
   stp.start();
   stp.getConf();
}
