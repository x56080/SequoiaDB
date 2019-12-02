function testGroupByNull( cl, docNumber )
{
   var cursor = cl.execAggregate( {$group:{_id:null}} ); 
   var retNumber = getRetNumber( cursor ); 
   if( retNumber != docNumber )
   {
      throw buildException( "testGroupByNull", 0, "cl.aggregate( {$group:{_id:null}} )", 
      docNumber, retNumber ); 
   }
}

function testGroupByNotExist( cl )
{
   var expectResult = [{no:1000, score:80, interest:["basketball", "football"], major:"计算机科学与技术", dep:"计算机学院", info:{name:"Tom", age:25, sex:"男"}}]
   var cursor = cl.execAggregate( {$group:{_id:"$field"}} ); 
   var ret = checkResult( cursor, expectResult ); 
   if( !ret[0] )
   {
      throw buildException( "main", 0, "cl.aggregate( {$group:{_id:'$field'}} )", 
      JSON.stringify( ret[1] ), JSON.stringify( ret[2] ) ); 
   }
}

function main()
{
   var cl = new collection( db, COMMCSNAME, COMMCLNAME ); 
   cl.create(); 
   var docNumber = cl.bulkInsert(); 
   testGroupByNull( cl, docNumber ); 
   testGroupByNotExist( cl ); 
   cl.drop(); 
}

main(); 
