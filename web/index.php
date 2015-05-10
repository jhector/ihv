<?php
error_reporting(E_ALL);

require_once __DIR__ . '/vendor/autoload.php';
require_once __DIR__ . '/include/config.php';

// set up twig
$viewer = new Twig_Environment(new Twig_Loader_Filesystem(__DIR__ . '/views'));

try {
    foreach ($config['controllers'] as $controller) {
        $path = __DIR__ . '/controller/' . $controller . '.php';
        if (file_exists($path))
            include($path);
        else
            throw new Exception('Controller: ' . $controller . ' missing');
    }

    $front = new DefaultController($viewer);
    $controller = ucfirst(strtolower($_REQUEST['site'])) . 'Controller';

    if (class_exists($controller))
        $front = new $controller($viewer);

    $front->run();
} catch (Exception $e) {
    $template = $viewer->loadTemplate('error.twig');
    echo $template->render(array(
        'error' => $e->GetMessage()
    ));
}
