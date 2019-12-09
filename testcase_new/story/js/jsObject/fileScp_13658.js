/******************************************************************************
*@Description : seqDB-13658:scp命令结束后，File.scp内部显示调用close关闭File对象 
*@Author      : 2019-3-19  XiaoNi Huang
******************************************************************************/
main();

function main ()
{
	println( "\n---Begin to run test" );
	var installDir = toolGetSequoiadbDir( COORDHOSTNAME, CMSVCNAME );
	var srcFile = installDir[0] + '/bin/sdblist';
	var dstFile = WORKDIR + '/sdblist';
	var cmd = new Cmd();

	//TODO:这里似乎没有覆盖远程文件
	println( "\n---Begin to exec File.scp" );
	// expect success, if it fails, it will throw "~bash: ./sdblist: Text file busy"
	File.scp( srcFile, dstFile );
	// check results
	var rc = cmd.run( dstFile + " -t all" );
	var rcObj = rc.split( '\n' );
	var actRcCode1 = rcObj[0].indexOf( "sequoiadb(" );
	var actRcCode2 = rcObj[0].indexOf( "sdb" );
	if( actRcCode1 !== 0 && actRcCode2 !== 0 )
	{
		throw buildException( "main", null, "", "0", "  " + actRcCode1 && actRcCode2 );
	}
}