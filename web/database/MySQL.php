<?php
class MySQL extends Database
{
    protected $db;

    public function __construct($host, $user, $pass, $db)
    {
        $this->db = new mysqli($host, $user, $pass, $db);
        if ($this->db->connect_errno)
            $this->RaiseException(__LINE__, $this->db->connect_error);
    }

    public function GetSnapshots($id_start = 0, $limit = 50)
    {
        if (!($stmt = $this->db->prepare(
                'SELECT * FROM snapshot WHERE snapshot_id >= ? LIMIT ?')))
            $this->RaiseException(__LINE__, $this->db->error);

        if (!$stmt->bind_param('ii', $id_start, $limit))
            $this->RaiseException(__LINE__, $stmt->error);

        if (!$stmt->execute())
            $this->RaiseException(__LINE__, $stmt->error);

        $stmt->bind_result($id, $time, $reason);
        while ($stmt->fetch())
            $snapshots[] = new Snapshot($id, $time, $reason);

        $stmt->close();

        return $snapshots;
    }

    public function GetSnapshotsWithReason($reason, $id_start = 0,
            $limit = 50)
    {
        if (!($stmt = $this->db->prepare('SELECT * FROM snapshot WHERE ' .
                'snapshot_id >= ? AND reason = ? LIMIT ?')))
            $this->RaiseException(__LINE__, $this->db->error);

        if (!$stmt->bind_param('iii', $id_start, $reason, $limit))
            $this->RaiseException(__LINE__, $stmt->error);

        if (!$stmt->execute())
            $stmt->RaiseException(__LINE__, $stmt->error);

        $stmt->bind_result($id, $time, $reason);
        while ($stmt->fetch())
            $snapshots[] = new Snapshot($id, $time, $reason);

        $stmt->close();
        return $snapshots;
    }

    public function GetAllocatedBlocks($snapshot_id, $limit = 50)
    {
        if (!($stmt = $this->db->prepare('SELECT * FROM block WHERE ' .
                'alloc_snapshot_id <= ? AND (free_snapshot_id > ? OR ' .
                'free_snapshot_id IS NULL)  LIMIT ?')))
            $this->RaiseException(__LINE__, $this->db->error);

        if (!$stmt->bind_param('iii', $snapshot_id, $snapshot_id, $limit))
            $this->RaiseException(__LINE__, $stmt->error);

        if (!$stmt->execute())
            $stmt->RaiseException(__LINE__, $stmt->error);

        $stmt->bind_result($id, $alloc_id, $free_id, $addr, $size);
        while ($stmt->fetch())
            $blocks[] = new Block($id, $alloc_id, $free_id, $addr, $size);

        $stmt->close();
        return $blocks;
    }

    private function RaiseException($line, $error)
    {
        throw new Exception('Database error[' . $line . ']: ' . $error);
    }
}
