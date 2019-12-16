/****************************************************
@description:	select with [like] by SQL, basic case
         testlink cases:   seqDB-7429
@input:        1 insert into records
               2 select with [like 'p']
               3 select with [like '^b']
               4 select with [like 'ia$']
               5 select with [like 'ia*']
               6 select with [like '^b.*[st].*n$']
               7 select with [like '[^a-t]']
               8 select with [like '(an+|s{2})']
@output:    return success, and results correct.
@modify list:
      2015-5-13 ShanShan Hu added  2016-3-16 XiaoNi Huang modify
****************************************************/
csName = COMMCSNAME;
clName = CHANGEDPREFIX + "_bar";

println( "------Begin to ready cl." );
try
{
	// db.execUpdate("create collection "+csName+"."+clName);
	commDropCL( db, csName, clName, true, true, "drop cl in begin" );
	var opt = { ReplSize: 0 };
	var varCL = commCreateCL( db, csName, clName, opt, true, false, "create cl in begin" );
}
catch( e )
{
	println( "Failed to drop/create cl in the begin." );
	throw e;
}

println( "------Begin to insert into records." );
try
{
	varCL.insert( { country: "china" } );
	varCL.insert( { country: "USA" } );
	varCL.insert( { country: "india" } );
	varCL.insert( { country: "japan" } );
	varCL.insert( { country: "korea" } );
	varCL.insert( { country: "britain" } );
	varCL.insert( { country: "germany" } );
	varCL.insert( { country: "france" } );
	varCL.insert( { country: "spain" } );
	varCL.insert( { country: "russia" } );
}
catch( e )
{
	println( "Failed to insert records." );
	throw e;
}

println( "------Begin to select with [like '\p']." );
try
{
	var rc = db.exec( "select * from " + csName + "." + clName + " where country like \"\p\"" );
}
catch( e )
{
	println( "Failed to select with [like '\p']" );
	throw e;
}

println( "------Begin to check results." );
var record = new Array();
var i = 0;
while( rc.next() )
{
	i++;
	record.push( rc.current().toObj()["country"] );
}
if( record[0] !== "japan" || record[1] !== "spain" )
{
	throw "Failed to compare records fields.";
}
if( 2 !== i )
{
	throw "Failed to compare records count.";
}

println( "------Begin to select with [like '^b']." );
try
{
	var rc = db.exec( "select * from " + csName + "." + clName + " where country like \"^b\"" );
}
catch( e )
{
	println( "Failed to select with [like '^b']." );
	throw e;
}

println( "------Begin to check results." );
var i = 0;
var record = new Array();
while( rc.next() )
{
	i++;
	record.push( rc.current().toObj()["country"] );
}

if( record[0] !== "britain" )
{
	throw "Failed to compare records fields.";
}
if( 1 !== i )
{
	throw "Failed to compare records count.";
}

println( "------Begin to select with [like 'ia$']." );
try
{
	var rc = db.exec( "select * from " + csName + "." + clName + " where country like \"\ia$\" order by country" );
}
catch( e )
{
	println( "failed to read record with the expression \ia$\ e=" + e );
	throw e;
}

println( "------Begin to check results." );
var i = 0;
var record = new Array();
while( rc.next() )
{
	i++;
	record.push( rc.current().toObj()["country"] );
}
if( record[0] !== "india" || record[1] !== "russia" )
{
	throw "Failed to compare records fields.";
}
if( 2 !== i )
{
	throw "Failed to compare records count.";
}

println( "------Begin to select with [like 'ia*']." );
try
{
	var rc = db.exec( "select * from " + csName + "." + clName + " where country like \"ia*\" order by country" );
}
catch( e )
{
	println( "Failed to select with [like 'ia*']." );
	throw e;
}

println( "------Begin to check results." );
var i = 0;
var record = new Array();
while( rc.next() )
{
	i++;
	record.push( rc.current().toObj()["country"] );
}
if( record[0] !== "britain" || record[1] !== "china" || record[2] !== "india"
	|| record[3] !== "russia" || record[4] !== "spain" )
{
	throw "Failed to compare records fields.";
}
if( 5 !== i )
{
	throw "Failed to compare records count.";
}

println( "------Begin to select with [like '^b.*[st].*n$']." );
try
{
	var rc = db.exec( "select * from " + csName + "." + clName + " where country like \"^b.*[st].*n$\"" );
}
catch( e )
{
	println( "Failed to select with [like '^b.*[st].*n$']." );
	throw e;
}

println( "------Begin to check results." );
var i = 0;
var record = new Array();
while( rc.next() )
{
	i++;
	record.push( rc.current().toObj()["country"] );
}

if( record[0] !== "britain" )
{
	throw "Failed to compare records fields.";
}
if( 1 !== i )
{
	throw "Failed to compare records count.";
}

println( "------Begin to select with [like '[^a-t]']." );
try
{
	var rc = db.exec( "select * from " + csName + "." + clName + " where country like \"[^a-t]\" order by country" );
}
catch( e )
{
	println( "Failed to select with [like '[^a-t]']." );
	throw e;
}

println( "------Begin to check results." );
var i = 0;
var record = new Array();
while( rc.next() )
{
	i++;
	record.push( rc.current().toObj()["country"] );
}

if( record[0] !== "USA" || record[1] !== "germany" || record[2] !== "russia" )
{
	throw "Failed to compare records fields.";
}
if( 3 !== i )
{
	throw "Failed to compare records count.";
}

println( "------Begin to select with [like '(an+|s{2})']." );
try
{
	var rc = db.exec( "select * from " + csName + "." + clName + " where country like \"(an+|s{2})\" order by country" );
}
catch( e )
{
	println( "Failed to select with [like '(an+|s{2})']." );
	throw e;
}

println( "------Begin to check results." );
var i = 0;
var record = new Array();
while( rc.next() )
{
	i++;
	record.push( rc.current().toObj()["country"] );
}

if( record[0] !== "france" || record[1] !== "germany"
	|| record[2] !== "japan" || record[3] !== "russia" )
{
	throw "Failed to compare records fields.";
}
if( 4 !== i )
{
	throw "Failed to compare records count.";
}

println( "------Begin to drop cl in the end." );
try
{
	db.execUpdate( "drop collection " + csName + "." + clName );
}
catch( e )
{
	println( "Failed to drop cl in the end." );
	throw e;
}