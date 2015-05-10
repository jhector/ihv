<?php
class Block
{
    public $id;
    public $alloc_snapshot_id;
    public $free_snapshot_id;
    public $usr_address;
    public $usr_size;

    public $real_address;
    public $real_size;

    public function __construct($id, $alloc, $free, $addr, $size)
    {
        $this->id = $id;
        $this->alloc_snapshot_id = $alloc;
        $this->free_snapshot_id = $free;
        $this->usr_address = $addr;
        $this->usr_size = $size;
    }

    public function AdjustDlmalloc(/* TODO $arch = 64 */)
    {
        $this->real_address = $this->usr_address - 8;
        $this->real_size = max(0x20, $this->usr_size + 8);
        $this->real_size += (0x10 - 1);
        $this->real_size -= ($this->real_size % 0x10);
    }
}
