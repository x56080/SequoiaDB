/****************************************************
@description:	update, normal case
         testlink cases: seqDB-7212
@input:		1 insert {_id:229095,name:"Jack",male:true},{_id:229096,name:"Harry",male:true}
				2 update by rest cmd "updator={$set:{"age":15}}&filter={"name":"Harry"}"
@expectation:	1 cl.count: 2
				2 check Harry rec: add [age] field, not add other field
				3 check Jack rec: don't have change
@modify list:
            	2015-4-3 Ting YU init   2016-3-16 XiaoNi Huang init
****************************************************/
var csName = COMMCSNAME;
var clName = COMMCLNAME;

function insertRecs ()
{
	try
	{
		varCL.insert( [
			{ _id: 229095, name: "Jack", male: true },
			{ _id: 229096, name: "Harry", male: true },
		] );
	} catch( e )
	{
		println( "fail to insert records in begin" );
		throw e;
	}
}

function updateAndCheck ()
{
	var word = "update";
	tryCatch(
		["cmd=" + word, "name=" + csName + '.' + clName, 'updator={$set:{"age":15}}', 'filter={"name":"Harry"}'],
		[0],
		"updateAndCheck: fail to run rest command: " + word );
	/******check count is 2**********/
	try
	{
		var size = varCL.count();
		if( 2 != size )
		{
			throw "rest [cmd=update] change the record number to " + size;
		}
	}
	catch( e )
	{
		throw e;
	}

	/**********check Harry record****************/
	var rc1 = varCL.find( { _id: 229096, name: "Harry", male: true, age: 15 } );
	try
	{
		var size = 0;
		while( rc1.next() )
		{
			size++;
			var fieldNum = 0;
			for( var x in rc1.current().toObj() )
			{
				fieldNum++;
			}
			if( fieldNum != 4 )
			{
				throw "Fail to update record by rest [cmd=update]";
			}
		}
		if( 1 != size )
		{
			throw "Fail to update record by rest [cmd=update]";
		}
	} catch( e )
	{
		throw e;
	}


	/**********check Jack record***************/
	var rc2 = varCL.find( { _id: 229095, name: "Jack", male: true } );
	try
	{
		var size = 0;
		while( rc2.next() )
		{
			size++;
			var fieldNum = 0;
			for( var x in rc2.current().toObj() )
			{
				fieldNum++;
			}
			if( fieldNum != 3 )
			{
				throw "Fail to update record by rest [cmd=update]";
			}
		}
		if( 1 != size )
		{
			throw "Fail to update record by rest [cmd=update]";
		}
	} catch( e )
	{
		throw e;
	}
}

commDropCL( db, csName, clName, true, true, "drop cl in begin" );
var opt = { ReplSize: 0 };
var varCL = commCreateCLByOption( db, csName, clName, opt, true, false, "create cl in begin" );
insertRecs();
updateAndCheck();
commDropCL( db, csName, clName, false, false, "drop cl in clean" );