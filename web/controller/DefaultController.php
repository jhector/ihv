<?php
class DefaultController extends BaseController
{
    public function indexAction()
    {
        $template = $this->viewer->loadTemplate($this->site . '.twig');
        echo $template->render($this->vars);

        exit(0);
    }
}
