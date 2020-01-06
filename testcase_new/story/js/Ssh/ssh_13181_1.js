/******************************************************************************
*@Description : seqDB-13181: 使用ssh推送拉取文件，源文件不存在
*@author      : Liang XueWang
******************************************************************************/
main( test );

function test()
{
   var hostName = getRemoteHostName();
   if( !checkCmUser( hostName, user ) )
   {
      return;
   } 

   var srcFile = "/tmp/pushsrc_13181_1.txt";
   var dstFile = "/tmp/pushdst_13181_1.txt";

   cleanLocalFile( srcFile );
   cleanRemoteFile( hostName, CMSVCNAME, dstFile );

   var ssh = new Ssh( hostName, user, password, port );
   try
   {
      ssh.push( srcFile, dstFile );
      throw "NEED_ERROR";
   }
   catch( e )
   {
      if( e !== -4 )
      {
         throw new Error( e );
      }
   }
   ssh.close();
}
