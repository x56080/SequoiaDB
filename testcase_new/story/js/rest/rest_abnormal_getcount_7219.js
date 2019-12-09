/****************************************************
@description:	get count, abnormal case
         testlink cases: seqDB-7219
@expectation:	lack [name] expect: return -6
@modify list:
            	2016-3-15 XiaoNi Huang init   2016-3-16 XiaoNi Huang init
****************************************************/
var csName = COMMCSNAME;
var clName = COMMCLNAME;
var cl = "name=" + csName + '.' + clName;
var varCL;

function ready ()
{
	commDropCL( db, csName, clName, true, true, "drop cl in begin" );
	var opt = { ReplSize: 0 };
	varCL = commCreateCLByOption( db, csName, clName, opt, true, false, "create cl in begin" );
	var index = { age: 1 };
	commCreateIndex( varCL, "ageIndex", index, false, false );
}

function lackName ()
{
	tryCatch( ["cmd=get count"],
		[-6], "Error occurs in " + getFuncName() + "lack of name." );
}

try
{
	ready();
	lackName();
}
catch( e )
{
	throw e;
}
finally
{
	commDropCL( db, csName, clName, false, true, "drop cl in clean in finally" );
}