/****************************************************
@description:	create collectionspace, normal case
         testlink cases: seqDB-7185 & 7188
@input:		run CURL command: 
				1 curl http://127.0.0.1:11814/ -d "create collectionspace&name=foo"
				2 curl http://127.0.0.1:11814/ -d "create collectionspace&name=FOO"
				5 curl http://127.0.0.1:11814/ -d "drop collectionspace&name=foo"
@expectation:	1/2/3 return errno:0
@modify list:
            	2015-4-3 Ting YU init   2016-3-16 XiaoNi Huang init
****************************************************/
var csName = CHANGEDPREFIX + "yuting";
var cs = "name=" + csName;

function createcsAndCheck ()
{
	tryCatch( ["cmd=create collectionspace", cs], [0], "Failed to create cs by rest cmd!" );
	try
	{
		db.getCS( csName );
	}
	catch( e )
	{
		println( 'Error: rest [cmd=create collectionspace]  has been done, but cs does not exist' );
		throw e;
	}
}

function createcsByCapital ()
{
	tryCatch( ["cmd=create collectionspace", "name=" + csName + "FOO"], [0], getFuncName() + "Error occurs" );
	try
	{
		db.getCS( csName + "FOO" );
	} catch( e )
	{
		println( getFuncName() + 'Faied to get cs: ' + csName + "FOO" );
		throw e;
	}
	try
	{
		db.dropCS( csName + "FOO" );
	} catch( e )
	{
		println( getFuncName() + 'Faied to drop cs: ' + csName + "FOO" );
		throw e;
	}
}

function dropcsAndCheck ()
{
	tryCatch( ["cmd=drop collectionspace", cs], [0], "Fail to drop existed cs by rest cmd!" );
	try
	{
		var flag = false;
		db.getCS( csName );
		flag = true;
	}
	catch( e )
	{
		if( e != -34 )
		{
			println( "Error: rest [cmd=drop collectionspace]  has been done, db.getCS(" + csName + "), expect: return -34, actually: return " + e );
			throw e;
		}
	}

	if( true == flag )
	{
		db.dropCS( csName );
		throw "Error: rest [cmd=drop collectionspace]  has been done, db.getCS(" + csName + "), expect: return -34, actually: return 0";
	}
}

commDropCS( db, csName, true, "drop cs in begin" );
commDropCS( db, csName + 'FOO', true, "drop cs in begin" );

createcsAndCheck();
createcsByCapital();
dropcsAndCheck();