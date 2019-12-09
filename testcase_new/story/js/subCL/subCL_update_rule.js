function getSourceGroupName_alone ( COMMCSNAME, CL_Name )
{
	var cata = new Sdb( COORDHOSTNAME, CATASVCNAME );
	var allCollections = cata.SYSCAT.SYSCOLLECTIONS.find().toArray();
	var CS_CL = COMMCSNAME + "." + CL_Name;
	var GroupName = "";
	for( var i = 0; i < allCollections.length; i++ )
	{
		var eval_CL = eval( "(" + allCollections[i] + ")" );
		if( eval_CL["Name"] == CS_CL )
		{
			println( eval_CL["Name"] );
			/*for(var j=0;j<eval_CL["CataInfo"].length;j++)
			{
				GroupName = eval_CL["CataInfo"][j]["GroupName"] ;
			}*/
			GroupName = eval_CL["CataInfo"][0]["GroupName"];
			break;
		}
	}
	return GroupName;
}
function getOtherDataGroups ( SourceGroupName )
{
	var allGroups = db.listReplicaGroups().toArray();
	var RoleGroupNumbers = 0;
	var Groups = [];
	for( var i = 0; i < allGroups.length; i++ )
	{
		var eval_node = eval( "(" + allGroups[i] + ")" );
		if( eval_node["Role"] == 0 )
		{
			if( eval_node["GroupName"] != SourceGroupName )
			{
				Groups.push( eval_node["GroupName"] );
			}
		}
	}
	return Groups;
}

function getPartition ( COMMCSNAME, CL_Name )
{
	var cata = new Sdb( COORDHOSTNAME, CATASVCNAME );
	var allCollections = cata.SYSCAT.SYSCOLLECTIONS.find().toArray();
	var CS_CL = COMMCSNAME + "." + CL_Name;
	var Partition = "";
	for( var i = 0; i < allCollections.length; i++ )
	{
		var eval_CL = eval( "(" + allCollections[i] + ")" );
		if( eval_CL["Name"] == CS_CL )
		{
			Partition = eval_CL["Partition"];
			break;
		}
	}
	return Partition;
}
function subCL_split_hash ( subcl, SourceGroupName, OtherDataGroups, Partition )
{
	var Partition_PerGroup = Partition / ( OtherDataGroups.length + 1 );
	for( var i = 0; i < OtherDataGroups.length; ++i )
	{
		var start_Partition = Math.round( Partition_PerGroup * i );
		var end_Partition = Math.round( Partition_PerGroup * ( i + 1 ) );
		println( start_Partition + '~~~~~~~~~~~~~~~~' + end_Partition );
		try
		{
			subcl.split( SourceGroupName, OtherDataGroups[i], { Partition: start_Partition }, { Partition: end_Partition } );
		}
		catch( e )
		{
			println( "can't split : " + e );
			return -1
		}
	}
	return 0;
}


