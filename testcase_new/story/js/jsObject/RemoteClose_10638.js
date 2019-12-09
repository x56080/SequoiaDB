/******************************************************************************
*@Description : test js object Remote function : close
*               TestLink : 10638 关闭Remote对象后，继续执行操作
*@author      : Liang XueWang
******************************************************************************/

// 测试remote关闭后执行操作
RemoteTest.prototype.testClose = function()
{
   this.testInit();
   var system = this.remote.getSystem();
   this.remote.close();

   try
   {
      system.type();
      throw "system call type after remote close should be failed";
   }
   catch( e )
   {
      if( e !== -15 )
      {
         throw buildException( "testClose", e,
            "system call type after remote close " + this, -15, e );
      }
   }
}

function main ()
{
   // 获取远程主机
   var remotehost = toolGetRemotehost();

   // 测试remote关闭后执行操作
   var rt = new RemoteTest( remotehost, CMSVCNAME );
   rt.testClose();
}

main()