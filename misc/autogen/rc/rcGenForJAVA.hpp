#ifndef RC_GEN_FOR_JAVA_HPP
#define RC_GEN_FOR_JAVA_HPP

#include "rcGeneratorBase.hpp"

#define RC_JAVA_FILE_PATH  DRIVER_PATH"java/src/main/java/com/sequoiadb/exception/SDBError.java"
#define RC_JAVA_7_FILE_PATH  DRIVER_PATH"java7/src/main/java/com/sequoiadb/exception/SDBError.java"


class rcGenForJAVABase : public rcGeneratorBase
{

public:
   rcGenForJAVABase() ;
   ~rcGenForJAVABase() ;

   bool hasNext() ;
   virtual int outputFile( int id, fileOutStream &fout,
                           string &outputPath ) = 0;
   const char* name(){ return "rc for JAVA" ; }

protected:
   int _outStream( int id, fileOutStream &fout ) ;

private:
   bool _isFinish ;
} ;

class rcGenForJAVA : public rcGenForJAVABase
{
DECLARE_GENERATOR_AUTO_REGISTER() ;

public:
   rcGenForJAVA() ;
   ~rcGenForJAVA() ;

   int outputFile( int id, fileOutStream &fout, string &outputPath ) ;
} ;

class rcGenForJAVA7 : public rcGenForJAVABase
{
DECLARE_GENERATOR_AUTO_REGISTER() ;

public:
   rcGenForJAVA7() ;
   ~rcGenForJAVA7() ;

   int outputFile( int id, fileOutStream &fout, string &outputPath ) ;
} ;

#endif