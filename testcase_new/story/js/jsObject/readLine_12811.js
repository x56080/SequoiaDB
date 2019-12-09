/************************************
*@Description: 
*@author:      liuxiaoxuan
*@createdate:  2017.10.11
*@testlinkCase:seqDB-12811
**************************************/
function main ()
{

	//WORKDIR: /tmp/jstest
	if( !File.exist( WORKDIR ) )
	{
		File.mkdir( WORKDIR, 0777 );
	}

	//read and check empty file
	var emptyFileName = WORKDIR + "/emptyFile_12811";
	readAndCheckEmptyFile( emptyFileName );

	//check text file
	var NonEmptyFileName = WORKDIR + "/NonEmptyFile_12811";

	//write content to text file
	var content1 = 'abcdabcdabcdabcdabcdabcdabcdabcd';
	writeContentToFile( NonEmptyFileName, content1 );

	//read and check not empty file
	var expectContent1 = ['abcdabcdabcdabcdabcdabcdabcdabcd'];
	readAndCheckNotEmptyFile( NonEmptyFileName, expectContent1 );

	//write content to text file(include '\t','\n'')
	var content2 = 'abcd\tabcd\nabcd\tabcd\tabcd\nabcd';
	writeContentToFile( NonEmptyFileName, content2 );

	//read and check not empty file
	var expectContent2 = ['abcd	abcd\n', 'abcd	abcd	abcd\n', 'abcd'];
	var fileMode = SDB_FILE_READONLY;
	readAndCheckNotEmptyFile( NonEmptyFileName, expectContent2, fileMode );

}

function readAndCheckEmptyFile ( emptyFileName )
{
	try
	{
		if( File.exist( emptyFileName ) )
		{
			File.remove( emptyFileName );
		}
		var emptyFile = new File( emptyFileName );
		var content = emptyFile.readLine();
		throw "EXPECT HIT END ERROR";
	} catch( e )
	{
		if( -9 !== e )
		{
			throw buildException( "readAndCheckEmptyFile()", e, "read empty file", "Failed", "Success" );
		}
	}
	File.remove( emptyFileName );
}

function writeContentToFile ( writeFileName, content )
{
	try
	{
		if( File.exist( writeFileName ) )
		{
			File.remove( writeFileName );
		}
		var writeFile = new File( writeFileName );
		writeFile.write( content );
	} catch( e )
	{
		throw buildException( "writeContentToFile()", e, e, 'Success', 'Failed' );
	}
}

function readAndCheckNotEmptyFile ( fileName, expectContent, fileMode )
{
	if( typeof ( fileMode ) == "undefined" ) { fileMode = SDB_FILE_READWRITE; }

	try
	{
		var readFile = new File( fileName, 0777, fileMode );
		var readLength = 0;
		var fileSize = File.getSize( fileName );
		var actContent = [];
		while( true )
		{
			var content = readFile.readLine();
			actContent.push( content );
			readLength = readLength + content.length;
			if( readLength >= fileSize )
			{
				println( "readLength: " + readLength + " fileSize: " + fileSize );
				break;
			}
		}

		//check the content length 
		if( actContent.length !== expectContent.length )
		{
			throw buildException( "check file content", null, "", actContent.length, expectContent.length );
		}
		//check content each row 
		for( var i in actContent )
		{
			var actRow = actContent[i];
			var expRow = expectContent[i];
			if( actRow !== expRow )
			{
				throw buildException( "check content error", e, "check error", actRow, expRow );
			}
		}

	} catch( e )
	{
		throw buildException( "readAndCheckNotEmptyFile()", e, 'check error', 'Success', 'Failed' );
	}
	File.remove( fileName );
}
main();