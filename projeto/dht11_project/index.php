<?php
include("conexao.php");

if ($conn->connect_error) {
    die("Falha na conexão: " . $conn->connect_error);
}

// Inicializa os valores dos meses
$valorGrafico = array_fill(0, 12, 0);

try {
    // Preenche os valores por mês com o último total_m3 do mês
    for ($i = 1; $i <= 12; $i++) {
        $consulta = $conn->query("SELECT total_m3 FROM leitura WHERE MONTH(dataEntrada) = $i ORDER BY dataEntrada DESC LIMIT 1");
        $resultado = $consulta->fetch_assoc();
        $valorGrafico[$i - 1] = $resultado['total_m3'] ?? 0;
        $consulta->close();
    }

    // Leitura anterior (último total do mês anterior)
    $lastMonth = date('m', strtotime('first day of last month'));
    $queryAnterior = $conn->query("SELECT total_m3 FROM leitura WHERE MONTH(dataEntrada) = '$lastMonth' ORDER BY dataEntrada DESC LIMIT 1");
    $resultAnterior = $queryAnterior->fetch_assoc();
    $leituraAnterior = $resultAnterior['total_m3'] ?? 0;
    $totalAnteriorReais = $leituraAnterior * 81.45;
    $queryAnterior->close();

    // Leitura atual (último total do mês atual)
    $currentMonth = date('m');
    $queryAtual = $conn->query("SELECT total_m3 FROM leitura WHERE MONTH(dataEntrada) = '$currentMonth' ORDER BY dataEntrada DESC LIMIT 1");
    $resultAtual = $queryAtual->fetch_assoc();
    $leituraAtual = $resultAtual['total_m3'] ?? 0;
    $totalAtualReais = $leituraAtual * 81.45;
    $queryAtual->close();

    // Última leitura do ESP32
    $queryUltima = $conn->query("SELECT vazao_m3, total_m3, dataEntrada FROM leitura ORDER BY dataEntrada DESC LIMIT 1");
    $ultimaLeitura = $queryUltima->fetch_assoc();
    $queryUltima->close();

} catch (Exception $e) {
    die("Erro ao consultar o banco de dados: " . $e->getMessage());
}
?>
<!DOCTYPE html>
<html lang="pt-BR">
<head>
    <meta charset="UTF-8">
    <title>Gráfico de Leitura de Água</title>
    <script src="https://cdn.jsdelivr.net/npm/chart.js"></script>
    <style>
        body {
            font-family: Arial, sans-serif;
            text-align: center;
            padding: 30px;
            background: #f5f5f5;
        }
        h1 {
            margin-bottom: 20px;
        }
        .valores {
            display: flex;
            justify-content: center;
            gap: 60px;
            margin: 20px 0;
            flex-wrap: wrap;
        }
        .valor-box {
            background: white;
            padding: 15px 25px;
            border-radius: 8px;
            box-shadow: 0 0 10px #ccc;
            min-width: 180px;
        }
        .valor-box h3 {
            margin: 0;
            font-size: 16px;
            color: #555;
        }
        .valor-box p {
            margin: 5px 0 0;
            font-size: 20px;
            color: #222;
            font-weight: bold;
        }
        canvas {
            max-width: 900px;
            margin: auto;
            background: white;
            padding: 15px;
            border-radius: 10px;
            box-shadow: 0 0 10px #ccc;
        }
    </style>
</head>
<body>
    <h1>Consumo de Água por Mês</h1>

    <div class="valores">
        <div class="valor-box">
            <h3>Leitura Anterior</h3>
            <p><?php echo number_format($leituraAnterior, 3, ',', '.') . ' m³'; ?></p>
        </div>
        <div class="valor-box">
            <h3>Valor Anterior</h3>
            <p><?php echo 'R$ ' . number_format($totalAnteriorReais, 2, ',', '.'); ?></p>
        </div>
        <div class="valor-box">
            <h3>Leitura Atual</h3>
            <p><?php echo number_format($leituraAtual, 3, ',', '.') . ' m³'; ?></p>
        </div>
        <div class="valor-box">
            <h3>Valor Atual</h3>
            <p><?php echo 'R$ ' . number_format($totalAtualReais, 2, ',', '.'); ?></p>
        </div>
        <div class="valor-box">
            <h3>Última Vazão</h3>
            <p><?php echo number_format($ultimaLeitura['vazao_m3'], 6, ',', '.') . ' m³/min'; ?></p>
        </div>
        <div class="valor-box">
            <h3>Último Total</h3>
            <p><?php echo number_format($ultimaLeitura['total_m3'], 6, ',', '.') . ' m³'; ?></p>
        </div>
        <div class="valor-box">
            <h3>Último Registro</h3>
            <p><?php echo date('d/m/Y H:i:s', strtotime($ultimaLeitura['dataEntrada'])); ?></p>
        </div>
    </div>

    <canvas id="graficoLeitura" width="900" height="400"></canvas>

    <script>
        const ctx = document.getElementById('graficoLeitura').getContext('2d');
        const grafico = new Chart(ctx, {
            type: 'bar',
            data: {
                labels: ['Jan', 'Fev', 'Mar', 'Abr', 'Mai', 'Jun', 'Jul', 'Ago', 'Set', 'Out', 'Nov', 'Dez'],
                datasets: [{
                    label: 'Leitura de Água (m³)',
                    data: <?php echo json_encode($valorGrafico); ?>,
                    backgroundColor: 'rgba(54, 162, 235, 0.6)',
                    borderColor: 'rgba(54, 162, 235, 1)',
                    borderWidth: 2
                }]
            },
            options: {
                responsive: true,
                plugins: {
                    legend: { display: false },
                    title: {
                        display: true,
                        text: 'Consumo de Água por Mês (em m³)'
                    }
                },
                scales: {
                    y: {
                        beginAtZero: true,
                        title: {
                            display: true,
                            text: 'm³'
                        }
                    }
                }
            }
        });
    </script>
</body>
</html>
<?php
$conn->close();
?>
