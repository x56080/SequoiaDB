/******************************************************************************
*@Description : test js object oma function: addAOmaSvcName delAOmaSvcName 
*                                            getAOmaSvcName
*               TestLink: 10633 Oma获取、增加、删除Oma端口
*                         10634 Oma增加Oma端口，端口已存在，isReplace为true
*                         10635 Oma增加Oma端口，端口已存在，isReplace为false
*@author      : Liang XueWang
******************************************************************************/
// 测试增加、删除、获取oma端口
OmaTest.prototype.testOmaSvcName = function()
{
   this.testInit();

   if( this.oma === Oma )
   {
      var user = System.getCurrentUser().user;
      var file = RSRVNODEDIR + "../conf/sdbcm.conf";
      var obj = getFileUsrGrp( file );
      if( user !== obj["user"] && user !== "root" )
      {
         return;
      }
   }

   // 测试addAOmaSvcName getAOmaSvcName   
   this.oma.addAOmaSvcName( "test", "19000" );
   var result = this.oma.getAOmaSvcName( "test" );
   assert.equal( result, "19000" );

   // 测试delAOmaSvcName  
   this.oma.delAOmaSvcName( "test" );
   result = this.oma.getAOmaSvcName( "test" );
   assert.equal( result, "11790" );

   if( this.oma === Oma )
   {
      if( user !== cmuser )
      {
         File.chown( file, obj );
      }
   }
   else
   {
      this.oma.close();
   }
}

// 测试增加Oma端口，isReplace为true/false
OmaTest.prototype.testOmaSvcNameReplace = function()
{
   this.testInit();

   if( this.oma === Oma )
   {
      var user = System.getCurrentUser().user;
      var file = RSRVNODEDIR + "../conf/sdbcm.conf";
      var obj = getFileUsrGrp( file );
      if( user !== obj["user"] && user !== "root" )
      {
         return;
      }
   }

   // 测试addAOmaSvcName,isReplace为true
   this.oma.addAOmaSvcName( "test", "19000" );
   this.oma.addAOmaSvcName( "test", "18900", true );
   var result = this.oma.getAOmaSvcName( "test" );
   assert.equal( result, "18900" );

   // 测试addAOmaSvcName,isReplace为false
   try
   {
      this.oma.addAOmaSvcName( "test", "19000", false );
      throw new Error( "should error" );
   }
   catch( e )
   {
      if( e.message != -6 )
      {
         throw e;
      }
   }

   this.oma.delAOmaSvcName( "test" );

   if( this.oma === Oma )
   {
      if( user !== cmuser )
      {
         File.chown( file, obj );
      }
   }
   else
   {
      this.oma.close();
   }
}

main( test );

function test ()
{
   // 获取本地和远程主机
   var localhost = toolGetLocalhost();
   var remotehost = toolGetRemotehost();

   var localOma = new OmaTest( localhost, CMSVCNAME );
   var remoteOma = new OmaTest( remotehost, CMSVCNAME );
   var staticOma = new OmaTest();

   var omas = [localOma, remoteOma, staticOma];
   for( var i = 0; i < omas.length; i++ )
   {
      // 测试增加、删除、获取Oma端口
      omas[i].testOmaSvcName();

      // 测试增加Oma端口，isReplace为true/false
      omas[i].testOmaSvcNameReplace();
   }
}


