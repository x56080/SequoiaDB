/******************************************************************************
*@Description : test js object ssh function:
*               pull( remote_file, local_file, [mode] )
*               seqDB-13179:使用ssh推送拉取文件
*               seqDB-13180:使用ssh推送拉取文件，指定文件权限
*               seqDB-13181:使用ssh推送拉取文件，源文件不存在
*               seqDB-13182:使用ssh推送拉取文件，源文件无权限
*               seqDB-13190:使用ssh推送拉取文件，目标文件已存在（有权限）
*               seqDB-13191:使用ssh推送拉取文件，目标文件已存在（无权限）
*@author      : Liang XueWang
******************************************************************************/
// seqDB-13179:使用ssh推送拉取文件
function testPull( hostname )
{
   var remoteFile = "/tmp/pullsrc_13179.txt"; 
   var localFile = "/tmp/pulldst_13179.txt"; 
   var content = "testPull"; 
   
   rmLocalFile( localFile ); 
   rmRemoteFile( hostname, remoteFile ); 
   
   var remote = new Remote( hostname, CMSVCNAME ); 
   var file = remote.getFile( remoteFile ); 
   file.write( content ); 
   file.close(); 
   
   var ssh = newSsh( hostname, sdbUser, sdbPasswd, sshPort ); 
   ssh.pull( remoteFile, localFile ); 
   ssh.close(); 
   
   checkLocalFile( localFile, "rw-r-----", content ); 
   
   rmLocalFile( localFile ); 
   rmRemoteFile( hostname, remoteFile ); 
}

// seqDB-13180:使用ssh推送拉取文件，指定文件权限
function testPullWithMode( hostname )
{
   var remoteFile = "/tmp/pullsrc_13180.txt"; 
   var localFile = "/tmp/pulldst_13180.txt"; 
   var srcMode = 0644; 
   var modes = [ 0755, 0644, 0640 ]; 
   var perms = [ "rwxr-xr-x", "rw-r--r--", "rw-r-----" ]; 
   var content = "testPullWithMode"; 
   
   rmLocalFile( localFile ); 
   rmRemoteFile( hostname, remoteFile ); 
   
   var remote = new Remote( hostname, CMSVCNAME ); 
   var file = remote.getFile( remoteFile, srcMode ); 
   file.write( content ); 
   file.close(); 
   remote.close(); 
   
   var ssh = newSsh( hostname, sdbUser, sdbPasswd, sshPort ); 
   for( var i = 0; i < modes.length; i++ )
   {
      ssh.pull( remoteFile, localFile, modes[i] ); 
      checkLocalFile( localFile, perms[i], content ); 
      rmLocalFile( localFile ); 
   }
   ssh.close(); 
   
   rmLocalFile( localFile ); 
   rmRemoteFile( hostname, remoteFile ); 
}

// seqDB-13181:使用ssh推送拉取文件，源文件不存在
function testPullNotExist( hostname )
{
   var remoteFile = "/tmp/pullsrc_13181.txt"; 
   var localFile = "/tmp/pulldst_13181.txt"; 
   
   rmRemoteFile( hostname, remoteFile ); 
   
   var ssh = newSsh( hostname, sdbUser, sdbPasswd, sshPort ); 
   try
   {
      ssh.pull( remoteFile, localFile ); 
      throw 0; 
   }
   catch( e )
   {
      if( e !== -10 )
      {
         throw buildException( "testPullNotExist", e, 
         "test pull not exist file " + remoteFile, -4, e ); 
      }
   }
   ssh.close(); 
}

// seqDB-13182:使用ssh推送拉取文件，源文件无权限
function testPullSrcPermission( hostname )
{
   var remoteFile = "/tmp/pullsrc_13182.txt"; 
   var localFile = "/tmp/pulldst_13182.txt"; 
   var srcModes = [ 0333, 0555, 0666 ]; // 无读、写、执行权限
   var content = "testPullSrcPermission"; 
   
   rmLocalFile( localFile ); 
   rmRemoteFile( hostname, remoteFile ); 
   var ssh = newSsh( hostname, sdbUser, sdbPasswd, sshPort ); 
   var remote = new Remote( hostname, CMSVCNAME ); 
   
   for( var i = 0; i < srcModes.length; i++ )
   {
      var file = remote.getFile( remoteFile ); 
      file.write( content ); 
      file.chmod( remoteFile, srcModes[i] ); 
      file.close(); 
      
      try
      {
         ssh.pull( remoteFile, localFile ); 
         if( srcModes[i] === 0333 )
         throw 0; 
      }
      catch( e )
      {
         if( srcModes[i] !== 0333 && e !== -10 )
         {
            throw buildException( "testPullSrcPermission", e, 
            "test pull file " + remotefile + " " + srcModes[i] + 
            " " + hostname, "0 -10", e ); 
         }
      }
      
      if( srcModes[i] !== 0333 )
      checkLocalFile( localFile, "rw-r-----", content ); 
      rmLocalFile( localFile ); 
      rmRemoteFile( hostname, remoteFile ); 
   }
   
   remote.close(); 
   ssh.close(); 
}

