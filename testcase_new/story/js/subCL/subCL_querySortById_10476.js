/**************************************
 * @Description: 查询并指定_id字段排序
 * @author: ouyangzhongnan 
 * @Date: 2016-11-22
 * @coverTestcace: 
 *       seqDB-10476:查询并指定_id字段排序
 * @RunDemo:
 * /opt/sequoiadb/bin/sdb -f "func.js,commlib.js,subCL_querySortById_10476.js" -e "var CHANGEDPREFIX='prefix';var COORDHOSTNAME='sdbserver01';var COORDSVCNAME='11810'"
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
	var mainClName = CHANGEDPREFIX + "maincl_10476";
	var subClNames = [
		CHANGEDPREFIX + "subcl_10476_0",
		CHANGEDPREFIX + "subcl_10476_1"
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
	var subCLOptions = [
		{ ReplSize: 0, Compressed: true },
		{ ReplSize: 0, Compressed: true, ShardingKey: { a: 1 }, ShardingType: "hash", Partition: 16 }
	];
	//create subcl
	subCls.push( commCreateCLByOption( db, COMMCSNAME, subClNames[0], subCLOptions[0], true, true ) );
	subCls.push( commCreateCLByOption( db, COMMCSNAME, subClNames[1], subCLOptions[1], true, true ) );
	//attach subcl
	attachCL( mainCl, COMMCSNAME + "." + subClNames[0], { LowBound: { a: 0 }, UpBound: { a: 100 } } );
	attachCL( mainCl, COMMCSNAME + "." + subClNames[1], { LowBound: { a: 100 }, UpBound: { a: 200 } } );

	//init data
	try
	{
		for( var i = 0; i < 200; i++ )
		{
			data.push( { _id: i, name: "name_" + i, a: i } );
		}
		mainCl.insert( data );
		//split cl
		ClSplitOneTimes( COMMCSNAME, subClNames[1], 50, null );
	} catch( e )
	{
		throw buildException( "failed to init 200 records data for mainCl", e );
	}

	//查询按_id正序
	var sortCond = { _id: 1 };
	var cursor = mainCl.find().sort( sortCond );
	if( cursor.count() != data.length )
	{
		flag_0 = false;
	} else
	{
		var compareObj = cursor.next().toObj();
		for( var i = 1; i < cursor.count(); i++ )
		{
			var toCompareObj = cursor.next().toObj();
			//println(JSON.stringify(toCompareObj));
			if( compareJSONObj( compareObj, toCompareObj, sortCond ) < 0 )
			{
				throw buildException( "query sort by id asc test", new Error(), "query and sort by _id asc", "sort asc", "sort not asc" );
			}
			compareObj = toCompareObj;
		}
	}

	//按_id逆序
	var sortCond = { _id: -1 };
	var cursor = mainCl.find().sort( sortCond );
	var flag_1 = true;
	if( cursor.count() != data.length )
	{
		flag_1 = false;
	} else
	{
		var compareObj = cursor.next().toObj();
		for( var i = 1; i < cursor.count(); i++ )
		{
			var toCompareObj = cursor.next().toObj();
			//println(JSON.stringify(toCompareObj));
			if( compareJSONObj( compareObj, toCompareObj, sortCond ) < 0 )
			{
				throw buildException( "query sort by id desc test", new Error(), "query and sort by _id desc", "sort desc", "sort not desc" );
			}
			compareObj = toCompareObj;
		}
	}

	//drop subcl and maincl
	commDropCL( db, COMMCSNAME, subClNames[0], true, true, "clean sub collection" );
	commDropCL( db, COMMCSNAME, subClNames[1], true, true, "clean sub collection" );
	commDropCL( db, COMMCSNAME, mainClName, true, true, "clean main collection" );
}