function test_range_attach_hash_update_1 ()// NOT Error, test mainCL'ShardingType is range and subCL's ShardingType is hash , insert's result
{
	MainCL_Name = CHANGEDPREFIX + "year";
	subCl_Name = CHANGEDPREFIX + "month";
	try
	{
		commDropCL( db, COMMCSNAME, subCl_Name + "1", true, true,
			"clean sub collection" );
		commDropCL( db, COMMCSNAME, subCl_Name + "2", true, true,
			"clean sub collection" );
		commDropCL( db, COMMCSNAME, MainCL_Name, true, true,
			"clean main collection" );
	}
	catch( e )
	{
		println( "failed to drop main and sub cl, rc = " + e );
		throw e;
	}
	try
	{
		var cs = commCreateCS( db, COMMCSNAME, true, "create cs in the beginning" );
		var mainCL = cs.createCL( MainCL_Name, { ShardingKey: { a: 1 }, ShardingType: "range", Partition: 4096, ReplSize: 0, Compressed: true, IsMainCL: true } );
		println( "mainCL" );
		var subCL1 = cs.createCL( subCl_Name + "1", { ShardingKey: { a: 1 }, ShardingType: "hash", ReplSize: 0, Compressed: true, IsMainCL: false } );
		println( "subCL1" );
		var subCL2 = cs.createCL( subCl_Name + "2", { ShardingKey: { a: 1 }, ShardingType: "hash", ReplSize: 0, Compressed: true, IsMainCL: false } );
		println( "subCL2" );
		mainCL.attachCL( COMMCSNAME + "." + subCl_Name + "1", { LowBound: { a: 0 }, UpBound: { a: 1 } } );
		println( "attach subCL1" );
		mainCL.attachCL( COMMCSNAME + "." + subCl_Name + "2", { LowBound: { a: 1 }, UpBound: { a: 2 } } );
		println( "attach subCL2" );
	}
	catch( e )
	{
		throw e;
	}

	try
	{
		var subCL = [];
		subCL.push( subCL1 );
		subCL.push( subCL2 );
		var numberOfsubCl = 2;
		for( var i = 0; i < numberOfsubCl; ++i )
		{
			var sourceDataGroupName = getSourceGroupName_alone( COMMCSNAME, subCl_Name + ( i + 1 ) );
			println( "sourceDataGroupName is : " + sourceDataGroupName );

			var desDataGroupName = getOtherDataGroups( sourceDataGroupName );
			println( "desDataGroupName is " + desDataGroupName );

			var Partition = getPartition( COMMCSNAME, subCl_Name + ( i + 1 ) );
			println( "Partition is : " + Partition );

			if( !subCL_split_hash( subCL[i], sourceDataGroupName, desDataGroupName, Partition ) )
			{
				println( "************SPLIT SUCCED***************" );
			}
		}
	}
	catch( e )
	{
		println( " Error: " + e );
		throw e;
	}

	try
	{
		mainCL.insert( { a: 0, b: [1, 2], salary: 100 } );
		mainCL.insert( { a: 1, b: [1, 2], salary: 100 } );
	}
	catch( e )
	{
		println( "insert data err is :" + e );
	}

	var res = false;
	try
	{
		mainCL.update( { $addtoset: { c: 2 } } );
	}
	catch( e )
	{
		if( e == -6 )
		{
			res = true;
		}
		else
		{
			println( "Update {$addtoset:{c:2}} failed: " + e );
		}
	}
	if( !res )
	{
		println( "mainCL.update( {$addtoset:{c:2}})" );
		throw -1
	}

	var res1 = true;
	try
	{
		mainCL.update( { $pull: { "b.0": 1 } } );
	}
	catch( e )
	{
		res1 = false;
		println( "Update {$pull:{b.0:1}} failed: " + e );
	}
	if( !res1 )
	{
		println( "mainCL.update( {$pull:{b.0:1}})" );
		throw -1
	}

	var res2 = true;
	try
	{
		mainCL.update( { $push: { salary: 1 } } );
	}
	catch( e )
	{
		res2 = false;
		println( "Update {$push:{salary:1}} failed: " + e );
	}
	if( !res2 )
	{
		println( "mainCL.update( {$push:{salary:1}})" );
		throw -1
	}

	var res3 = false;
	try
	{
		mainCL.update( { $pull_all: { b: 3 } } );
	}
	catch( e )
	{
		if( e == -6 )
		{
			res3 = true;
		}
		else
		{
			println( "Update {$pull_all:{b:3}} failed: " + e );
		}
	}
	if( !res3 )
	{
		println( "mainCL.update( {$pull_all:{b:3}})" );
		throw -1
	}

	var res4 = false;
	try
	{
		mainCL.update( { $push_all: { b: 2 } } );
	}
	catch( e )
	{
		if( e == -6 )
		{
			res4 = true;
		}
		else
		{
			println( "Update {$push_all:{b:2}} failed: " + e );
		}
	}
	if( !res4 )
	{
		println( "mainCL.update( {$push_all:{b:2}}) " );
		throw -1
	}

	var res5 = false;
	try
	{
		mainCL.update( { $pop: { b: [2] } } );
	}
	catch( e )
	{
		if( e == -6 )
		{
			res5 = true;
		}
		else
		{
			println( "Update {$pop:{b:[2]}} failed: " + e );
		}
	}
	if( !res5 )
	{
		println( "mainCL.update( {$pop:{b:[2]}})" );
		throw -1
	}
	//	println( "mainCL.find({a:0})" ) ;
	//	println( mainCL.find({a:0}) ) ;
	//	println( "mainCL.find({a:1})" ) ;
	//	println( mainCL.find({a:1}) ) ;
	//	
	//	println( "subCL1.find({a:0})" ) ;
	//	println( subCL1.find({a:0}) ) ;
	//	println( "subCL1.find({a:1})" ) ;
	//	println( subCL1.find({a:1}) ) ;
	//	
	//	println( "subCL2.find({a:0})" ) ;
	//	println( subCL2.find({a:0}) ) ;
	//	println( "subCL2.find({a:1})" ) ;
	//	println( subCL2.find({a:1}) ) ;
}

function main ()
{
	//set priority from masterNode
	db.setSessionAttr( { PreferedInstance: "M" } );

	try
	{
		db.listReplicaGroups();
	}
	catch( e )
	{
		if( e == -159 )
		{
			println( "can't run in standalone" );
			return;
		}
		println( "fail to check standalone" );
		throw e;
	}
	println( "test_range_attach_hash_update_1 is start" );
	test_range_attach_hash_update_1();
	println( "test_range_attach_hash_update_1 is end" );
	println();

}

main();


