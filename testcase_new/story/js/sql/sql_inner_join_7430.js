/****************************************************
@description:	inner join, basic case
         testlink cases:   seqDB-7430
@input:        1 insert into records
               2 select with [inner join, order by g.Cno]
@output:    return success, and results correct.
@modify list:
      2015-5-13 ShanShan Hu added  2016-3-16 XiaoNi Huang modify
****************************************************/
csName = COMMCSNAME;
clName1 = CHANGEDPREFIX + "_student";
clName2 = CHANGEDPREFIX + "_grade";

println( "------Begin to ready cl." );
try
{
	// db.execUpdate("create collection "+csName+"."+clName);
	commDropCL( db, csName, clName1, true, true, "drop cl in begin" );
	commDropCL( db, csName, clName2, true, true, "drop cl in begin" );
	var opt = { ReplSize: 0 };
	var cl_pro = commCreateCLByOption( db, csName, clName1, opt, true, false, "create cl in begin" );
	var cl_name = commCreateCLByOption( db, csName, clName2, opt, true, false, "create cl in begin" );
}
catch( e )
{
	println( "Failed to drop/create cl in the begin." );
	throw e;
}

println( "------Begin to insert into records." );
var name = new Array( "Tom", "Mike", "John", "Lily", "Lucy" );
var dept = new Array( "computer", "physics", "mathematics", "chemistry" );

//student
var _dept;
for( var i = 0; i < name.length; i++ )
{
	if( i == 0 || i == 1 ) _dept = dept[0];
	else _dept = dept[i - 1];
	//db.execUpdate("insert into "+csName+"."+clName1+"(Sno,Sname) values"+"("+i+",\""+name[i]+"\")");
	try
	{
		db.execUpdate( "insert into " + csName + "." + clName1 + "(Sno,Sname,Sdept) values" + "(" + i + ",\"" + name[i] + "\",\"" + _dept + "\")" );
	}
	catch( e )
	{
		println( "Failed to insert into " + csName + "." + clName1 );
		throw e;
	}
}

//grade
var cno = new Array( "1001", "1002", "1003", "1004", "1005", "1006", "1007" );
var grade = new Array( "96", "85", "90", "89", "86", "83", "95" );
for( var j = 0; j < cno.length; j++ )
{
	if( j % 2 == 0 ) sno = j;
	else sno = 1;
	try
	{
		db.execUpdate( "insert into " + csName + "." + clName2 + "(Sno,Cno,Grade) values" + "(" + sno + "," + cno[j] + "," + grade[j] + ")" );
	}
	catch( e )
	{
		println( "Failed to insert into " + csName + "." + clName2 );
		throw e;
	}
}

println( "------Begin to exec select with [inner join, order by g.Cno]." );
try
{
	var cur = db.exec( "select s.Sname,s.Sdept,g.Cno,g.Grade from " + csName + "." + clName1 + " as s" + " inner join " + csName + "." + clName2 + " as g" + " on s.Sno=g.Sno order by g.Cno" );
}
catch( e )
{
	println( "Failed to exec select." );
	throw e;
}

println( "------Begin to check results." )
var cno = new Array();
var grade = new Array();
var sname = new Array();
var sdept = new Array();
while( cur.next() )
{
	sname[i] = cur.current().toObj()["Sname"];
	grade[i] = cur.current().toObj()["Grade"];
	cno[i] = cur.current().toObj()["Cno"];
	sdept[i] = cur.current().toObj()["Sdept"];
	if( cno[i] == null || grade[i] == null
		|| sdept[i] == null || sname[i] == null )
	{
		throw "Failed to compare results.";
	}
}

println( "------Begin to drop cl in the end." );
try
{
	db.execUpdate( "drop collection " + csName + "." + clName1 );
	db.execUpdate( "drop collection " + csName + "." + clName2 );
}
catch( e )
{
	println( "Failed to drop cl in the end." );
	throw e;
}