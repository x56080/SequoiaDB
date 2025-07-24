/*******************************************************************************

   Copyright (C) 2011-Present SequoiaDB Ltd.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

      http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

   Source File Name = typeTest.php

   Descriptive Name = N/A

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who         Description
   ====== =========== =========== =============================================
          23/07/2025  fangjiabin  Initial Draft

   Last Changed =

*******************************************************************************/
<?php
class typeTest extends PHPUnit_Framework_TestCase
{
   public function test_type()
   {
      $oid   = new SequoiaID( '123456789012345678901234' ) ;
      $this -> assertEquals( '123456789012345678901234', $oid -> __toString(), 'Oid错误' ) ;
      
      $int64 = new SequoiaINT64( '123' ) ;
      $this -> assertEquals( '123', $int64 -> __toString(), 'Int64错误' ) ;
 
      $date  = new SequoiaDate( '1991-11-27' ) ;
      $this -> assertEquals( '1991-11-27', $date -> __toString(), 'Date错误' ) ;
      
      $date1 = new SequoiaDate( '1991-11-27T16:30:12.123456Z' ) ;
      $this -> assertEquals( '1991-11-28', $date1 -> __toString(), 'Date错误' ) ;
      
      $date2 = new SequoiaDate( '2051222400000' ) ;
      $this -> assertEquals( '2035-01-01', $date2 -> __toString(), 'Date错误' ) ;

      $time  = new SequoiaTimestamp( '1991-11-27-12.30.20.123456' ) ;
      $this -> assertEquals( '1991-11-27-12.30.20.123456', $time -> __toString(), 'Timestamp错误' ) ;
      
      $regex = new SequoiaRegex( 'a', 'i' ) ;
      $this -> assertEquals( '/a/i', $regex -> __toString(), 'Regex错误' ) ;
      
      $bin   = new SequoiaBinary( 'aGVsbG8=', '1' ) ;
      $this -> assertEquals( '(1)aGVsbG8=', $bin -> __toString(), 'Binary错误' ) ;
      
      $min   = new SequoiaMinKey() ;
      $this -> assertTrue( is_object( $min ) && is_a( $min, 'SequoiaMinKey' ), 'MinKey错误' ) ;
      
      $max   = new SequoiaMaxKey() ;
      $this -> assertTrue( is_object( $max ) && is_a( $max, 'SequoiaMaxKey' ), 'MaxKey错误' ) ;
      
      $decimal1 = new SequoiaDecimal( '10.00000000000000000000000001' ) ;
      $this -> assertTrue( is_object( $decimal1 ) && is_a( $decimal1, 'SequoiaDecimal' ), 'Decimal错误' ) ;
      $this -> assertEquals( '10.00000000000000000000000001', $decimal1 -> __toString(), 'Decimal错误' ) ;
      
      $decimal2 = new SequoiaDecimal( 100, 6, 3 ) ;
      $this -> assertTrue( is_object( $decimal2 ) && is_a( $decimal2, 'SequoiaDecimal' ), 'Decimal错误' ) ;
      $this -> assertEquals( '100.000', $decimal2 -> __toString(), 'Decimal错误' ) ;
      
      $decimal3 = new SequoiaDecimal( 100.01, 5, 2 ) ;
      $this -> assertTrue( is_object( $decimal3 ) && is_a( $decimal3, 'SequoiaDecimal' ), 'Decimal错误' ) ;
      $this -> assertEquals( '100.01', $decimal3 -> __toString(), 'Decimal错误' ) ;
   }
}
?>
