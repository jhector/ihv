<?php
class TestController extends BaseController
{
    public function indexAction()
    {
	$this->vars['snapshots'] = $this->db->GetSnapshotsWithReason(1);
	$blocks = $this->db->GetAllocatedBlocks(14);

	foreach ($blocks as $block)
            $block->AdjustDlmalloc();

	$this->vars['blocks'] = $blocks;
	$this->render();
    }
}
