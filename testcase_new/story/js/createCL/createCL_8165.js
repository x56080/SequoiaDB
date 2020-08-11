/******************************************************************************
*@Description : createCL basic testcase
*@Modify list :
*               2015-01-22   Pusheng Ding Init
******************************************************************************/
csName_1 = CHANGEDPREFIX + "_normal";
csName_2 = CHANGEDPREFIX + "_rangecl";
csName_3 = CHANGEDPREFIX + "_hashcl";
csName_4 = CHANGEDPREFIX + "_maincl";
csName_5 = CHANGEDPREFIX + "_subcl1";
csName_6 = CHANGEDPREFIX + "_subcl2";

var db = new Sdb( COORDHOSTNAME, COORDSVCNAME );

try
{
   commDropCL( db, COMMCSNAME, csName_1, true, true,
      "drop CL1 in the beginning" );
   commDropCL( db, COMMCSNAME, csName_2, true, true,
      "drop CL2 in the beginning" );
   commDropCL( db, COMMCSNAME, csName_3, true, true,
      "drop CL3 in the beginning" );
   commDropCL( db, COMMCSNAME, csName_5, true, true,
      "drop CL5 in the beginning" );
   commDropCL( db, COMMCSNAME, csName_6, true, true,
      "drop CL6 in the beginning" );
   // main cl
   commDropCL( db, COMMCSNAME, csName_4, true, true,
      "drop CL4 in the beginning" );
}
catch( e )
{
   println( "unexpected err happened when clear cs:" + e );
   throw e;
}

try
{
   var varCS = commCreateCS( db, COMMCSNAME, true, "create CS" );
}
catch( e )
{
   println( "failed to create cs, rc= " + e );
   throw e;
}

//normal cl
try
{
   var varCL = varCS.createCL( csName_1, { Compressed: true } );
}
catch( e )
{
   println( "failed to create cl " + csName_1 + ", rc= " + e );
   throw e;
}
println( "create cl " + csName_1 + " finished" );

//range cl
try
{
   var varCL1 = varCS.createCL( csName_2, { Compressed: false, ShardingType: "range", ShardingKey: { a: 1, b: 1 } } );
}
catch( e )
{
   println( "failed to create cl " + csName_2 + ", rc= " + e );
   throw e;
}
println( "create range cl " + csName_2 + " finished" );

//hash cl
try
{
   var varCL2 = varCS.createCL( csName_3, { ShardingType: "hash", ShardingKey: { a: 1 }, Partition: 4096, Compressed: true, AutoSplit: true } );
}
catch( e )
{
   println( "failed to create cl " + csName_3 + ", rc= " + e );
   throw e;
}
println( "create hash cl " + csName_3 + " finished" );

//main cl
try
{
   var varCL3 = varCS.createCL( csName_4, { ShardingKey: { id: 1 }, IsMainCL: true } );
   varCS.createCL( csName_5 );
   varCS.createCL( csName_6, { ShardingKey: { a: 1, b: -1 }, ShardingType: "hash", Compressed: true } );
   varCL3.attachCL( COMMCSNAME + "." + csName_5, { LowBound: { id: { "$minKey": 1 } }, UpBound: { id: 0 } } );
   varCL3.attachCL( COMMCSNAME + "." + csName_6, { LowBound: { id: 0 }, UpBound: { id: { "$maxKey": 1 } } } );
}
catch( e )
{
   if( false == commIsStandalone( db ) )
   {
      println( "failed to create cl " + csName_4 + ", rc= " + e );
      throw e;
   }
}

if( false == commIsStandalone( db ) )
{
   println( "create main cl " + csName_4 + " and attach subcl finished" );

   try
   {
      varCL3.detachCL( COMMCSNAME + "." + csName_6 );
      varCL3.detachCL( COMMCSNAME + "." + csName_5 );
   } catch( e )
   {
      println( csName_4 + " detachCL failed, rc= " + e );
      throw e;
   }
   println( csName_4 + " detachCL finished" );

}
try
{
   commDropCL( db, COMMCSNAME, csName_1, true, true,
      "drop CL1 in the beginning" );
   commDropCL( db, COMMCSNAME, csName_2, true, true,
      "drop CL2 in the beginning" );
   commDropCL( db, COMMCSNAME, csName_3, true, true,
      "drop CL3 in the beginning" );
   commDropCL( db, COMMCSNAME, csName_5, true, true,
      "drop CL5 in the beginning" );
   commDropCL( db, COMMCSNAME, csName_6, true, true,
      "drop CL6 in the beginning" );
   // main cl
   commDropCL( db, COMMCSNAME, csName_4, true, true,
      "drop CL4 in the beginning" );
}
catch( e )
{
   println( "unexpected err happened when clear cs:" + e );
   throw e;
}
