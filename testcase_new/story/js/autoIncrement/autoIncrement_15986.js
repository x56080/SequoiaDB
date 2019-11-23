/******************************************************************************
@Description :   seqDB-15986:新增嵌套类型的自增字段
@Modify list :   2018-10-16    xiaoni Zhao  Init
******************************************************************************/
function main()
{
   if(commIsStandalone( db ))
   {
      println("Deploy is standalone");
      return;
   } 
    
   var clName = COMMCLNAME + "_15986";
   
   commDropCL( db, COMMCSNAME, clName );
   
   var dbcl = commCreateCLByOption( db, COMMCSNAME, clName );
   
   dbcl.insert( { a : 1, b : 1 } );
  
   dbcl.createAutoIncrement( { Field : "a.aa" } ); 
   
   dbcl.createAutoIncrement( { Field : "b.bb.bbb" } );  
 
   var options = [{ Field : "c.1" }];
   create(dbcl, options, false);  
   
   //check autoIncrement
   var clID = getCLID( COMMCSNAME, clName );
   var sequenceNames = [ "SYS_" + clID + "_a.aa_SEQ", "SYS_" + clID + "_b.bb.bbb_SEQ" ];
   var expAotoIncrement = [ { Field : "a.aa", SequenceName : sequenceNames[0] },
                            { Field : "b.bb.bbb", SequenceName : sequenceNames[1] } ];
   checkAutoIncrementonCL( COMMCSNAME, clName, expAotoIncrement );
  
   //check sequence
   for( var i in sequenceNames )
   {
      checkSequence( sequenceNames[i], {} );
   }
   
   dbcl.dropAutoIncrement(["a.aa", "b.bb.bbb"]);
   
   commDropCL( db, COMMCSNAME, clName );
}

try
{
   main();
}
catch(e)
{
   if ( e.constructor === Error )
   {
      println(e.stack) ;  
   }
   throw e ;
}
