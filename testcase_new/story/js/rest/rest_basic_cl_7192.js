/****************************************************
@description:	create cl by special character, abnormal case
         testlink cases: seqDB-7192
@expectation:	create cl which name includes special char by rest, and check by db.cs.getCL() 
@modify list:
            	2015-4-7 Ting YU init   2016-3-16 XiaoNi Huang init
****************************************************/
var csName = COMMCSNAME;
var clBaseName = COMMCLNAME;
var clBase = "name=" + csName + '.' + clBaseName;

var arr = [
	['+', '%2B'],
	[' ', '%20'],
	['/', '%2F'],
	['?', '%3F'],
	['%', '%25'],
	['#', '%23'],
	['&', '%26'],
	['=', '%3D'],
	['!', '%21'],
	['^', '%5E'],
	['`', '%60'],
	['{', '%7B'],
	['}', '%7D'],
	['|', '%7C'],
	['[', '%5B'],
	[']', '%5D'],
	['"', '%22'],
	['<', '%3C'],
	['>', '%3E'],
	['\\', '%5C']
];
function createclBySpeciChar ( p1, p2 )
{
	var clName = clBaseName + p1;  	//be use in db
	var cl = clBase + p2;					//be use in rest

	try
	{
		varCS.dropCL( clName );
	} catch( e )
	{
		if( e != -23 )
		{
			println( 'Faied to drop cl: ' + clName + " in begin!" );
			throw e;
		}
	}

	tryCatch( ["cmd=create collection", cl],
		[0],
		"fail to create cl:" + clName + " by rest command!" );

	try
	{
		varCS.getCL( clName );
	} catch( e )
	{
		println( 'Faied to get cl: ' + clName + " !" );
		throw e;
	}

	try
	{
		varCS.dropCL( clName );
	} catch( e )
	{
		println( 'Faied to drop cl: ' + clName + " in clean!" );
		throw e;
	}

}

try
{
	var varCS = db.getCS( csName );
} catch( e )
{
	println( "fail to getCS:" + csName + " in begin!" );
	throw e;
}

for( var i = 0; i < arr.length; i++ )
{
	createclBySpeciChar( arr[i][0], arr[i][1] );
}