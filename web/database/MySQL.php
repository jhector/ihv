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
        while ($row = $stmt->fetch()) {
            $snapshots[] = new Snapshot($id, $time, $reason);
        }

        $stmt->close();

        return $snapshots;
    }

    private function RaiseException($line, $error)
    {
        throw new Exception('Database error[' . $line . ']: ' . $error);
    }
}
