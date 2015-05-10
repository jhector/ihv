<?php
error_reporting(E_ALL & ~E_NOTICE);

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

    foreach ($config['database_handler'] as $db_handler) {
        $path = __DIR__ . '/database/' . $db_handler . '.php';
        if (file_exists($path))
            include($path);
        else
            throw new Exception('Database handler: ' . $db_handler. ' missing');
    }

    foreach ($config['assets'] as $asset) {
        $path = __DIR__ . '/assets/' . $asset. '.php';
        if (file_exists($path))
            include($path);
        else
            throw new Exception('Asset class: ' . $asset. ' missing');
    }

    $db = new MySQL('localhost', 'root', 'root', 'ihv');

    $front = new DefaultController($viewer, $db);
    $controller = ucfirst(strtolower($_REQUEST['site'])) . 'Controller';

    if (class_exists($controller))
        $front = new $controller($viewer, $db);

    $front->run();
} catch (Exception $e) {
    $template = $viewer->loadTemplate('error.twig');
    echo $template->render(array(
        'error' => $e->GetMessage()
    ));
}
