/**************************************
 * @Description: 主子表使用聚集查询
 * @author: ouyangzhongnan 
 * @coverTestcace: 
 *       seqDB-10463:主子表使用聚集查询
 * @RunDemo:
 * /opt/sequoiadb/bin/sdb -f "func.js,commlib.js,subCL_queryByAggregate_10463.js" -e "var CHANGEDPREFIX='prefix';var COORDHOSTNAME='sdbserver01';var COORDSVCNAME='11810'"
 **************************************/


var mainCl = null;
var subCls = [];
var data = [];
var incomeSumByBJ = 0;
var incomeSumBySH = 0;
var incomeSumByGZ = 0;
var mainClName = CHANGEDPREFIX + "maincl_10463";
var subClNames = [
	CHANGEDPREFIX + "subcl_10463_0",
	CHANGEDPREFIX + "subcl_10463_1",
];

/**
 * [main:入口函数]
 */
main();
function main ()
{

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
		throw e;
	}

	init();
	initData();
	queryByAggregate();
	end();
}


function init ()
{

	//drop subcl and maincl
	commDropCL( db, COMMCSNAME, subClNames[0], true, true, "clean sub collection" );
	commDropCL( db, COMMCSNAME, subClNames[1], true, true, "clean sub collection" );
	commDropCL( db, COMMCSNAME, mainClName, true, true, "clean main collection" );
	println( "LOG:clean the environment" );

	//create maincl and subcl
	db.setSessionAttr( { PreferedInstance: "M" } );
	var mainCLOption = { IsMainCL: true, ShardingKey: { a: 1 }, ShardingType: "range", ReplSize: 0, Compressed: true };
	mainCl = commCreateCL( db, COMMCSNAME, mainClName, mainCLOption, true, true );
	subCls.push( commCreateCL( db, COMMCSNAME, subClNames[0] ) );
	subCls.push( commCreateCL( db, COMMCSNAME, subClNames[1], { ReplSize: 0, Compressed: true, ShardingKey: { a: 1 }, ShardingType: "range" }, true, true ) );
	//attach subcl
	attachCL( mainCl, COMMCSNAME + "." + subClNames[0], { LowBound: { a: 0 }, UpBound: { a: 100 } } );
	attachCL( mainCl, COMMCSNAME + "." + subClNames[1], { LowBound: { a: 100 }, UpBound: { a: 200 } } );
	println( "LOG:create maincl and subcl,attach subcl to maincl" );
}

function end ()
{
	//drop subcl and maincl
	commDropCL( db, COMMCSNAME, subClNames[0], true, true, "clean sub collection" );
	commDropCL( db, COMMCSNAME, subClNames[1], true, true, "clean sub collection" );
	commDropCL( db, COMMCSNAME, mainClName, true, true, "clean main collection" );
	println( "LOG:clean the environment" );
}

function initData ()
{
	var vIncome = 0;
	var vCity = "";
	for( var i = 0; i < 200; i++ )
	{
		var flag = i % 6;
		vIncome = parseInt( Math.random() * 10000 );

		//总记录数中各个城市的总数不一样
		switch( flag )
		{
			case 0:
				vCity = "广州";
				incomeSumByGZ += vIncome;
				break;
			case 1:
			case 2:
				vCity = "上海";
				incomeSumBySH += vIncome;
				break;
			case 3:
			case 4:
			case 5:
				vCity = "北京";
				incomeSumByBJ += vIncome;
				break;
		}

		data.push( { name: "name" + i, income: vIncome, city: vCity, a: i } );
	}
	// println( "北京 "+ incomeSumByBJ);
	// println( "上海 "+ incomeSumBySH);
	// println( "广州 "+ incomeSumByGZ);
	try
	{
		mainCl.insert( data );
		//split cl
		ClSplitOneTimes( COMMCSNAME, subClNames[1], 50, null );
	} catch( e )
	{
		throw buildException( "failed to initData", e );
	}
	println( "LOG:init 200 records data for mainCl" );
}

/**
 * 使用聚集查询每个城市的收入总和
 * @return {[type]} [description]
 */
function queryByAggregate ()
{
	var flag = true;
	var cursor = mainCl.aggregate( { $group: { _id: "$city", income: { "$sum": "$income" }, city: { "$first": "$city" } } } ).toArray();
	for( var i = 0; i < cursor.length; i++ )
	{
		var res = JSON.parse( cursor[i] );
		switch( res.city )
		{
			case "北京":
				// println(res.city +" "+ res.income);
				if( res.income !== incomeSumByBJ ) flag = false;
				break;
			case "上海":
				// println(res.city +" "+ res.income);
				if( res.income !== incomeSumBySH ) flag = false;
				break;
			case "广州":
				// println(res.city +" "+ res.income);
				if( res.income !== incomeSumByGZ ) flag = false;
				break;
		}
	}

	if( !flag )
	{
		println( "queryByAggregate ERROR" );
		throw buildException( "queryByAggregate", new Error(), "通过分组查询，查询每个城市的收入总和", "查询结果与插入一致", "查询结果与插入不一致" );
	}
	println( "queryByAggregate SUCCED" );
}