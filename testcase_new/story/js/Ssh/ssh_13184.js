/******************************************************************************
*@Description : seqDB-13184:重复执行close关闭ssh连接
                seqDB-13185:关闭ssh连接后执行操作
*@author      : Liang XueWang
******************************************************************************/
main( test );

function test()
{
   var ssh = new Ssh( COORDHOSTNAME, user, password, port );
   ssh.close();
   ssh.close();
  
   ssh.getLocalIP();
   ssh.getPeerIP();   
   ssh.getLastRet();
   ssh.getLastOut();
   
   try
   {
      ssh.exec( "hostname" );
      throw "NEED_ERROR";
   }
   catch( e )
   {
      if( e !== -15 )
      {
         throw new Error( e );
      }
   }
 
   try
   {
      ssh.push( "/tmp/src.txt", "/tmp/dst.txt" );
      throw "NEED_ERROR";
   }
   catch( e )
   {
      if( e !== -15 )
      {
         throw new Error( e );
      }
   }
   
   try
   {
      ssh.pull( "/tmp/src.txt", "/tmp/dst.txt" );
      throw "NEED_ERROR";
   }
   catch( e )
   {
      if( e !== -15 )
      {
         throw new Error( e );
      }
   }
}


