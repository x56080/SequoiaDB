<?php
/*******************************************************************************
   Copyright (C) 2012-2018 SequoiaDB Ltd.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

   http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*******************************************************************************/

/** \file class_timestamp.php
    \brief timestamp class
 */

/**
 * Class for create an object of the timestamp type
 */
class SequoiaTimestamp
{
   /**
    * Constructor.
    *
    * @param string $timestamp is timestamp string
    *
    * @code
    * $timeObj = new SequoiaTimestamp( '2000-01-01-12.30.20.123456' ) ;
    * $arr = array( 'time' => $timeObj ) ; // json ==> { "time": { "$timestamp": "2000-01-01-12.30.20.123456" } }
    * @endcode
   */
   public function __construct( $timestamp ){}

   /**
    * PHP Magic Methods, the class as string output.
    *
    * @return timestamp string
    *
    * @code
    * $timeObj = new SequoiaTimestamp( '2000-01-01-12.30.20.123456' ) ;
    * echo $timeObj ; // output ==> 2000-01-01-12.30.20.123456
    * @endcode
    */
   public function __toString(){}
}