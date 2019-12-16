/****************************************************
@description:	select by SQL with regex, basic case
         testlink cases:   seqDB-7420
@input:        1 insert into records, success
               2 select with regex, success
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
   db.execUpdate( "insert into " + csName + "." + clName + " (name) values('Aom1') " );
   db.execUpdate( "insert into " + csName + "." + clName + " (name) values('Aom2') " );
   db.execUpdate( "insert into " + csName + "." + clName + " (name) values('aom3') " );
   db.execUpdate( "insert into " + csName + "." + clName + " (name) values('Bom4') " );
   db.execUpdate( "insert into " + csName + "." + clName + " (name) values('Tom5') " );
   db.execUpdate( "insert into " + csName + "." + clName + " (name) values('CCC6') " );
   db.execUpdate( "insert into " + csName + "." + clName + " (name) values('D 7') " );
}
catch( e )
{
   println( "Failed to insert records." );
   throw e;
}

println( "------Begin to select with regex." );
try
{
   var cur = db.exec( "select * from " + csName + "." + clName + " where name like '^A.*1$' " );
   while( cur.next() )
   {
      if( "Aom1" != cur.current().toObj()["name"] )
         throw "Failed to select with '^A.*1$'.";
   }

   var cur = db.exec( "select * from " + csName + "." + clName + " where name like 'C+' " );
   while( cur.next() )
   {
      if( "CCC6" != cur.current().toObj()["name"] )
         throw "Failed to select with 'C+'.";
   }

   var cur = db.exec( "select * from " + csName + "." + clName + " where name like 'Bo*' " );
   while( cur.next() )
   {
      if( "Bom4" != cur.current().toObj()["name"] )
         throw "Failed to select with 'C+'.";
   }

   var cur = db.exec( "select * from " + csName + "." + clName + " where name like '[A-B]' " );
   i = 0;
   while( cur.next() )
   {
      i++;
   }
   if( i != 3 )
      throw "Failed to select with '[A-B]'.";

   var cur = db.exec( "select * from " + csName + "." + clName + " where name like '.*m1{1,}' " );
   while( cur.next() )
   {
      if( "Aom1" != cur.current().toObj()["name"] )
         throw "Failed to select with '.*m1{1,}'.";
   }

   var cur = db.exec( "select * from " + csName + "." + clName + " where name like 'Aom.' " );
   i = 0;
   while( cur.next() )
   {
      i++;
   }
   if( i != 2 )
      throw "Failed to select with 'Aom.'.";

   var cur = db.exec( "select * from " + csName + "." + clName + " where name like 'B|T' " );
   i = 0;
   while( cur.next() )
   {
      i++;
   }
   if( i != 2 )
      throw "Failed to select with 'B|T'.";

   var cur = db.exec( "select * from " + csName + "." + clName + " where name like '^[aB]' " );
   i = 0;
   while( cur.next() )
   {
      i++;
   }
   if( i != 2 )
      throw "Failed to select with '^[aB]'.";

   var cur = db.exec( "select * from " + csName + "." + clName + " where name like '^[^aAB]' " );
   i = 0;
   while( cur.next() )
   {
      i++;
   }
   if( i != 3 )
      throw "Failed to select with '^[^aAB]'.";
}
catch( e )
{
   println( "Failed to select with regex." );
   throw e;
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