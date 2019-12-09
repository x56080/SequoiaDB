/**************************************
 * @Description: 修改子表属性，主子表数据操作正常
 * @author: ouyangzhongnan 
 * @coverTestcace: 
 *       seqDB-10480:修改子表属性，主子表数据操作正常
 * @RunDemo:
 * /opt/sequoiadb/bin/sdb -f "func.js,commlib.js,subCL_updateSubAttr_CRUD_10480.js" -e "var CHANGEDPREFIX='prefix';var COORDHOSTNAME='sdbserver01';var COORDSVCNAME='11810'"
 **************************************/


/**
 * [main:入口函数]
 */
main();
function main ()
{
	var mainCl = null;
	var subCls = [];
	var data = [];
	var mainClName = CHANGEDPREFIX + "maincl_10480";
	var subClNames = [
		CHANGEDPREFIX + "subcl_10480_0",
		CHANGEDPREFIX + "subcl_10480_1"
	];
	//check test environment before split
	try
	{
		//standalone can not split
		if( true == commIsStandalone( db ) )
		{
			println( "run mode is standalone" );
			return;
		}
		//less two groups,can not split
		allGroupName = getGroupName( db );
		if( 1 === allGroupName.length )
		{
			println( "--least two groups" );
			return;
		}
	} catch( e )
	{
		throw buildException( "failed init environment", e );
	}

	//drop subcl and maincl
	commDropCL( db, COMMCSNAME, subClNames[0], true, true, "clean sub collection" );
	commDropCL( db, COMMCSNAME, subClNames[1], true, true, "clean sub collection" );
	commDropCL( db, COMMCSNAME, mainClName, true, true, "clean main collection" );
	//create maincl for range split
	db.setSessionAttr( { PreferedInstance: "M" } );
	var mainCLOption = { IsMainCL: true, ShardingKey: { a: 1 }, ShardingType: "range", ReplSize: 0, Compressed: true };
	mainCl = commCreateCLByOption( db, COMMCSNAME, mainClName, mainCLOption, true, true );
	//create subcl
	subCls.push( commCreateCL( db, COMMCSNAME, subClNames[0], 0 ) );
	subCls.push( commCreateCL( db, COMMCSNAME, subClNames[1], 0 ) );
	//attach subcl
	attachCL( mainCl, COMMCSNAME + "." + subClNames[0], { LowBound: { a: 0 }, UpBound: { a: 100 } } );
	attachCL( mainCl, COMMCSNAME + "." + subClNames[1], { LowBound: { a: 100 }, UpBound: { a: 200 } } );

	//init data
	try
	{
		for( var i = 0; i < 200; i++ )
		{
			if( i === 50 || i === 150 ) continue;
			data.push( { name: "name_" + i, a: i } );
		}
		mainCl.insert( data );
	} catch( e )
	{
		throw buildException( "failed to init 200 records data for mainCl", e );
	}

	//update subcl attr test
	try
	{
		subCls[0].alter( { ShardingKey: { a: 1 }, ShardingType: "hash", Partition: 16 } );
		subCls[1].alter( { ShardingKey: { a: 1 }, ShardingType: "range" } );
		//split cl
		ClSplitOneTimes( COMMCSNAME, subClNames[0], 50, null );
		ClSplitOneTimes( COMMCSNAME, subClNames[1], 50, null );
	} catch( e )
	{
		println( "failed to updata subCL attribute, rc = " + e );
		throw buildException( "failed to updata subCL attribute", e );
	}

	//insert test
	var newData = [{ name: "newNameA50", a: 50 }, { name: "newNameA150", a: 150 }];
	mainCl.insert( newData );
	var cursor_A50 = mainCl.find( { a: 50 } );
	var cursor_A150 = mainCl.find( { a: 150 } );
	if( cursor_A50.count() != 1 && cursor_A150.count() != 1 )
	{
		throw buildException( "insert failed", new Error(), "update subCL attribute, then insert", "insert success", "insert failed" );
	}

	//update test
	mainCl.update( { $set: { name: "newNameA50_update" } }, { a: 50 } );
	mainCl.update( { $set: { name: "newNameA150_update" } }, { a: 150 } );
	var cursor_A50 = mainCl.find( { a: 50 } );
	var cursor_A150 = mainCl.find( { a: 150 } );
	if( cursor_A50.next().toObj().name !== "newNameA50_update" || cursor_A150.next().toObj().name !== "newNameA150_update" )
	{
		throw buildException( "update failed", new Error(), "update subCL attribute, then update", "update success", "update failed" );
	}

	//delete test
	mainCl.remove( { a: 50 } );
	mainCl.remove( { a: 150 } );
	var cursor_A50 = mainCl.find( { a: 50 } );
	var cursor_A150 = mainCl.find( { a: 150 } );
	if( cursor_A50.count() == 1 || cursor_A150.count() == 1 )
	{
		throw buildException( "update failed", new Error(), "update subCL attribute, then delete", "delete success", "delete failed" );
	}

	//drop subcl and maincl
	commDropCL( db, COMMCSNAME, subClNames[0], true, true, "clean sub collection" );
	commDropCL( db, COMMCSNAME, subClNames[1], true, true, "clean sub collection" );
	commDropCL( db, COMMCSNAME, mainClName, true, true, "clean main collection" );
}