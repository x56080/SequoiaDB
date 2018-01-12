/*******************************************************************
* @Description : test export with --filter
*                seqDB-13581:--filter指定正确的查询条件
*                匹配符：$gt $gte $lt $lte $ne $et 
*                        $mod 
*                        $in $nin
*                        $isnull
*                        $all 
*                        $and $not $or $type（变成函数操作） 
*                        $exists
*                        $elemMatch 
*                        $1 $size（变成函数操作） 
*                        $regex 
*                        $field 
*                        $expand    
*                        $returnMatch
* @author      : Liang XueWang
* 
*******************************************************************/
var csname = COMMCSNAME ;
var clname = COMMCLNAME + "_sdbexprt13581" ;
var clname1 = COMMCLNAME + "_sdbimprt13581" ;

main() ;

function main()
{  
   testExprtFilter1() ;  // test filter with $gte
   testExprtFilter2() ;  // test filter with $mod 
   testExprtFilter3() ;  // test filter with $in
   testExprtFilter4() ;  // test filter with $isnull
   testExprtFilter5() ;  // test filter with $all
   testExprtFilter6() ;  // test filter with $and
   testExprtFilter7() ;  // test filter with $exists
   testExprtFilter8() ;  // test filter with $elemMatch
   testExprtFilter9() ;  // test filter with $1
   testExprtFilter10() ;  // test filter with $regex
   testExprtFilter11() ;  // test filter with $field
   testExprtFilter12() ;  // test filter with $expand
   testExprtFilter13() ;  // test filter with $returnMatch
}

function testExprtFilter1()
{
   println( "test filter with $gte" ) ;
   var docs = [ { a: 1 }, { a: 2 }, { a: 3 }, { a: 4 } ] ;
   var cl = commCreateCL( db, csname, clname, 0 ) ;
   var cl1 = commCreateCL( db, csname, clname1, 0 ) ;
   cl.insert( docs ) ;
   
   var csvfile = workDir + "sdbexprt13581.csv" ;
   cmd.run( "rm -rf " + csvfile ) ;
   
   var command = installPath + "bin/sdbexprt" +
                 " -s " + COORDHOSTNAME +
                 " -p " + COORDSVCNAME +
                 " -c " + csname +
                 " -l " + clname + 
                 " --file " + csvfile +
                 " --filter '{ a: { \\$gte: 2 } }'" +
                 " --type csv" +
                 " --sort '{ _id: 1 }'" +
                 " --fields a" ;                
   testRunCommand( command ) ;
   
   var content = "a\n2\n3\n4\n" ;
   checkFileContent( csvfile, content ) ;
   
   command = installPath + "bin/sdbimprt" +
             " -s " + COORDHOSTNAME +
             " -p " + COORDSVCNAME +
             " -c " + csname +
             " -l " + clname1 +
             " --type csv" +
             " --file " + csvfile +
             " --fields='a int'" ;
   testRunCommand( command ) ;
  
   var expRecs = [ "{\"a\":2}", "{\"a\":3}", "{\"a\":4}" ] ; 
   var cursor = cl1.find( {}, { _id: { $include: 0 } } ).sort( { a: 1 } ) ;
   var actRecs = getRecords( cursor ) ;
   checkRecords( expRecs, actRecs ) ;
   
   cmd.run( "rm -rf " + csvfile ) ;
   
   commDropCL( db, csname, clname ) ;
   commDropCL( db, csname, clname1 ) ;
}

