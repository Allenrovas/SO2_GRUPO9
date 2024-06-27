import React, { useEffect, useState } from 'react';
import axios from 'axios';
import { Pie } from 'react-chartjs-2';
import { Chart, ArcElement, Tooltip, Legend } from 'chart.js/auto'; // Importar desde chart.js/auto para Chart.js 3.x
import './App.css';

Chart.register(ArcElement, Tooltip, Legend);

function App() {
  const [data, setData] = useState([]);

  useEffect(() => {
    const fetchProcessLogs = async () => {
      try {
        const response = await axios.get('http://localhost:5000/process_logs');
        setData(response.data);
      } catch (error) {
        console.error('Error al obtener los datos de los procesos:', error);
      }
    };

    fetchProcessLogs(); // Solicitud al cargar la página

    const interval = setInterval(() => {
      fetchProcessLogs();
    }, 3000); // Solicitud cada 3 segundos

    return () => clearInterval(interval); // Limpiar intervalo al desmontar el componente
  }, []);

  const aggregatedData = data.reduce((acc, log) => {
    const { pid, nombre, tamanio_memoria } = log;
    if (!acc[pid]) {
      acc[pid] = { pid, nombre, tamanio_memoria: 0, count: 0 };
    }
    acc[pid].tamanio_memoria += tamanio_memoria;
    acc[pid].count++;
    return acc;
  }, {});

  const sortedData = Object.values(aggregatedData).sort((a, b) => b.tamanio_memoria - a.tamanio_memoria);

  const memoryData = sortedData.map(log => (log.tamanio_memoria / 1048576).toFixed(2)); // Convertir bytes a MB
  //Top 10 para el gráfico y agrupar los demás en "Otros"
  /*const labels = sortedData.slice(0, 10).map(log => log.pid);
  labels.push('Otros');
  memoryData.splice(10, memoryData.length - 10);
  memoryData.push(sortedData.slice(10).reduce((acc, log) => acc + log.tamanio_memoria, 0) / 1048576);*/
  //Gráfico Top 10 y agrupar los demáss en "Otros" y "No asignada"; debe ser sobre el porcentaje de memoria, el tamanio total de la memoria es 8 GB
  const labels = sortedData.slice(0, 10).map(log => log.pid);
  labels.push('Otros');
  labels.push('No asignada');
  memoryData.splice(10, memoryData.length - 10);
  memoryData.push(sortedData.slice(10).reduce((acc, log) => acc + log.tamanio_memoria, 0) / 1048576);
  memoryData.push(8192 - sortedData.reduce((acc, log) => acc + log.tamanio_memoria, 0) / 1048576);

  


  const pieData = {
    labels: labels,
    datasets: [
      {
        data: memoryData,
        backgroundColor: [
          '#FF6384',
          '#36A2EB',
          '#FFCE56',
          '#FF6347',
          '#6A2EAB',
          '#FF7F50',
          '#6A5ACD'
        ],
        hoverBackgroundColor: [
          '#FF6384',
          '#36A2EB',
          '#FFCE56',
          '#FF6347',
          '#6A2EAB',
          '#FF7F50',
          '#6A5ACD'
        ]
      }
    ]
  };

  return (
    <div className="App">
      <nav className="navbar navbar-dark bg-dark">
        <a className="navbar-brand" href="#">PROYECTO 1 - SO2 - 202000343 - 202004745</a>
      </nav>
      
      <div className="container content">
        <div className="table-container">
          
        <h2>Procesos</h2>
          <table className="table table-striped">
            <thead>
              <tr>
                <th>PID</th>
                <th>Nombre</th>
                <th>Tamaño Memoria (MB)</th>
                <th>Porcentaje de Memoria</th>
              </tr>
            </thead>
            <tbody>
              {sortedData.map((log) => (
                <tr key={log.pid}>
                  <td>{log.pid}</td>
                  <td>{log.nombre}</td>
                  <td>{(log.tamanio_memoria / 1048576).toFixed(2)}</td>
                  <td>{((log.tamanio_memoria / 8192 / 1048576) * 100).toFixed(2)} %</td>
                </tr>
              ))}
            </tbody>
          </table>
        </div>
        <div className="chart-container">
          
          <h2>Gráfica de Porcentaje de Uso de Memoria</h2>
          <Pie data={pieData} />
        </div>
      </div>

      <div>
        <h2>Solicitudes</h2>
        <table className="table table-striped">
          <thead>
            <tr>
              <th>PID</th>
              <th>Nombre</th>
              <th>Llamada</th>
              <th>Tamaño Memoria (MB)</th>
              <th>Fecha y Hora</th>
            </tr>
          </thead>
          <tbody>
            {data.map((log) => (
              <tr key={log.pid}>
                <td>{log.pid}</td>
                <td>{log.nombre}</td>
                <td>{log.llamada}</td>
                <td>{(log.tamanio_memoria / 1048576).toFixed(2)}</td>
                <td>{new Date(log.fechahora).toLocaleString()}</td>
              </tr>
            ))}
          </tbody>
        </table>
      </div>
    </div>
  );
}

export default App;