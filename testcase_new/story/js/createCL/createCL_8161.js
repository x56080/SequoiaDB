//creat cl
//innomal case4

TESTCLNAME = CHANGEDPREFIX + "bar";

try
{
   commDropCL( db, COMMCSNAME, TESTCLNAME, true, true, "drop CL in the beginning" );
}
catch( e )
{
   if( e != -34 )
   {
      println( "unexpected err happened when clear cs:" + e );
      throw e;
   }
}
try
{
   var varCS = commCreateCS( db, COMMCSNAME, true, "failed to create CS" );
}
catch( e )
{
   println( "failed to create cs,rc=" + e );
   throw e;
}

var res = false;
try
{
   varCS.createCL( "fsdajfasdjkfasdfsdkfhsdjkfsdfsdfhsdkfhsdfhsdfhsdkfurwerywerhsdjfhsdjfsadfhweruwerewfsdfhsdjfdsjfkhrehfqweufywerhfsdjfhasdjfasdfhsadjkghsafjghruwthwerweiruuuuuuasdkfhasdklfhasdhgsadjfhwedsajhfqweiourweqioruwejfsdahfsadhfsadkl;fhqweiruwqepruweruewruweoifusdjfsdjkagfkgsdfgjsdfkjsdasdfjsadjfwekurqweputiertierutewrigpawertqrtierugfjgfhgfhgafksdghaskfdgjasdfkjadfjas;dfgjasdadfskjasdhf;sjedhrfqweiuqirtursjfsdgjasdfakghartyrqieotwqrasgslketirhungsahdfsdajhfa" );
}
catch( e )
{
   if( e == -6 )
   {
      res = true;
   }
}
if( !res )
{
   throw -1;
}

try
{
   commDropCL( db, COMMCSNAME, TESTCLNAME, true, true, "drop CL in the end" );
}
catch( e )
{
   println( "unexpected err happened when clear cs:" + e );
   throw e;
}
