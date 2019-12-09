/****************************************************
@description:	create/drop cs, abnormal case
         testlink cases: seqDB-7186 & 7187 & 7189
@expectation:	1 twoName(): "XXXX&nam=foo&name=foo1" expect: create foo, dont create foo1
				2 lackCmd(): expect: return -100
				3 lackName(): expect: return -6
				4 lackCmdAndName(): expect: return 0
				5 misspell(): cmd misspell expect: return -100
						  	   name misspell expect: return -6
				6 formatError1(): "XXXX&name=foo=foo" expect: create foo=foo
				7 formatError2(): "XXXX&name&b=foo" expect: return -6
				8 formatError3(): "XXXX&name==foo" expect: create =foo
				9 formatError4(): "XXXX&&&name=foo" expect: create foo
				10 lackNameDropCS(): expect: return -6
				note: [1-9] cases are for create cs, [10] case is for drop cs.
@modify list:
               2015-3-30 Ting YU init   2016-3-16 XiaoNi Huang init
****************************************************/
var csName = CHANGEDPREFIX;
var cs = "name=" + csName;

function twoName ()
{  //"XXXX&nam=foo&name=foo1" 
	var cs1Name = csName + '1';
	var cs1 = "name=" + cs1Name;

	var word = "create collectionspace";
	tryCatch( ["cmd=" + word, cs, cs1], [0], getFuncName() + "Failed to run rest cmd=" + word );
	try
	{
		db.getCS( csName );
	} catch( e )
	{
		println( getFuncName() + 'Faied to get cs: ' + csName );
		throw e;
	}

	try
	{
		var flag = false;
		db.getCS( cs1Name );
		flag = true;
	} catch( e )
	{
		if( e != -34 )
		{
			println( getFuncName() + 'get cs [' + cs1Name + '], expect" error -34, actual: error ' + e );
			throw e;
		}
	}

	if( true == flag )
	{
		throw ( getFuncName() + 'get cs [' + cs1Name + '], expect error -34, actual: error 0' );
	}

	try
	{
		db.dropCS( csName );
	} catch( e )
	{
		println( getFuncName() + 'Faied to drop cs: ' + csName );
		throw e;
	}
}

function lackCmd ()
{
	tryCatch( [cs], [-100], getFuncName() + "Error occurs" );
}

function lackName ()
{
	tryCatch( ["cmd=create collectionspace"], [-6], getFuncName() + "Error occurs" );
}

function lackCmdAndName ()
{
	tryCatch( [""], [-100], getFuncName() + "Error occurs" );
}

function misspell ()
{
	var word = 'command=create collectionspace';
	tryCatch( [word, cs], [-100], "Error occurs in misspell function, " + word );

	word = 'cmd=create collectionspaces';
	tryCatch( [word, cs], [-100], "Error occurs in misspell function, " + word );

	word = 'cmd=create collectionspace';
	var csMisspell = "namename=" + csName;
	tryCatch( [word, csMisspell], [-6], "Error occurs in misspell function, " + csMisspell );
}

function formatError1 ()
{  //"XXXX&a=1=2&XXXX"   a value is 1=2
	var word = "create collectionspace";
	var csNameFormat1 = csName + '=' + csName;
	var csFormat1 = "name=" + csNameFormat1;
	tryCatch( ["cmd=" + word, csFormat1], [0], "Fai to create cs by rest " + csNameFormat1 );
	try
	{
		db.getCS( csNameFormat1 );
	} catch( e )
	{
		println( getFuncName() + 'Faied to get cs: ' + csNameFormat1 );
		throw e;
	}
	try
	{
		db.dropCS( csNameFormat1 );
	} catch( e )
	{
		println( getFuncName() + 'Faied to drop cs: ' + csNameFormat1 );
		throw e;
	}
}

function formatError2 ()
{  //"XXXX&a&b=1"   can't get a value
	var word = "create collectionspace";
	tryCatch( ["cmd=" + word, 'name', 'b=' + csName], [-6], getFuncName() + "error occurs!!" );
}

function formatError3 ()
{  //"XXXX&a==1&XXXX"   a value is =1
	var word = "create collectionspace";
	var csNameFormat3 = '=' + csName;
	tryCatch( ["cmd=" + word, 'name=' + csNameFormat3], [0], getFuncName() + "error occurs!!" );
	try
	{
		db.getCS( csNameFormat3 );
	} catch( e )
	{
		println( getFuncName() + 'Faied to get cs: ' + csNameFormat3 );
		throw e;
	}
	try
	{
		db.dropCS( csNameFormat3 );
	} catch( e )
	{
		println( getFuncName() + 'Faied to drop cs: ' + csNameFormat3 );
		throw e;
	}
}

function formatError4 ()
{  //"XXXX&&&a=1"   a value is 1
	var word = "create collectionspace";
	tryCatch( ["cmd=" + word, '', '', 'name=' + csName], [0], getFuncName() + "Fai to create cs by rest: &&&name=" + csName );
	try
	{
		db.getCS( csName );
	} catch( e )
	{
		println( getFuncName() + 'Faied to get cs: ' + csName );
		throw e;
	}
	try
	{
		db.dropCS( csName );
	} catch( e )
	{
		println( getFuncName() + 'Faied to drop cs: ' + csName );
		throw e;
	}
}

function lackNameDropCS ()
{
	tryCatch( ["cmd=drop collectionspace"], [-6], "Error occurs in lackNameDropCL function" );
}
commDropCS( db, csName, true, "drop cs in begin" );
commDropCS( db, csName + '1', true, "drop cs1 in begin" );
commDropCS( db, csName + '=' + cs, true, "drop cs in begin" );
commDropCS( db, "name==" + cs, true, "drop cs1 in begin" );

twoName();
lackCmd();
lackName();
lackCmdAndName();
misspell();
formatError1();
formatError2();
formatError3();
formatError4();
lackNameDropCS();