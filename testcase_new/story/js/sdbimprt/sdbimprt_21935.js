/***************************************************************************
@Description : seqDB-21935:指定 linepriority默认、auto、true、false情况下导入记录优先级正确性
@Modify list :
2020-03-11  chensiqin  Create
****************************************************************************/

testConf.csName = COMMCSNAME;
testConf.clName = COMMCLNAME + "_21935";

main( test );

function test( testPara )
{

   cl = testPara.testCL;
   clName = testConf.clName
   testImprtJson1( clName, cl );
   testImprtJson2( clName, cl );
   testImprtCsv1( clName, cl );
   testImprtCsv2( clName, cl );
}

function testImprtJson1( clName, cl )
{
   var filename = tmpFileDir + "21935_1.json";
   var file = fileInit( filename );
   file.write( "{ \"a\": 1, \"c\": \"csq\"csq\" }\n" );
   file.write( "{ \"a\": 1, \"b\": 2 }" );
   file.close();
   var command = installDir + "bin/sdbimprt" +
      " --hosts " + COORDHOSTNAME + ":" + COORDSVCNAME +
      " -c " + COMMCSNAME +
      " -l " + clName +
      " --type json " + 
      "--fields a,b,c " +
      "--file " + filename;
   
   //不指定--linepriority 
   cmd.run( command );
   var expectResult = [];
   commCompareResults( cl.find(), expectResult );
   //linepriority auto
   cmd.run( command + " --linepriority auto");
   commCompareResults( cl.find(), expectResult );
   //linepriority flase
   cmd.run( command + " --linepriority false");
   commCompareResults( cl.find(), expectResult );
   // linepriority true
   cmd.run( command + " --linepriority true");
   expectResult =  [ { "a": 1, "b": 2 } ];
   commCompareResults( cl.find(), expectResult ); 
}

function testImprtJson2( clName, cl )
{
   cl.remove();
   var filename = tmpFileDir + "21935_2.json";
   var file = fileInit( filename );
   file.write( "{ \"a\": \"Mike\n\" }\n" );
   file.write( "{ \"a\": 1, \"b\": 2 }" );
   file.close();
   var command = installDir + "bin/sdbimprt" +
      " --hosts " + COORDHOSTNAME + ":" + COORDSVCNAME +
      " -c " + COMMCSNAME +
      " -l " + clName +
      " --type json " + 
      "--fields a,b,c " +
      "--file " + filename;
   
   //不指定--linepriority 
   cmd.run( command );
   var expectResult = [{"a":"Mike\n"},{"a":1,"b":2}];
   commCompareResults( cl.find(), expectResult );
   //linepriority auto
   cl.remove();
   cmd.run( command + " --linepriority auto");
   commCompareResults( cl.find(), expectResult );
   //linepriority flase
   cl.remove();
   cmd.run( command + " --linepriority false");
   commCompareResults( cl.find(), expectResult );
   // linepriority true
   cl.remove();
   cmd.run( command + " --linepriority true");
   expectResult = [ { "a": 1, "b": 2 } ];
   commCompareResults( cl.find(), expectResult );
}

function testImprtCsv1( clName, cl )
{
   var filename = tmpFileDir + "21935_1.csv";
   var file = fileInit( filename );
   file.write( "\"Jack\",18,\"Chi\"na\"\n" );
   file.write( "\"Mike\",20,\"USA\"\n" );
   file.close();
   var command = installDir + "bin/sdbimprt" +
      " --hosts " + COORDHOSTNAME + ":" + COORDSVCNAME +
      " -c " + COMMCSNAME +
      " -l " + clName +
      " --type csv " + 
      "--fields a,b,c " +
      "--file " + filename;
   cl.remove();
   //不指定--linepriority 
   cmd.run( command );
   var expectResult = [ {"a": "Mike","b": 20,"c": "USA"} ];
   commCompareResults( cl.find(), expectResult );
   cl.remove();
   //linepriority auto
   cmd.run( command + " --linepriority auto");
   commCompareResults( cl.find(), expectResult );
   cl.remove();
   //linepriority flase
   cmd.run( command + " --linepriority true");
   commCompareResults( cl.find(), expectResult );
   cl.remove();
   // linepriority true
   cmd.run( command + " --linepriority false");
   expResult =  [];
   commCompareResults( cl.find(), expectResult );
}

function testImprtCsv2( clName, cl )
{
   var filename = tmpFileDir + "21935_2.csv";
   var file = fileInit( filename );
   file.write( '"Jack",18,"Chi\n' );
   file.write( 'na"\n' );
   file.write( '"Mike",20,"USA"\n' );
   file.close();
   var command = installDir + "bin/sdbimprt" +
      " --hosts " + COORDHOSTNAME + ":" + COORDSVCNAME +
      " -c " + COMMCSNAME +
      " -l " + clName +
      " --type csv " + 
      "--fields a,b,c " +
      "--file " + filename;
   cl.remove();
   //不指定--linepriority 
   cmd.run( command );
   var expectResult = [ {"a": "Jack","b": 18,"c": "Chi"}, {"a": "na"}, {"a": "Mike","b": 20,"c": "USA"} ];
   commCompareResults( cl.find(), expectResult );
   cl.remove();
   //linepriority auto
   cmd.run( command + " --linepriority auto");
   commCompareResults( cl.find(), expectResult );
   cl.remove();
   //linepriority flase
   cmd.run( command + " --linepriority true");
   commCompareResults( cl.find(), expectResult );
   cl.remove();
   // linepriority true
   cmd.run( command + " --linepriority false");
   expectResult =  [{"a": "Jack","b": 18,"c": "Chi\nna"}, {"a": "Mike","b": 20,"c": "USA"} ];
   commCompareResults( cl.find(), expectResult );   
}