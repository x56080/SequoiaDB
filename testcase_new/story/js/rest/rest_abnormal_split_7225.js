/****************************************************
@description:	split, abnormal case
         testlink cases: seqDB-7225
@input:		create a cl with [Partition:1024]
				split cl by [splitendquery={Partition:128}]
@expectation:	lackSplitquery(): expect: return -6
@modify list:
            	2015-4-7 Ting YU init   2016-3-16 XiaoNi Huang init
****************************************************/
var csName = COMMCSNAME;
var clName = COMMCLNAME;
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

function lackSplitquery ()
{
	var word = "split";
	tryCatch(
		["cmd=" + word,
		"name=" + csName + '.' + clName,
		'source=' + sourceGroup,
		'target=' + targetGroup,
			//		'splitquery={Partition:0}',
			'splitendquery={Partition:128}'],
		[-6],
		"Error occurs in " + getFuncName() );
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
		var opt = { ShardingKey: { age: 1 }, ShardingType: "hash", Partition: 1024, ReplSize: 0 };
		var varCL = commCreateCLByOption( db, csName, clName, opt, true, false, "create cl in begin" );
		getSourceTargetGroup();
		lackSplitquery();
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