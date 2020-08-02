/******************************************************************************
*@Description : 1.SEQUOIADBMAINSTREAM-272
*               fixed: SVN 13586
*@Modify list :
*               2014-07-17 pusheng Ding  Init
******************************************************************************/
   indexName = CHANGEDPREFIX + "idx";
   commDropCL( db, csName, clName );
   var varCL = commCreateCL( db, csName, clName );

//create index
try
{
	varCL.createIndex( indexName, { a: 1 } );
	println( "create index finished" );
} catch( e )
{
	//when redefine index, ignore the exception
	if( e != -247 )
	{
		println( "can't create index:" + indexName + " rc=" + e );
		throw e;
	}
	println( "already exist index:" + indexName );
}


//insert data
try
{
	for( var i = 0; i < 2621 * 5; i += 5 ) { varCL.insert( { a: i } ); }
} catch( e )
{
	println( "insert data fail! rc=" + e );
	throw e;
}
println( "insert data finished" );

//select * from ... where a!=11780
try
{
	var sel = varCL.find( { a: { $ne: 11780 } } );
	var size = 0;
	var flag = true;
	while( sel.next() )
	{
		size++;
		if( size > 2621 )
		{
			flag = false;
			throw 1;
		}
		var ret = sel.current();
		if( ret.toObj()['a'] == 11780 )
		{
			flag = false;
			throw 2;
		}
	}
	sel.close();
	if( flag && size != 2620 )
	{
		flag = false;
		throw 1;
	}
} catch( e )
{
	if( e != 1 && e != 2 )
	{
		println( "select data fail! rc=" + e );
		throw e;
	}
	else if( e == 1 )
	{
		println( "return rows not expected! expected:2620 return:" + size + ( size > 2621 ? " or more" : "" ) );
		throw e;
	}
	else if( e == 2 )
	{
		println( "return incorrect record! " + ret );
		throw e;
	}
}
println( "select data finished!" );

//clean test-env
try
{
	varCL.dropIndex( indexName );
	commDropCL( db, csName, clName, false, false,
		"drop colleciton in the end" );
} catch( e )
{
	println( "clean test-evn fail! rc=" + e );
	throw e;
}
println( "clean test-evn succ!" );
