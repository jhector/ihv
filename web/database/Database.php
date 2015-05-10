<?php
abstract class Database
{
    abstract public function GetSnapshots($id_start = 1, $limit = 50);
    abstract public function GetSnapshotsWithReason($reason, $id_start = 1,
            $limit = 50);
    abstract public function GetAllocatedBlocks($snapshot_id, $limit = 50);
}