function testExprtFilter2()
{
   println( "test filter with $mod" ) ;
   var docs = [ { a: 1 }, { a: 2 }, { a: 3 }, { a: 4 } ] ;
   var cl = commCreateCL( db, csname, clname, 0 ) ;
   var cl1 = commCreateCL( db, csname, clname1, 0 ) ;
   cl.insert( docs ) ;
   
   var csvfile = workDir + "sdbexprt13581.csv" ;
   cmd.run( "rm -rf " + csvfile ) ;
   
   var command = installPath + "bin/sdbexprt" +
                 " -s " + COORDHOSTNAME +
                 " -p " + COORDSVCNAME +
                 " -c " + csname +
                 " -l " + clname + 
                 " --file " + csvfile +
                 " --filter '{ a: { \\$mod: [ 2, 1 ] } }'" +
                 " --type csv" +
                 " --sort '{ _id: 1 }'" +
                 " --fields a" ;                
   testRunCommand( command ) ;
   
   var content = "a\n1\n3\n" ;
   checkFileContent( csvfile, content ) ;
   
   command = installPath + "bin/sdbimprt" +
             " -s " + COORDHOSTNAME +
             " -p " + COORDSVCNAME +
             " -c " + csname +
             " -l " + clname1 +
             " --type csv" +
             " --file " + csvfile +
             " --fields='a int'" ;
   testRunCommand( command ) ;
  
   var expRecs = [ "{\"a\":1}", "{\"a\":3}" ] ; 
   var cursor = cl1.find( {}, { _id: { $include: 0 } } ).sort( { a: 1 } ) ;
   var actRecs = getRecords( cursor ) ;
   checkRecords( expRecs, actRecs ) ;
   
   cmd.run( "rm -rf " + csvfile ) ;
   
   commDropCL( db, csname, clname ) ;
   commDropCL( db, csname, clname1 ) ;
}

function testExprtFilter3()
{
   println( "test filter with $in" ) ;
   var docs = [ { a: 1 }, { a: 2 }, { a: 3 }, { a: 4 } ] ;
   var cl = commCreateCL( db, csname, clname, 0 ) ;
   var cl1 = commCreateCL( db, csname, clname1, 0 ) ;
   cl.insert( docs ) ;
   
   var csvfile = workDir + "sdbexprt13581.csv" ;
   cmd.run( "rm -rf " + csvfile ) ;
   
   var command = installPath + "bin/sdbexprt" +
                 " -s " + COORDHOSTNAME +
                 " -p " + COORDSVCNAME +
                 " -c " + csname +
                 " -l " + clname + 
                 " --file " + csvfile +
                 " --filter '{ a: { \\$in: [ 1, 3, 4 ] } }'" +
                 " --type csv" +
                 " --sort '{ _id: 1 }'" +
                 " --fields a" ;                
   testRunCommand( command ) ;
   
   var content = "a\n1\n3\n4\n" ;
   checkFileContent( csvfile, content ) ;
   
   command = installPath + "bin/sdbimprt" +
             " -s " + COORDHOSTNAME +
             " -p " + COORDSVCNAME +
             " -c " + csname +
             " -l " + clname1 +
             " --type csv" +
             " --file " + csvfile +
             " --fields='a int'" ;
   testRunCommand( command ) ;
  
   var expRecs = [ "{\"a\":1}", "{\"a\":3}", "{\"a\":4}" ] ; 
   var cursor = cl1.find( {}, { _id: { $include: 0 } } ).sort( { a: 1 } ) ;
   var actRecs = getRecords( cursor ) ;
   checkRecords( expRecs, actRecs ) ;
   
   cmd.run( "rm -rf " + csvfile ) ;
   
   commDropCL( db, csname, clname ) ;
   commDropCL( db, csname, clname1 ) ;
}

