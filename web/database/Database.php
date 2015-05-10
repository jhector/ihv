<?php
abstract class Database
{
    abstract public function GetSnapshots($id_start = 1, $limit = 50);
}
