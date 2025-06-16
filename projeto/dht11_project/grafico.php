<?php
include("conexao.php");

header('Content-Type: application/json');

$sql = "SELECT * FROM leitura ORDER BY dataEntrada DESC LIMIT 20";
$result = $conn->query($sql);

if ($result === false) {
    die(json_encode(['error' => $conn->error]));
}

$data = [];

while ($row = $result->fetch_assoc()) {
    $data[] = [
        'dataEntrada' => date('Y-m-d H:i:s', strtotime($row['dataEntrada'])),
        'leitura' => round(floatval($row['leitura']), 2),
        'hora' => date('H:i:s', strtotime($row['dataEntrada']))
    ];
}

echo json_encode($data);
$conn->close();
?>