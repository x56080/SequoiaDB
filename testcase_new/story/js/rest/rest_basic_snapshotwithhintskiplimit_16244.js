/******************************************************************************
 * @Description : test snapshot parameters: hint/limit/skip
 *                seqDB-16244:指定hint/limit/skip获取SDB_SNAP_DATABASE快照信息
 * @author      : Fan YU
 * @date        ：2018.10.23
 ******************************************************************************/
main();
function main ()
{
	if( commIsStandalone( db ) )
	{
		println( "run mode is standalone" );
		return;
	}
	//get a group in cluster
	var groupName = getDataGroups( db )[0];
	var nodes = getGroupNodes( db, groupName );
	var nodeNum = nodes.length;
	//the options of snapshot 
	var type = "database";
	var filter = "{RawData:true,GroupName:\"" + groupName + "\"}";
	var selector = "{ NodeName:" + null + ", GroupName:" + null + "}";
	var hint = "{$options:{expand:true}}";
	var limit = nodeNum;
	var skip = 1;
	var returnnum = nodeNum - skip;

	//snapshot with hint/limit/skip
	var actInfo = snapshot( type, filter, selector, hint, limit, skip, returnnum );
	var expInfo = { "num": returnnum, "GroupName": groupName };
	checkResult( expInfo, actInfo );
}

function snapshot ( type, filter, selector, hint, limit, skip, returnnum )
{
	println( "\n---Begin to excute " + getFuncName() );
	var curlPara = ['cmd=snapshot ' + type,
	"filter=" + filter,
	"selector=" + selector,
	"hint=" + hint,
	"limit=" + limit,
	"skip=" + skip,
	"returnnum=" + returnnum
	];
	var expErrno = 0;
	runCurl( curlPara );
	var resp = infoSplit;
	var actErrno = JSON.parse( resp[0] ).errno;
	if( expErrno != actErrno )
	{
		throw "get database by snapshot failed,info = " + infoSplit.toString();
	}
	resp.shift();
	return resp;
}

function checkResult ( expInfo, actInfo )
{
	println( "\n---Begin to excute " + getFuncName() );
	var exp = expInfo;
	if( exp.num != actInfo.length )
	{
		throw "expNum is not equal to actNum,expInfo = " + expInfo + ",actInfo = " + actInfo;
	}
	for( var i = 0; i < actInfo.length; i++ )
	{
		var act = JSON.parse( actInfo[i] );
		if( exp.GroupName != act.GroupName )
		{
			throw "exp.GroupName is not equal to act.GroupName,expInfo = " + expInfo + ",actInfo = " + actInfo;
		}
	}
}

function getDataGroups ( db )
{
	println( "\n---Begin to excute " + getFuncName() );
	var groups = [];
	if( commIsStandalone( db ) )
	{
		return groups;
	}
	var cursor = db.listReplicaGroups();
	var tmpInfo;
	while( tmpInfo = cursor.next() )
	{
		var groupName = tmpInfo.toObj().GroupName;
		if( groupName == "SYSCoord" || groupName == "SYSCatalogGroup" )
			continue;
		groups.push( groupName );
	}
	return groups;
}

function getGroupNodes ( db, rgName )
{
	println( "\n---Begin to excute " + getFuncName() );
	var nodes = [];
	if( commIsStandalone( db ) )
	{
		return nodes;
	}
	var tmpObj = db.getRG( rgName ).getDetail().next().toObj();
	var tmpGroupArray = tmpObj["Group"];
	for( var i = 0; i < tmpGroupArray.length; i++ )
	{
		var tmpNodeObj = tmpGroupArray[i];
		var nodename = tmpNodeObj["HostName"];
		for( var j = 0; j < tmpNodeObj.Service.length; j++ )
		{
			var tmpSvcObj = tmpNodeObj.Service[j];
			if( tmpSvcObj["Type"] == 0 )
			{
				nodename = tmpSvcObj["Name"];
				nodes.push( nodename );
				break;
			}
		}
	}
	return nodes;
}

