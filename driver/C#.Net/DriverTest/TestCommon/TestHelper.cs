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

   Source File Name = TestHelper.cs

   Descriptive Name = N/A

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who         Description
   ====== =========== =========== =============================================
          23/07/2025  fangjiabin  Initial Draft

   Last Changed =

*******************************************************************************/
﻿using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace DriverTest.TestCommon
{
    public class TestHelper
    {
        
        public static bool ByteArrayEqual(byte[] left, byte[] right)
        {
            if (left == null && right == null)
            {
                return true;
            }
            if (left == null || right == null)
            {
                return false;
            }
            if (left.Length != right.Length)
            {
                return false;
            }
            int compareLength = left.Length;
            for (int i = 0; i < compareLength; i++)
            {
                if (left[i] != right[i])
                {
                    return false;
                }
            }
            return true;
        }

        public static int ByteArrayCompare(byte[] left, byte[] right)
        {
            if (left == null && right == null)
            {
                return 0;
            }
            if (left != null && right == null)
            {
                return 1;
            }
            if (left == null && right != null)
            {
                return -1;
            }
            int compareLength = Math.Min(left.Length, right.Length);
            for (int i = 0; i < compareLength; i++)
            {
                if (left[i] > right[i])
                {
                    return 1;
                }
                else if (left[i] < right[i])
                {
                    return -1;
                }
            }
            if (left.Length - compareLength > 0)
            {
                return 1;
            }
            if (right.Length - compareLength > 0)
            {
                return -1;
            }
            return 0;
        }
    }
}