function testExprtFilter4()
{
   println( "test filter with $isnull" ) ;
   var docs = [ { a: 1 }, { a: 2 }, { a: 3 }, { a: 4 } ] ;
   var cl = commCreateCL( db, csname, clname, 0 ) ;
   var cl1 = commCreateCL( db, csname, clname1, 0 ) ;
   cl.insert( docs ) ;
   
   var csvfile = workDir + "sdbexprt13581.csv" ;
   cmd.run( "rm -rf " + csvfile ) ;
   
   var command = installPath + "bin/sdbexprt" +
                 " -s " + COORDHOSTNAME +
                 " -p " + COORDSVCNAME +
                 " -c " + csname +
                 " -l " + clname + 
                 " --file " + csvfile +
                 " --filter '{ a: { \\$isnull: 0 } }'" +
                 " --type csv" +
                 " --sort '{ _id: 1 }'" +
                 " --fields a" ;                
   testRunCommand( command ) ;
   
   var content = "a\n1\n2\n3\n4\n" ;
   checkFileContent( csvfile, content ) ;
   
   command = installPath + "bin/sdbimprt" +
             " -s " + COORDHOSTNAME +
             " -p " + COORDSVCNAME +
             " -c " + csname +
             " -l " + clname1 +
             " --type csv" +
             " --file " + csvfile +
             " --fields='a int'" ;
   testRunCommand( command ) ;
  
   var expRecs = [ "{\"a\":1}", "{\"a\":2}", "{\"a\":3}", "{\"a\":4}" ] ; 
   var cursor = cl1.find( {}, { _id: { $include: 0 } } ).sort( { a: 1 } ) ;
   var actRecs = getRecords( cursor ) ;
   checkRecords( expRecs, actRecs ) ;
   
   cmd.run( "rm -rf " + csvfile ) ;
   
   commDropCL( db, csname, clname ) ;
   commDropCL( db, csname, clname1 ) ;
}

function testExprtFilter5()
{
   println( "test filter with $all" ) ;
   var docs = [ { name: [ "Tom", "Mike", "Jack" ] }, 
                { name: [ "Tom", "John" ] } ] ;
   var cl = commCreateCL( db, csname, clname, 0 ) ;
   var cl1 = commCreateCL( db, csname, clname1, 0 ) ;
   cl.insert( docs ) ;
   
   var csvfile = workDir + "sdbexprt13581.csv" ;
   cmd.run( "rm -rf " + csvfile ) ;
   
   var command = installPath + "bin/sdbexprt" +
                 " -s " + COORDHOSTNAME +
                 " -p " + COORDSVCNAME +
                 " -c " + csname +
                 " -l " + clname + 
                 " --file " + csvfile +
                 " --filter '{ name: { \\$all: [ \"Tom\", \"Mike\" ] } }'" +
                 " --type csv" +
                 " --fields name" ;                
   testRunCommand( command ) ;
   
   var content = "name\n" + 
					  "\"[ \"\"Tom\"\", \"\"Mike\"\", \"\"Jack\"\" ]\"\n" ;
   checkFileContent( csvfile, content ) ;
   
   command = installPath + "bin/sdbimprt" +
             " -s " + COORDHOSTNAME +
             " -p " + COORDSVCNAME +
             " -c " + csname +
             " -l " + clname1 +
             " --type csv" +
             " --file " + csvfile +
             " --headerline true" +
             " --fields='name string'" ;
   testRunCommand( command ) ;
  
   var expRecs = [ "{\"name\":\"[ \\\"Tom\\\", \\\"Mike\\\", \\\"Jack\\\" ]\"}" ] ; 
   var cursor = cl1.find( {}, { _id: { $include: 0 } } ) ;
   var actRecs = getRecords( cursor ) ;
   checkRecords( expRecs, actRecs ) ;
   
   cmd.run( "rm -rf " + csvfile ) ;
   
   commDropCL( db, csname, clname ) ;
   commDropCL( db, csname, clname1 ) ;
}

