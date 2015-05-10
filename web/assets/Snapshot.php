<?php
class Snapshot
{
    public $id;
    public $time;
    public $reason;

    public function __construct($id, $time, $reason)
    {
        $this->id = $id;
        $this->time = $time;
        $this->reason = $reason;
    }
}
