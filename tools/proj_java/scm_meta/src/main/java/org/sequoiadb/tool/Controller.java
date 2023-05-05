/**
 * Copyright (C) 2023 SequoiaDB Inc.
 * <p>
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * <p>
 * http://www.apache.org/licenses/LICENSE-2.0
 * <p>
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
package org.sequoiadb.tool;

public class Controller {
    private static volatile boolean running = true;
    private static volatile boolean hasErr = false;

    public static boolean isRunning() {
        return running;
    }

    public static void stop() {
        running = false;
    }

    public static void setHasErr(boolean hasErr) {
        Controller.hasErr = hasErr;
    }

    public static boolean hasErr() {
        return hasErr;
    }
}