function testExprtFilter6()
{
   println( "test filter with $and" ) ;
   var docs = [ { a: 1 }, { a: 2 }, { a: 3 }, { a: 4 } ] ;
   var cl = commCreateCL( db, csname, clname, 0 ) ;
   var cl1 = commCreateCL( db, csname, clname1, 0 ) ;
   cl.insert( docs ) ;
   
   var csvfile = workDir + "sdbexprt13581.csv" ;
   cmd.run( "rm -rf " + csvfile ) ;
   
   var command = installPath + "bin/sdbexprt" +
                 " -s " + COORDHOSTNAME +
                 " -p " + COORDSVCNAME +
                 " -c " + csname +
                 " -l " + clname + 
                 " --file " + csvfile +
                 " --filter '{ \\$and: [ { a: { \\$gte: 2 } }, { a: { \\$lte: 3 } } ] }'" +
                 " --type csv" +
                 " --sort '{ _id: 1 }'" +
                 " --fields a" ;                
   testRunCommand( command ) ;
   
   var content = "a\n2\n3\n" ;
   checkFileContent( csvfile, content ) ;
   
   command = installPath + "bin/sdbimprt" +
             " -s " + COORDHOSTNAME +
             " -p " + COORDSVCNAME +
             " -c " + csname +
             " -l " + clname1 +
             " --type csv" +
             " --file " + csvfile +
             " --fields='a int'" ;
   testRunCommand( command ) ;
  
   var expRecs = [ "{\"a\":2}", "{\"a\":3}" ] ; 
   var cursor = cl1.find( {}, { _id: { $include: 0 } } ).sort( { a: 1 } ) ;
   var actRecs = getRecords( cursor ) ;
   checkRecords( expRecs, actRecs ) ;
   
   cmd.run( "rm -rf " + csvfile ) ;
   
   commDropCL( db, csname, clname ) ;
   commDropCL( db, csname, clname1 ) ;
}

function testExprtFilter7()
{
   println( "test filter with $exists" ) ;
   var docs = [ { a: 1 }, { a: 2, b: 2 }, { a: 3 }, { a: 4, b: 4 } ] ;
   var cl = commCreateCL( db, csname, clname, 0 ) ;
   var cl1 = commCreateCL( db, csname, clname1, 0 ) ;
   cl.insert( docs ) ;
   
   var csvfile = workDir + "sdbexprt13581.csv" ;
   cmd.run( "rm -rf " + csvfile ) ;
   
   var command = installPath + "bin/sdbexprt" +
                 " -s " + COORDHOSTNAME +
                 " -p " + COORDSVCNAME +
                 " -c " + csname +
                 " -l " + clname + 
                 " --file " + csvfile +
                 " --filter '{ b: { \\$exists: 1 } }'" +
                 " --type csv" +
                 " --sort '{ _id: 1 }'" +
                 " --fields a,b" ;   
   testRunCommand( command ) ;
   
   var content = "a,b\n2,2\n4,4\n" ;
   checkFileContent( csvfile, content ) ;
   
   command = installPath + "bin/sdbimprt" +
             " -s " + COORDHOSTNAME +
             " -p " + COORDSVCNAME +
             " -c " + csname +
             " -l " + clname1 +
             " --type csv" +
             " --file " + csvfile +
             " --fields='a int,b int'" ;
   testRunCommand( command ) ;
  
   var expRecs = [ "{\"a\":2,\"b\":2}", "{\"a\":4,\"b\":4}" ] ; 
   var cursor = cl1.find( {}, { _id: { $include: 0 } } ).sort( { a: 1 } ) ;
   var actRecs = getRecords( cursor ) ;
   checkRecords( expRecs, actRecs ) ;
   
   cmd.run( "rm -rf " + csvfile ) ;
   
   commDropCL( db, csname, clname ) ;
   commDropCL( db, csname, clname1 ) ;
}

