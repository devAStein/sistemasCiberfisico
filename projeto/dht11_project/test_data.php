<?php
header('Content-Type: application/json');

include("conexao.php");

// Verifica o método da requisição
if ($_SERVER['REQUEST_METHOD'] == 'POST') {
    // Recebe os dados brutos
    $data = json_decode(file_get_contents('php://input'), true) ?: $_POST;
    
    if (isset($data['vazao']) && isset($data['total'])) {
        $vazao_m3 = floatval($data['vazao']);
        $total_m3 = floatval($data['total']);
        $dataEntrada = date("Y-m-d H:i:s");

        // Usando prepared statements para segurança
        $stmt = $conn->prepare("INSERT INTO leitura (dataEntrada, vazao_m3, total_m3) VALUES (?, ?, ?)");
        $stmt->bind_param("sdd", $dataEntrada, $vazao_m3, $total_m3);
        
        if ($stmt->execute()) {
            echo json_encode([
                'status' => 'success',
                'message' => 'Dados salvos com sucesso',
                'data' => [
                    'vazao_m3' => $vazao_m3,
                    'total_m3' => $total_m3,
                    'timestamp' => $dataEntrada
                ]
            ]);
        } else {
            echo json_encode([
                'status' => 'error',
                'message' => 'Erro ao salvar dados: ' . $conn->error
            ]);
        }
        
        $stmt->close();
    } else {
        echo json_encode([
            'status' => 'error',
            'message' => 'Parâmetros vazao ou total não recebidos',
            'received_data' => $data
        ]);
    }
} else {
    echo json_encode([
        'status' => 'error',
        'message' => 'Método não permitido. Use POST.'
    ]);
}

?>