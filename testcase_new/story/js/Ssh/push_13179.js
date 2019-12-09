/******************************************************************************
*@Description : test js object ssh function:
*               push( local_file, dst_file, [mode] )
*               seqDB-13179:使用ssh推送拉取文件
*               seqDB-13180:使用ssh推送拉取文件，指定文件权限
*               seqDB-13181:使用ssh推送拉取文件，源文件不存在
*               seqDB-13182:使用ssh推送拉取文件，源文件无权限
*               seqDB-13190:使用ssh推送拉取文件，目标文件已存在（有权限）
*               seqDB-13191:使用ssh推送拉取文件，目标文件已存在（无权限）
*@author      : Liang XueWang
******************************************************************************/
// seqDB-13179:使用ssh推送拉取文件
function testPush ( hostname )
{
   var srcFile = "/tmp/pushsrc_13179.txt";
   var dstFile = "/tmp/pushdst_13179.txt";
   var content = "testPush";

   rmLocalFile( srcFile );
   rmRemoteFile( hostname, dstFile );

   var file = new File( srcFile );
   file.write( content );
   file.close();

   var ssh = newSsh( hostname, sdbUser, sdbPasswd, sshPort );
   ssh.push( srcFile, dstFile );
   ssh.close();

   checkRemoteFile( hostname, dstFile, "rwxr-xr-x", content );

   rmLocalFile( srcFile );
   rmRemoteFile( hostname, dstFile );
}

// seqDB-13180:使用ssh推送拉取文件，指定文件权限
function testPushWithMode ( hostname )
{
   var srcFile = "/tmp/pushsrc_13180.txt";
   var dstFile = "/tmp/pushdst_13180.txt";
   var srcMode = 0644;
   var modes = [0755, 0644, 0640];
   var perms = ["rwxr-xr-x", "rw-r--r--", "rw-r-----"];
   var content = "testPushWithMode";

   rmLocalFile( srcFile );
   rmRemoteFile( hostname, dstFile );

   var file = new File( srcFile, srcMode );
   file.write( content );
   file.close();

   var ssh = newSsh( hostname, sdbUser, sdbPasswd, sshPort );
   for( var i = 0; i < modes.length; i++ )
   {
      ssh.push( srcFile, dstFile, modes[i] );
      checkRemoteFile( hostname, dstFile, perms[i], content );
      rmRemoteFile( hostname, dstFile );
   }
   ssh.close();

   rmLocalFile( srcFile );
   rmRemoteFile( hostname, dstFile );
}

// seqDB-13181:使用ssh推送拉取文件，源文件不存在
function testPushNotExist ( hostname )
{
   var srcFile = "/tmp/pushsrc_13181.txt";
   var dstFile = "/tmp/pushdst_13181.txt";

   rmLocalFile( srcFile );

   var ssh = newSsh( hostname, sdbUser, sdbPasswd, sshPort );
   try
   {
      ssh.push( srcFile, dstFile );
      throw 0;
   }
   catch( e )
   {
      if( e !== -4 )
      {
         throw buildException( "testPushNotExist", e,
            "test push not exist file " + srcFile, -4, e );
      }
   }
   ssh.close();
}

// seqDB-13182:使用ssh推送拉取文件，源文件无权限
function testPushSrcPermission ( hostname )
{
   var srcFile = "/tmp/pushsrc_13182.txt";
   var dstFile = "/tmp/pushdst_13182.txt";
   var srcModes = [0333, 0555, 0666]; // 无读、写、执行权限
   var errnos = [0, 0, 0];
   var user = System.getCurrentUser().toObj().user;
   if( user !== "root" )
      errnos[0] = -3;
   var content = "testPushSrcPermission";

   rmLocalFile( srcFile );
   rmRemoteFile( hostname, dstFile );
   var ssh = newSsh( hostname, sdbUser, sdbPasswd, sshPort );

   for( var i = 0; i < srcModes.length; i++ )
   {
      var file = new File( srcFile );
      file.write( content );
      file.chmod( srcFile, srcModes[i] );
      file.close();

      try
      {
         ssh.push( srcFile, dstFile );
         if( errnos[i] !== 0 )
            throw 0;
      }
      catch( e )
      {
         if( e !== errnos[i] )
         {
            throw buildException( "testPushSrcPermission", e, "test push file "
               + srcFile + " " + srcModes[i] + " " + user, "0 -3", e );
         }
      }

      if( errnos[i] === 0 )
         checkRemoteFile( hostname, dstFile, "rwxr-xr-x", content );
      rmLocalFile( srcFile );
      rmRemoteFile( hostname, dstFile );
   }
   ssh.close();
}