function testExprtFilter8()
{
   println( "test filter with $elemMatch" ) ;
   var docs = [ { info: { name: "Jack", phone: "1234" } },
                { info: [ { name: "Jack", phone: "5678" } ] } ] ;
   var cl = commCreateCL( db, csname, clname, 0 ) ;
   var cl1 = commCreateCL( db, csname, clname1, 0 ) ;
   cl.insert( docs ) ;
   
   var csvfile = workDir + "sdbexprt13581.csv" ;
   cmd.run( "rm -rf " + csvfile ) ;
   
   var command = installPath + "bin/sdbexprt" +
                 " -s " + COORDHOSTNAME +
                 " -p " + COORDSVCNAME +
                 " -c " + csname +
                 " -l " + clname + 
                 " --file " + csvfile +
                 " --filter '{ info: { \\$elemMatch: { name: \"Jack\" } } }'" +
                 " --type csv" +
                 " --sort '{ _id: 1 }'" +
                 " --fields info" ;   
   testRunCommand( command ) ;
   
   var content = "info\n" +
                 "\"{ \"\"name\"\": \"\"Jack\"\", \"\"phone\"\": \"\"1234\"\" }\"\n" +
                 "\"[ { \"\"name\"\": \"\"Jack\"\", \"\"phone\"\": \"\"5678\"\" } ]\"\n" ;
   checkFileContent( csvfile, content ) ;
   
   command = installPath + "bin/sdbimprt" +
             " -s " + COORDHOSTNAME +
             " -p " + COORDSVCNAME +
             " -c " + csname +
             " -l " + clname1 +
             " --type csv" +
             " --file " + csvfile +
             " --headerline true" +
             " --fields='info string'" ;
   testRunCommand( command ) ;
  
   var expRecs = [ 
       "{\"info\":\"{ \\\"name\\\": \\\"Jack\\\", \\\"phone\\\": \\\"1234\\\" }\"}", 
       "{\"info\":\"[ { \\\"name\\\": \\\"Jack\\\", \\\"phone\\\": \\\"5678\\\" } ]\"}" 
                 ] ; 
   var cursor = cl1.find( {}, { _id: { $include: 0 } } ) ;
   var actRecs = getRecords( cursor ) ;
   checkRecords( expRecs, actRecs ) ;
   
   cmd.run( "rm -rf " + csvfile ) ;
   
   commDropCL( db, csname, clname ) ;
   commDropCL( db, csname, clname1 ) ;
}

function testExprtFilter9()
{
   println( "test filter with $1" ) ;
   var docs = [ { a: [ 1, 2, 3, 4, 5 ] },
                { a: [ 1, 4, 5 ] }, { a: [ 1, 2, 4 ] } ] ;
   var cl = commCreateCL( db, csname, clname, 0 ) ;
   var cl1 = commCreateCL( db, csname, clname1, 0 ) ;
   cl.insert( docs ) ;
   
   var csvfile = workDir + "sdbexprt13581.csv" ;
   cmd.run( "rm -rf " + csvfile ) ;
   
   var command = installPath + "bin/sdbexprt" +
                 " -s " + COORDHOSTNAME +
                 " -p " + COORDSVCNAME +
                 " -c " + csname +
                 " -l " + clname + 
                 " --file " + csvfile +
                 " --filter '{ a.\\$1: 5 }'" +
                 " --type csv" +
                 " --sort '{ _id: 1 }'" +
                 " --fields a" ;   
   testRunCommand( command ) ;
   
   var content = "a\n" +
                 "\"[ 1, 2, 3, 4, 5 ]\"\n" +
                 "\"[ 1, 4, 5 ]\"\n" ;
   checkFileContent( csvfile, content ) ;
   
   command = installPath + "bin/sdbimprt" +
             " -s " + COORDHOSTNAME +
             " -p " + COORDSVCNAME +
             " -c " + csname +
             " -l " + clname1 +
             " --type csv" +
             " --file " + csvfile +
             " --headerline true" +
             " --fields='a string'" ;
   testRunCommand( command ) ;
  
   var expRecs = [ 
       "{\"a\":\"[ 1, 2, 3, 4, 5 ]\"}", 
       "{\"a\":\"[ 1, 4, 5 ]\"}" 
                 ] ; 
   var cursor = cl1.find( {}, { _id: { $include: 0 } } ) ;
   var actRecs = getRecords( cursor ) ;
   checkRecords( expRecs, actRecs ) ;
   
   cmd.run( "rm -rf " + csvfile ) ;
   
   commDropCL( db, csname, clname ) ;
   commDropCL( db, csname, clname1 ) ;
}

