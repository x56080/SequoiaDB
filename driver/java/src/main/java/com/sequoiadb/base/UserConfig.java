/*
 * Copyright 2022 SequoiaDB Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except
 * in compliance with the License. You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software distributed under the License
 * is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express
 * or implied. See the License for the specific language governing permissions and limitations under
 * the License.
 */

package com.sequoiadb.base;

import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;

<<<<<<< HEAD
=======
import java.io.File;
>>>>>>> c4064a6f2c2dfdf2b1bf049c2f904b74db0494b2
import java.util.Objects;

/**
 * The user config of SequoiaDB
 */
public class UserConfig {
<<<<<<< HEAD
    private final String userName ;
    private final String password ;
=======
    private final String userName;
    private final String password;
    private final File cipherFile;
    private final String token;
>>>>>>> c4064a6f2c2dfdf2b1bf049c2f904b74db0494b2

    /**
     * Create an user config object with empty username and empty password.
     */
    public UserConfig() {
<<<<<<< HEAD
        this.userName = "";
        this.password = "";
=======
        this("", "");
>>>>>>> c4064a6f2c2dfdf2b1bf049c2f904b74db0494b2
    }

    /**
     * Create an user config object with username and password.
     *
     * @param userName The user name
     * @param password The password
     */
    public UserConfig( String userName, String password ) {
        if ( userName == null || password == null ) {
            throw new BaseException( SDBError.SDB_INVALIDARG, "User name or password is null" );
        }
        this.userName = userName;
        this.password = password;
<<<<<<< HEAD
=======
        this.cipherFile = null;
        this.token = null;
    }

    /**
     * Create an user config object with username and cipher file.
     *
     * @param userName The user name
     * @param cipherFile The cipher file
     */
    public UserConfig( String userName, File cipherFile ) {
        this( userName, cipherFile, null );
    }

    /**
     * Create an user config object with username, cipher file and token.
     *
     * @param userName The user name
     * @param cipherFile The cipher file
     * @param token The password encryption token
     */
    public UserConfig( String userName, File cipherFile, String token ) {
        if ( userName == null || cipherFile == null) {
            throw new BaseException( SDBError.SDB_INVALIDARG, "User name or cipher file is null" );
        }
        if ( !cipherFile.exists() ) {
            throw new BaseException( SDBError.SDB_FNE,
                    "File not exist: " + cipherFile.getAbsolutePath() );
        }
        if ( !cipherFile.isFile() ) {
            throw new BaseException( SDBError.SDB_INVALIDARG,
                    "It is not file: " + cipherFile.getAbsolutePath() );
        }
        this.userName = userName;
        this.password = null;
        this.cipherFile = cipherFile;
        this.token = token;
>>>>>>> c4064a6f2c2dfdf2b1bf049c2f904b74db0494b2
    }

    /**
     * @return The user name.
     */
    public String getUserName() {
        return userName;
    }

    /**
     * @return The password.
     */
    public String getPassword() {
        return password;
    }

<<<<<<< HEAD
=======
    /**
     * @return The cipher file.
     */
    public File getCipherFile() {
        return cipherFile;
    }

    /**
     * @return The token.
     */
    public String getToken() {
        return token;
    }

>>>>>>> c4064a6f2c2dfdf2b1bf049c2f904b74db0494b2
    @Override
    public boolean equals( Object o ) {
        if ( this == o ) return true;
        if ( o == null || getClass() != o.getClass() ) return false;
        UserConfig user = ( UserConfig ) o;
        return Objects.equals( userName, user.userName ) &&
<<<<<<< HEAD
                Objects.equals( password, user.password );
=======
                Objects.equals( password, user.password ) &&
                Objects.equals( cipherFile, user.cipherFile ) &&
                Objects.equals( token, user.token );
>>>>>>> c4064a6f2c2dfdf2b1bf049c2f904b74db0494b2
    }

    @Override
    public int hashCode() {
<<<<<<< HEAD
        return Objects.hash( userName, password );
=======
        return Objects.hash( userName, password, cipherFile, token );
>>>>>>> c4064a6f2c2dfdf2b1bf049c2f904b74db0494b2
    }

    @Override
    public String toString() {
        return "UserConfig{" +
                "userName='" + userName + '\'' +
                '}';
    }
}

