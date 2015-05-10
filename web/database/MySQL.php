<?php
class MySQL extends Database
{
    protected $conn;

    public function __construct($host, $user, $pass, $db)
    {
        $this->conn = mysql_connect($host, $user, $pass);
        if (!$this->conn)
            throw new Exception('Database error: ' . mysql_error());

        if (!mysql_select_db($db))
            throw new Exception('Database error: ' . mysql_error());
    }
}