// seqDB-13190:使用ssh推送拉取文件，目标文件已存在（有权限）
function testPushDstExisted ( hostname )
{
   var srcFile = "/tmp/pushsrc_13190.txt";
   var dstFile = "/tmp/pushdst_13190.txt";
   var srcContent = "testPushDstExisted_src";
   var dstContent = "testPushDstExisted_dst";
   var dstMode = 0755;
   var permission = "rwxr-xr-x";

   rmLocalFile( srcFile );
   rmRemoteFile( hostname, dstFile );

   var file = new File( srcFile );
   file.write( srcContent );
   file.close();
   var remote = new Remote( hostname, CMSVCNAME );
   file = remote.getFile( dstFile, dstMode );
   file.write( dstContent );
   file.close();
   remote.close();

   var ssh = newSsh( hostname, sdbUser, sdbPasswd, sshPort );
   ssh.push( srcFile, dstFile );
   ssh.close();

   checkRemoteFile( hostname, dstFile, permission, srcContent );

   rmLocalFile( srcFile );
   rmRemoteFile( hostname, dstFile );
}

// seqDB-13191:使用ssh推送拉取文件，目标文件已存在（无权限）
function testPushDstPermission ( hostname )
{
   var srcFile = "/tmp/pushsrc_13191.txt";
   var dstFile = "/tmp/pushdst_13191.txt";
   var srcContent = "testPushDstPermission_src";
   var dstContent = "testPushDstPermission_dst";
   var dstModes = [0333, 0555, 0666];
   var perms = ["-wx-wx-wx", "r-xr-xr-x", "rw-rw-rw-"];

   rmLocalFile( srcFile );
   rmRemoteFile( hostname, dstFile );

   var file = new File( srcFile );
   file.write( srcContent );
   file.close();
   var remote = new Remote( hostname, CMSVCNAME );
   var ssh = newSsh( hostname, sdbUser, sdbPasswd, sshPort );

   for( var i = 0; i < dstModes.length; i++ )
   {
      var file = remote.getFile( dstFile );
      file.write( dstContent );
      file.chmod( dstFile, dstModes[i] );
      file.close();

      try
      {
         ssh.push( srcFile, dstFile );
         if( dstModes[i] === 0555 )
            throw 0;
      }
      catch( e )
      {
         if( dstModes[i] !== 0555 && e !== -10 )
         {
            throw buildException( "testPushDstPermission", e,
               "test push dst file " + dstFile + " " + dstModes[i] +
               " " + hostname, "0, -10", e );
         }
      }

      rmRemoteFile( hostname, dstFile );
   }

   ssh.close();
   remote.close();

   rmLocalFile( srcFile );
   rmRemoteFile( hostname, dstFile );
}

function main ()
{
   var remotehost = toolGetRemotehost();
   println( "ssh hostname: " + remotehost );

   if( !checkSsh( remotehost, sdbUser, sdbPasswd, sshPort ) ||
      !checkCmUser( remotehost, sdbUser ) )
   {
      return;
   }

   testPush( remotehost );
   testPushWithMode( remotehost );
   testPushNotExist( remotehost );
   testPushSrcPermission( remotehost );
   testPushDstExisted( remotehost );
   testPushDstPermission( remotehost );
}

main()