function testExprtFilter10()
{
   println( "test filter with $regex" ) ;
   var docs = [ { a: "abandon" }, { a: "Alice" }, { a: "beyond" } ] ;
   var cl = commCreateCL( db, csname, clname, 0 ) ;
   var cl1 = commCreateCL( db, csname, clname1, 0 ) ;
   cl.insert( docs ) ;
   
   var csvfile = workDir + "sdbexprt13581.csv" ;
   cmd.run( "rm -rf " + csvfile ) ;
   
   var command = installPath + "bin/sdbexprt" +
                 " -s " + COORDHOSTNAME +
                 " -p " + COORDSVCNAME +
                 " -c " + csname +
                 " -l " + clname + 
                 " --file " + csvfile +
                 " --filter '{ a: { \\$regex: \"^a\", \\$options: \"i\" } }'" +
                 " --type csv" +
                 " --sort '{ _id: 1 }'" +
                 " --fields a" ;   
   testRunCommand( command ) ;
   
   var content = "a\n\"abandon\"\n\"Alice\"\n" ;
   checkFileContent( csvfile, content ) ;
   
   command = installPath + "bin/sdbimprt" +
             " -s " + COORDHOSTNAME +
             " -p " + COORDSVCNAME +
             " -c " + csname +
             " -l " + clname1 +
             " --type csv" +
             " --file " + csvfile +
             " --headerline true" +
             " --fields='a string'" ;
   testRunCommand( command ) ;
  
   var expRecs = [ "{\"a\":\"abandon\"}", "{\"a\":\"Alice\"}" ] ; 
   var cursor = cl1.find( {}, { _id: { $include: 0 } } ) ;
   var actRecs = getRecords( cursor ) ;
   checkRecords( expRecs, actRecs ) ;
   
   cmd.run( "rm -rf " + csvfile ) ;
   
   commDropCL( db, csname, clname ) ;
   commDropCL( db, csname, clname1 ) ;
}

function testExprtFilter11()
{
   println( "test filter with $field" ) ;
   var docs = [ { a: 1, b: 1 }, { a: 2, b: 1 }, { a: 3, b: 3 },
                { a: 4, b: 3 } ] ;
   var cl = commCreateCL( db, csname, clname, 0 ) ;
   var cl1 = commCreateCL( db, csname, clname1, 0 ) ;
   cl.insert( docs ) ;
   
   var csvfile = workDir + "sdbexprt13581.csv" ;
   cmd.run( "rm -rf " + csvfile ) ;
   
   var command = installPath + "bin/sdbexprt" +
                 " -s " + COORDHOSTNAME +
                 " -p " + COORDSVCNAME +
                 " -c " + csname +
                 " -l " + clname + 
                 " --file " + csvfile +
                 " --filter '{ a: { \\$field: \"b\" } }'" +
                 " --type csv" +
                 " --sort '{ _id: 1 }'" +
                 " --fields a,b" ;   
   testRunCommand( command ) ;
   
   var content = "a,b\n1,1\n3,3\n" ;
   checkFileContent( csvfile, content ) ;
   
   command = installPath + "bin/sdbimprt" +
             " -s " + COORDHOSTNAME +
             " -p " + COORDSVCNAME +
             " -c " + csname +
             " -l " + clname1 +
             " --type csv" +
             " --file " + csvfile +
             " --headerline true" +
             " --fields='a int,b int'" ;
   testRunCommand( command ) ;
  
   var expRecs = [ "{\"a\":1,\"b\":1}", "{\"a\":3,\"b\":3}" ] ; 
   var cursor = cl1.find( {}, { _id: { $include: 0 } } ) ;
   var actRecs = getRecords( cursor ) ;
   checkRecords( expRecs, actRecs ) ;
   
   cmd.run( "rm -rf " + csvfile ) ;
   
   commDropCL( db, csname, clname ) ;
   commDropCL( db, csname, clname1 ) ;
}

