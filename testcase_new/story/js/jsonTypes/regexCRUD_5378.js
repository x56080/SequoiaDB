/******************************************************************************
*@Description : test CRUD with Regex function
*@Modify list :
*               2016-07-11   XueWang Liang  Init
******************************************************************************/

function main( db )
{
   var cl = commCreateCL( db, COMMCSNAME, COMMCLNAME, 0, true, true, true, "create CL in the begining..." ); 
   
   // 以Regex函数的方式插入数据
   cl.insert( {reg:Regex( "^W", "i" )} ); 
   
   // 以Regex函数的方式查询数据
   var rc = cl.find( {reg:{$et:Regex( "^W", "i" )}}, {reg:""} ); 
   var expRecs = {reg:{$regex:"^W", $options:"i"}}; 
   checkRec( rc, [expRecs] ); 
   
   // 以普通方式查询数据
   rc = cl.find( {reg:{$et:{"$regex":"^W", "$options":"i"}}}, {reg:""} ); 
   expRecs = {reg:{$regex:"^W", $options:"i"}}; 
   checkRec( rc, [expRecs] ); 
   
   // 使用$type匹配符查询数据
   rc = cl.find( {reg:{$type:1, $et:11}}, {reg:""} ); 
   expRecs = {reg:{$regex:"^W", $options:"i"}}; 
   checkRec( rc, [expRecs] ); 
   
   println( ">success to test CRUD with Regex function.\n\n" ); 
}


// Test
try
{
   commDropCL( db, COMMCSNAME, COMMCLNAME, true, true, 
   "clean collection in the beginning" ); 
   main( db ); 
}
catch( e )
{
   throw e; 
}
finally
{
   commDropCL( db, COMMCSNAME, COMMCLNAME, true, true, 
   "clean collection in the end, wrong" ); 
   db.close(); 
}
