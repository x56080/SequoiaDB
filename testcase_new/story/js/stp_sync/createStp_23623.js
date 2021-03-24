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
main( test );
function test ()
{
   testCreateStp23623();
   testOmaGetStp23624();
}

function testCreateStp23623()
{
   var oma = new Oma("localhost",CMSVCNAME);
   var stpConf = null;
   try
   {
      var conf = oma.getStp().getConf();
      //当前机器存在stp时，先备份该机器的stp节点信息，然后再删除
      stpConf = JSON.parse(conf.toString());
   }
   catch( e )
   {
      if( e != -146 )
      {
         throw new Error( "oma.getStp() " + e );
      }
   }
   //1.不传入config值，检查默认值正确性； 
   //清理当前机器上的stp环境，一台机器只能部署一个stp节点
   try
   {
      oma.removeStp();
   }
   catch( e )
   {
      if( e != -146 )
      {
         throw new Error( " clear env : oma.removeStp() " + e );
      }
   }
   oma.createStp();
   oma.startStp();
   //seqDB-23626 1.启动stp节点、重复启动stp节点
   try
   {
      oma.startStp();
   }
   catch( e )
   {
      if( e != -143 )
      {
         throw new Error( " oma.startStp() " + e );
      }
   }
   
   var localStp = new Stp();
   var localConf = JSON.parse( localStp.getConf().toString());
   delete localConf["serverlist"];
   var expConf = {port:9622,role:"server",syncinterval:60,maxtimeerror:50000,diaglevel:3};
   commCompareObject(localConf, expConf);
   //seqDB-23626 oma.stopStp() 2.停止stp节点，重复停止相同节点
   oma.stopStp();
   oma.stopStp();
   //2.传入所有参数的非默认值/非法配置项/非法配置值，检查stp节点信息正确性；
   try
   {
      oma.removeStp();
   }
   catch( e )
   {
      if( e != -146 )
      {
         throw new Error( " clear env : oma.removeStp() " + e );
      }
   }
   var configs = {serverlist:"u1604-csq:9623",port:9633,role:"server",syncinterval:50,maxtimeerror:60000,diaglevel:2};
   oma.createStp(configs);
   oma.startStp();
   localStp = new Stp(STPHOSTNAME,9633);
   localConf = JSON.parse( localStp.getConf().toString());
   commCompareObject(localConf, configs);
   
   //3.创建已存在stp节点，错误信息正确; 
   try
   {
      oma.createStp(configs);
   }
   catch( e )
   {
      if( e != -145 )
      {
         throw new Error( "oma.createStp(configs) " + e );
      }
   }
   
   //4.创建7个server节点 oma不支持远程创建stp节点，只需验证指定serverlist有7个server节点时当前stp节点是否能创建成功
   try
   {
      oma.removeStp();
   }
   catch( e )
   {
      if( e != -146 )
      {
         throw new Error( " clear env : oma.removeStp() " + e );
      }
   }
   //SEQUOIADBMAINSTREAM-6892
   //configs = {serverlist:"host1:9622,host2:9622,host3:9622,host4:9622,host5:9622,host6:9622,host7:9622"};
   //oma.createStp(configs);
   //5.创建8个server节点，该测试点报错，验证报错信息 SEQUOIADBMAINSTREAM-6892
   if(stpConf != null)
   {
      oma.createStp(stpConf);
      oma.startStp();
   }
}

function testOmaGetStp23624()
{
   var oma = new Oma("localhost",CMSVCNAME);
   var stp = oma.getStp();
   stp.stop();
   stp.start();
   stp.stop();
   stp.start();
}