// seqDB-13190:使用ssh推送拉取文件，目标文件已存在（有权限）
function testPullDstExisted( hostname )
{
   var remoteFile = "/tmp/pullsrc_13190.txt"; 
   var localFile = "/tmp/pulldst_13190.txt"; 
   var srcContent = "testPullDstExisted_src"; 
   var dstContent = "testPullDstExisted_dst"; 
   var dstMode = 0755; 
   var permission = "rwxr-xr-x"; 
   
   rmLocalFile( localFile ); 
   rmRemoteFile( hostname, remoteFile ); 
   
   var remote = new Remote( hostname, CMSVCNAME ); 
   var file = remote.getFile( remoteFile ); 
   file.write( srcContent ); 
   file.close(); 
   remote.close(); 
   file = new File( localFile, dstMode ); 
   file.write( dstContent ); 
   file.close(); 
   
   var ssh = newSsh( hostname, sdbUser, sdbPasswd, sshPort ); 
   
   ssh.pull( remoteFile, localFile ); 
   ssh.close(); 
   
   checkLocalFile( localFile, permission, srcContent ); 
   
   rmLocalFile( localFile ); 
   rmRemoteFile( hostname, remoteFile ); 
}

// seqDB-13191:使用ssh推送拉取文件，目标文件已存在（无权限）
function testPullDstPermission( hostname )
{
   var remoteFile = "/tmp/pullsrc_13191.txt"; 
   var localFile = "/tmp/pulldst_13191.txt"; 
   var srcContent = "testPullDstPermission_src"; 
   var dstContent = "testPullDstPermission_dst"; 
   var dstModes = [ 0333, 0555, 0666 ]; 
   var perms = [ "-wx-wx-wx", "r-xr-xr-x", "rw-rw-rw-" ]; 
   var errnos = [ 0, 0, 0 ]; 
   var user = System.getCurrentUser().toObj().user; 
   if( user !== "root" )
   errnos[1] = -3; 
   
   rmLocalFile( localFile ); 
   rmRemoteFile( hostname, remoteFile ); 
   
   var remote = new Remote( hostname, CMSVCNAME ); 
   var file = remote.getFile( remoteFile ); 
   file.write( srcContent ); 
   file.close(); 
   remote.close(); 
   
   var ssh = newSsh( hostname, sdbUser, sdbPasswd, sshPort ); 
   
   for( var i = 0; i < dstModes.length; i++ )
   {
      var file = new File( localFile ); 
      file.write( dstContent ); 
      file.chmod( localFile, dstModes[i] ); 
      file.close(); 
      
      try
      {
         ssh.pull( remoteFile, localFile ); 
         if( errnos[i] !== 0 )
         throw 0; 
      }
      catch( e )
      {
         if( e !== errnos[i] )
         {
            throw buildException( "testPullDstPermission", e, 
            "test pull local file " + localFile + " " + dstModes[i], 
            "0 -3", e ); 
         }
      }
      
      rmLocalFile( localFile ); 
   }
   
   ssh.close(); 
   remote.close(); 
   
   rmLocalFile( localFile ); 
   rmRemoteFile( hostname, remoteFile ); 
}

function main()
{
   var remotehost = toolGetRemotehost(); 
   println( "ssh hostname: " + remotehost ); 
   
   if( !checkSsh( remotehost, sdbUser, sdbPasswd, sshPort )||
   !checkCmUser( remotehost, sdbUser ) )
   {
      return; 
   }
   
   testPull( remotehost ); 
   testPullWithMode( remotehost ); 
   testPullNotExist( remotehost ); 
   testPullSrcPermission( remotehost ); 
   testPullDstExisted( remotehost ); 
   testPullDstPermission( remotehost ); 
}

main()
