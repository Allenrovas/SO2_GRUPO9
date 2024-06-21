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
  }, []);

  const memoryData = data.map(log => log.tamanio_memoria);
  const labels = data.map(log => `PID ${log.pid}`);

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
          <table className="table table-striped">
            <thead>
              <tr>
                <th>PID</th>
                <th>Nombre</th>
                <th>Tamaño Memoria</th>
                <th>Porcentaje de Memoria</th>
              </tr>
            </thead>
            <tbody>
              {data.map((log) => (
                <tr key={log.pid}>
                  <td>{log.pid}</td>
                  <td>{log.nombre}</td>
                  <td>{log.tamanio_memoria}</td>
                  <td>{8192 / log.tamanio_memoria} %</td>

                </tr>
              ))}
            </tbody>
          </table>
        </div>

        <div className="chart-container">
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
              <th>Tamaño Memoria</th>
              <th>Fecha y Hora</th>
            </tr>
          </thead>
          <tbody>
            {data.map((log) => (
              <tr key={log.pid}>
                <td>{log.pid}</td>
                <td>{log.nombre}</td>
                <td>{log.llamada}</td>
                <td>{log.tamanio_memoria}</td>
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
