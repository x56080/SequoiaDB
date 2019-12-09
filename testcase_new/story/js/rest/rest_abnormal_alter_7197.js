/****************************************************
@description:	alter collection, abnormal case
         testlink cases: seqDB-7197
@expectation:	lackInAltercl(): lack [name/options] expect: return -6
@modify list:
            	2015-4-7 Ting YU init   2016-3-16 XiaoNi Huang init
****************************************************/
var csName = COMMCSNAME;
var clName = COMMCLNAME;
var cl = "name=" + csName + '.' + clName;

function lackInAltercl ()
{
	var word = 'alter collection';
	//lack of name
	tryCatch( ["cmd=" + word, 'options={ShardingKey:{age:1}}'], [-6], "Error occurs in " + getFuncName() + "; lack of name" );
	//lack of options
	tryCatch( ["cmd=" + word, cl], [-6], "Error occurs in " + getFuncName() + "; lack of options" );
}

if( true == commIsStandalone( db ) )
{
	println( "Mode is standalone!" );
}
else
{
	commDropCL( db, csName, clName, true, true, "drop cl in begin" );
	var opt = { ReplSize: 0 };
	var varCL = commCreateCLByOption( db, csName, clName, opt, true, false, "create cl in begin" );

	lackInAltercl();

	commDropCL( db, csName, clName, false, false, "drop cl in clean" );
}