function testExprtFilter12()
{
   println( "test filter with $expand" ) ;
   var docs = [ { a: [ 1, 2, 3 ] } ] ;
   var cl = commCreateCL( db, csname, clname, 0 ) ;
   var cl1 = commCreateCL( db, csname, clname1, 0 ) ;
   cl.insert( docs ) ;
   
   var csvfile = workDir + "sdbexprt13581.csv" ;
   cmd.run( "rm -rf " + csvfile ) ;
   
   var command = installPath + "bin/sdbexprt" +
                 " -s " + COORDHOSTNAME +
                 " -p " + COORDSVCNAME +
                 " -c " + csname +
                 " -l " + clname + 
                 " --file " + csvfile +
                 " --filter '{ a: { \\$expand: 1 } }'" +
                 " --type csv" +
                 " --fields a" ;   
   testRunCommand( command ) ;
   
   var content = "a\n1\n2\n3\n" ;
   checkFileContent( csvfile, content ) ;
   
   command = installPath + "bin/sdbimprt" +
             " -s " + COORDHOSTNAME +
             " -p " + COORDSVCNAME +
             " -c " + csname +
             " -l " + clname1 +
             " --type csv" +
             " --file " + csvfile +
             " --headerline true" +
             " --fields='a int'" ;
   testRunCommand( command ) ;
  
   var expRecs = [ "{\"a\":1}", "{\"a\":2}", "{\"a\":3}" ] ; 
   var cursor = cl1.find( {}, { _id: { $include: 0 } } ) ;
   var actRecs = getRecords( cursor ) ;
   checkRecords( expRecs, actRecs ) ;
   
   cmd.run( "rm -rf " + csvfile ) ;
   
   commDropCL( db, csname, clname ) ;
   commDropCL( db, csname, clname1 ) ;
}

function testExprtFilter13()
{
   println( "test filter with $returnMatch" ) ;
   var docs = [ { a: [ 1, 2, 4, 7, 9 ] } ] ;
   var cl = commCreateCL( db, csname, clname, 0 ) ;
   var cl1 = commCreateCL( db, csname, clname1, 0 ) ;
   cl.insert( docs ) ;
   
   var csvfile = workDir + "sdbexprt13581.csv" ;
   cmd.run( "rm -rf " + csvfile ) ;
   
   var command = installPath + "bin/sdbexprt" +
                 " -s " + COORDHOSTNAME +
                 " -p " + COORDSVCNAME +
                 " -c " + csname +
                 " -l " + clname + 
                 " --file " + csvfile +
                 " --filter '{ a: { \\$returnMatch: 0, \\$in: [ 1, 4, 7 ] } }'" +
                 " --type csv" +
                 " --fields a" ;   
   testRunCommand( command ) ;
   
   var content = "a\n\"[ 1, 4, 7 ]\"\n" ;
   checkFileContent( csvfile, content ) ;
   
   command = installPath + "bin/sdbimprt" +
             " -s " + COORDHOSTNAME +
             " -p " + COORDSVCNAME +
             " -c " + csname +
             " -l " + clname1 +
             " --type csv" +
             " --file " + csvfile +
             " --headerline true" +
             " --fields='a string'" ;
   testRunCommand( command ) ;
  
   var expRecs = [ "{\"a\":\"[ 1, 4, 7 ]\"}"  ] ; 
   var cursor = cl1.find( {}, { _id: { $include: 0 } } ) ;
   var actRecs = getRecords( cursor ) ;
   checkRecords( expRecs, actRecs ) ;
   
   cmd.run( "rm -rf " + csvfile ) ;
   
   commDropCL( db, csname, clname ) ;
   commDropCL( db, csname, clname1 ) ;
}
