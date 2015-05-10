<?php
class BaseController
{
    protected $action;
    protected $site;
    protected $vars;
    protected $viewer;

    public function __construct($viewer)
    {
        if (isset($_REQUEST['action']))
            $this->action = strtolower($_REQUEST['action']) . 'Action';
        else 
            $this->action = 'indexAction';

        if (isset($_REQUEST['site']))
            $this->site = strtolower($_REQUEST['site']);
        else 
            $this->site = 'default';

        $this->vars = array();
        $this->viewer = $viewer;
    }

    public function run()
    {
        if (in_array($this->action, get_class_methods($this))) {
            $this->{$this->action}();
        } else {
            throw new Exception(
                    'Controller doesn\'t have a handler for the action: ' .
                    $this->action);
        }
    }
}
