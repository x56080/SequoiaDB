/****************************************************
@description: 对单条lob进行插入查询删除操作
@author:
              2015-10-27 TingYU
****************************************************/
main();

function main ()
{
	try
	{
		var cl = readyCL( { ReplSize: 0 } );

		//generate a lob file
		var fileName = CHANGEDPREFIX + "_lobtest.file";
		var fileSize = "10M";
		var srcMd5 = create1File( fileName, fileSize );//md5sum of source file

		//put lob and check
		var lobNum = 1;
		var lobIdArr = putSomeLobs( cl, fileName, lobNum );
		var expLobArr = lobIdArr;
		checkLob( cl, expLobArr, srcMd5 );

		//delete lob and check
		deleteSomeLobs( cl, lobIdArr );
		var expLobArr = [];
		checkLob( cl, expLobArr, "" );

		clean();
	}
	catch( e )
	{
		throw e;
	}
	finally
	{
		var cmd = new Cmd();
		cmd.run( "rm -rf *" + CHANGEDPREFIX + "*.file" );
	}
}