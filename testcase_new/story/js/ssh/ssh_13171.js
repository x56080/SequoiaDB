/******************************************************************************
*@Description : seqDB-13171:使用ssh连接未建立信赖关系的用户( 不指定密码 )
*               seqDB-13172:使用ssh连接主机用户，用户密码错误
*               seqDB-13173:使用ssh连接主机用户，端口错误
*               seqDB-13174:不指定用户、用户密码、端口（使用默认值）创建ssh
*               seqDB-13175:指定用户、用户密码、端口创建ssh
*@author      : Liang XueWang
******************************************************************************/
main( test );

function test()
{
   if( !isUserExist( COORDHOSTNAME, user ) )
   {
      return;
   }

   //使用未建立信赖关系的用户名建立ssh连接
   try
   {
      var ssh = new SshObj( COORDHOSTNAME, "user" );
      throw "Connect to " + COORDHOSTNAME + " should be failed!";
   }
   catch( e )
   {
      if( !commCompareErrorCode( e, -6 ) )
      {
         commThrowError( e );         
      }
   }
  
   //使用错误的密码建立ssh连接
   try
   {
      ssh = new SshObj( COORDHOSTNAME, user, "password" );
      throw "Connect to " + COORDHOSTNAME + " should be failed!";
   }
   catch( e )
   {
      if( !commCompareErrorCode( e, -6 ) )
      {
         commThrowError( e );
      }
   }

   //使用错误的端口号建立ssh连接
   try
   {
      ssh = new SshObj( COORDHOSTNAME, user, password, 8 );
      throw "Connect to " + COORDHOSTNAME + " should be failed!";
   }
   catch( e )
   {
      if( !commCompareErrorCode( e, -79 ) )
      {
         commThrowError( e );
      }
   }
  
   //password和port使用默认值建立ssh连接
   
   //指定用户名、密码、端口号建立ssh连接
   ssh = new SshObj( COORDHOSTNAME, user, password, port );
   ssh.close()
}

