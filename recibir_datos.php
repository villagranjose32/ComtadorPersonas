<?php
if ($_SERVER["REQUEST_METHOD"] == "POST") {
    $cantidad = intval($_POST['cantidad']);

    file_put_contents("datos.json", json_encode(["cantidad" => $cantidad]));

    echo "Datos recibidos correctamente: $cantidad personas";
} else {
    echo "Método no permitido";
}
?>
