<?php
class TestController extends BaseController
{
    public function indexAction()
    {
	$this->vars['snapshots'] = $this->db->GetSnapshots();
	$this->render();
    }
}
