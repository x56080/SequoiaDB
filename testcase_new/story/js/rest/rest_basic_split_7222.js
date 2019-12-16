/****************************************************
@description:	split, normal case
         testlink cases: seqDB-7222
@input:		insert a rec: {age:1}, 
            split by [splitpercent=50],redundant [splitquery]
@expectation:	
@modify list:
            	2015-4-7 Ting YU init   2016-3-16 XiaoNi Huang init
****************************************************/
var csName = COMMCSNAME;
var clName = COMMCLNAME;
var db = new Sdb( COORDHOSTNAME, COORDSVCNAME );
var sourceGroup;
var targetGroup;

function checkGroupsMorethan1 ()
{
	try
	{
		var dataGroups = commGetGroups( db );
		if( dataGroups.length > 1 )
		{
			return true;
		} else
		{
			return false;
		}
	} catch( e )
	{
		throw e;
	}
}

function insertRecs ()
{
	try
	{
		varCL.insert( { age: 1 } );
	} catch( e )
	{
		println( "fail to insert records in begin" );
		throw e;
	}
}
function getSourceTargetGroup ()
{
	/***********get sourceGroup*****************/
	try
	{
		sourceGroup = commGetCLGroups( db, csName + "." + clName );
		if( sourceGroup.length != 1 )
		{
			throw csName + "." + clName + " is in more than 1 data groups!";
		}
		sourceGroup = sourceGroup[0];
	} catch( e )
	{
		throw e;
	}
	/***********target sourceGroup*****************/
	try
	{
		var dataGroups = commGetGroups( db );

		for( var i = 0; i < dataGroups.length; i++ )
		{
			if( dataGroups[i][0].GroupName != sourceGroup )
			{
				targetGroup = dataGroups[i][0].GroupName;
				break;
			}
		}
		if( undefined == targetGroup ) { throw "Fail to get targetGroup"; }
	} catch( e )
	{
		throw e;
	}
}

function redunSplitquery ()  //redundant [splitquery]
{
	var word = "split";
	tryCatch(
		["cmd=" + word,
		"name=" + csName + '.' + clName,
		'source=' + sourceGroup,
		'target=' + targetGroup,
			'splitpercent=50',
			'splitquery={age:0}'], //redundant
		[0],
		getFuncName() + "fail to run rest cmd=" + word );

	var obj = db.snapshot( 8, { Name: csName + '.' + clName } ).current().toObj().CataInfo;
	for( var i = 0; i < obj.length; i++ )
	{
		switch( obj[i].GroupName )
		{
			case targetGroup:
				for( var p in obj[i]["LowBound"] )
				{
					if( obj[i]["LowBound"][p] != 1 ) { throw "Fail to split by rest cmd=" + word + ",\ntargetGroup LowBound expect: 1, actual: " + obj[i]["LowBound"][p]; }
					break;
				}
				break;
			case sourceGroup:
				for( var p in obj[i]["UpBound"] )
				{
					if( obj[i]["UpBound"][p] != 1 ) { throw "Fail to split by rest cmd=" + word + ",\nsourceGroup UpBound expect: 1, actual: " + obj[i]["UpBound"][p]; }
					break;
				}
				break;
			default:
				throw "cl[" + csName + '.' + clName + "] is in group[" + obj[i].GroupName + "],\nexpect: it is in sourceGroup or targeGroup, actual: targeGroup[" + targetGroup + "] sourceGroup[" + sourceGroup + "]";
		}
	}
}


try
{
	if( true == commIsStandalone( db ) )
	{
		println( "Mode is standalone!" );
	}
	else if( false == checkGroupsMorethan1() )
	{
		println( "data groups number is less than 2! You need at least 2 groups to split!" );
	}
	else
	{
		commDropCL( db, csName, clName, true, true, "drop cl in begin" );
		var opt = { ShardingKey: { age: 1 }, ShardingType: "range", ReplSize: 0 };
		var varCL = commCreateCL( db, csName, clName, opt, true, false, "create cl in begin" );
		insertRecs();
		getSourceTargetGroup();
		redunSplitquery();
		commDropCL( db, csName, clName, false, false, "drop cl in clean" );
	}
}
catch( e )
{
	throw e;
}
finally
{
	commDropCL( db, csName, clName, false, true, "drop cl in clean" );
}