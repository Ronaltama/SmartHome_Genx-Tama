// main.js

const options = {
  username: "",
  password: "",
};

// Pastikan menggunakan wss:// untuk koneksi browser
const client = mqtt.connect("wss://broker.hivemq.com:8884/mqtt", options);

// --- DHT22 Chart Data Storage for Living Room (Data from Bedroom 2) ---
let livingRoomTempData = [];
let livingRoomHumData = [];
let livingRoomTimeLabels = [];
const LIVING_ROOM_MAX_DATA_POINTS = 10; // Hanya menyimpan 10 data terakhir untuk chart Living Room
const LIVING_ROOM_DHT_STORAGE_KEY = "living_room_dht_chart_data"; // Key for sessionStorage

let livingRoomDhtChartInstance = null; // Instance Chart.js untuk Living Room

// Fungsi untuk memuat data dari sessionStorage untuk Living Room
function loadLivingRoomDhtDataFromStorage() {
  const storedData = sessionStorage.getItem(LIVING_ROOM_DHT_STORAGE_KEY);
  if (storedData) {
    const parsedData = JSON.parse(storedData);
    livingRoomTempData = parsedData.tempData || [];
    livingRoomHumData = parsedData.humData || [];
    livingRoomTimeLabels = parsedData.timeLabels || [];
    console.log("Loaded Living Room DHT22 data from session storage.");
  }
}

// Fungsi untuk menyimpan data ke sessionStorage untuk Living Room
function saveLivingRoomDhtDataToStorage() {
  const dataToStore = {
    tempData: livingRoomTempData,
    humData: livingRoomHumData,
    timeLabels: livingRoomTimeLabels,
  };
  sessionStorage.setItem(
    LIVING_ROOM_DHT_STORAGE_KEY,
    JSON.stringify(dataToStore)
  );
}

// Fungsi untuk menginisialisasi Chart.js untuk Living Room
function initializeLivingRoomDhtChart() {
  const chartCanvas = document.getElementById("livingRoomDhtChart");
  if (!chartCanvas) {
    // console.warn("Canvas element with ID 'livingRoomDhtChart' not found. Chart will not be initialized on this page.");
    return; // Keluar jika elemen chart tidak ada di halaman ini
  }

  const ctx = chartCanvas.getContext("2d");

  if (livingRoomDhtChartInstance) {
    livingRoomDhtChartInstance.destroy(); // Hancurkan instance chart yang lama jika ada
  }

  livingRoomDhtChartInstance = new Chart(ctx, {
    type: "line",
    data: {
      labels: livingRoomTimeLabels,
      datasets: [
        {
          label: "Temperature (°C)",
          data: livingRoomTempData,
          borderColor: "#b02a37", // Warna merah dari tema Anda
          backgroundColor: "rgba(176, 42, 55, 0.1)",
          yAxisID: "y",
          tension: 0.4, // Kurva halus
          pointRadius: 3, // Ukuran titik data
          pointBackgroundColor: "#b02a37",
        },
        {
          label: "Humidity (%)",
          data: livingRoomHumData,
          borderColor: "#007bff", // Warna biru
          backgroundColor: "rgba(0, 123, 255, 0.1)",
          yAxisID: "y1",
          tension: 0.4, // Kurva halus
          pointRadius: 3,
          pointBackgroundColor: "#007bff",
        },
      ],
    },
    options: {
      responsive: true,
      maintainAspectRatio: false,
      plugins: {
        title: {
          display: true,
          text: "Real-time Temperature and Humidity", // Sesuaikan judul
          font: {
            size: 16,
            weight: "bold",
          },
          color: "#333",
        },
        tooltip: {
          mode: "index",
          intersect: false,
          callbacks: {
            label: function (context) {
              let label = context.dataset.label || "";
              if (label) {
                label += ": ";
              }
              if (context.parsed.y !== null) {
                label +=
                  context.parsed.y +
                  (context.dataset.label.includes("Temperature") ? "°C" : "%");
              }
              return label;
            },
          },
        },
        legend: {
          display: true,
          position: "bottom",
          labels: {
            color: "#555",
          },
        },
      },
      scales: {
        x: {
          title: {
            display: true,
            text: "Time",
            color: "#555",
          },
          grid: {
            color: "#eee",
          },
          ticks: {
            color: "#777",
          },
        },
        y: {
          type: "linear",
          position: "left",
          title: {
            display: true,
            text: "Temperature (°C)",
            color: "#b02a37",
          },
          grid: {
            color: "#eee",
          },
          ticks: {
            color: "#b02a37",
          },
          min: 0, // Batasan suhu minimum
          max: 50, // Batasan suhu maksimum
        },
        y1: {
          type: "linear",
          position: "right",
          title: {
            display: true,
            text: "Humidity (%)",
            color: "#007bff",
          },
          grid: {
            drawOnChartArea: false, // Hanya tampilkan grid Y pertama
            color: "#eee",
          },
          ticks: {
            color: "#007bff",
          },
          min: 0, // Batasan kelembaban minimum
          max: 100, // Batasan kelembaban maksimum
        },
      },
    },
  });
}

// Fungsi untuk menambahkan data baru ke history dan mengupdate chart Living Room
function updateLivingRoomDhtChartData(temperature, humidity) {
  const timestamp = new Date().toLocaleTimeString();

  // Tambahkan data baru
  livingRoomTempData.push(parseFloat(temperature));
  livingRoomHumData.push(parseFloat(humidity));
  livingRoomTimeLabels.push(timestamp);

  // Pertahankan hanya LIVING_ROOM_MAX_DATA_POINTS terakhir
  if (livingRoomTempData.length > LIVING_ROOM_MAX_DATA_POINTS) {
    livingRoomTempData.shift();
    livingRoomHumData.shift();
    livingRoomTimeLabels.shift();
  }

  // Simpan ke storage setelah setiap update
  saveLivingRoomDhtDataToStorage();

  if (livingRoomDhtChartInstance) {
    livingRoomDhtChartInstance.update(); // Perbarui chart
  } else {
    // Jika chart belum diinisialisasi (misalnya saat DOMContentLoaded belum fired), inisialisasi sekarang
    initializeLivingRoomDhtChart();
  }
}

// --- MQTT Client Handlers ---

client.on("connect", () => {
  console.log("MQTT Connected");

  // Subscribe semua elemen yang punya data-topic-status
  document.querySelectorAll("[data-topic-status]").forEach((el) => {
    const topic = el.getAttribute("data-topic-status");
    client.subscribe(topic, (err) => {
      if (err) console.error("Failed to subscribe to", topic, err);
    });
  });

  // Subscribe topik untuk sensor suhu DHT22 Bedroom 2 (data untuk chart Living Room)
  client.subscribe("bedroom2/dht22/temperature", (err) => {
    if (err)
      console.error("Failed to subscribe to bedroom2/dht22/temperature", err);
  });

  // Subscribe topik untuk sensor kelembaban DHT22 Bedroom 2 (data untuk chart Living Room)
  client.subscribe("bedroom2/dht22/humidity", (err) => {
    if (err)
      console.error("Failed to subscribe to bedroom2/dht22/humidity", err);
  });

  // Subscribe topik untuk sensor asap di kitchen
  client.subscribe("kitchen/gas/alarm", (err) => {
    if (err) console.error("Failed to subscribe to kitchen/gas/alarm", err);
  });

  // Subscribe topics for Living Room devices
  client.subscribe("livingroom/light/status", (err) => {
    if (err)
      console.error("Failed to subscribe to livingroom/light/status", err);
  });
  client.subscribe("livingroom/fan/status", (err) => {
    if (err) console.error("Failed to subscribe to livingroom/fan/status", err);
  });
});

let currentBedroom2TemperatureForLivingRoomChart = null; // Variabel baru
let currentBedroom2HumidityForLivingRoomChart = null; // Variabel baru

client.on("message", (topic, message) => {
  const payload = message.toString();
  // console.log(`Received message: Topic=${topic}, Payload=${payload}`); // Debugging: lihat pesan masuk

  // Update tombol switch status
  document.querySelectorAll("[data-topic-status]").forEach((el) => {
    if (el.getAttribute("data-topic-status") === topic) {
      el.checked = payload === "1";
    }
  });

  // Handle DHT22 Temperature Bedroom 2
  if (topic === "bedroom2/dht22/temperature") {
    const tempValue = parseFloat(payload);
    if (!isNaN(tempValue)) {
      // Perbarui tampilan suhu di halaman Bedroom jika ada
      document
        .querySelectorAll('[data-topic-temp="bedroom2/dht22/temperature"]')
        .forEach((el) => {
          el.textContent = `${tempValue}°C`;
        });

      // Simpan nilai suhu untuk chart Living Room
      currentBedroom2TemperatureForLivingRoomChart = tempValue;

      // Panggil update chart Living Room jika kedua nilai sudah tersedia
      if (
        currentBedroom2TemperatureForLivingRoomChart !== null &&
        currentBedroom2HumidityForLivingRoomChart !== null
      ) {
        updateLivingRoomDhtChartData(
          currentBedroom2TemperatureForLivingRoomChart,
          currentBedroom2HumidityForLivingRoomChart
        );
        currentBedroom2TemperatureForLivingRoomChart = null; // Reset untuk menunggu pasangan data berikutnya
        currentBedroom2HumidityForLivingRoomChart = null;
      }
    } else {
      console.warn("Invalid temperature payload:", payload);
    }
  }

  // Handle DHT22 Humidity Bedroom 2
  if (topic === "bedroom2/dht22/humidity") {
    const humValue = parseFloat(payload);
    if (!isNaN(humValue)) {
      // Simpan nilai kelembaban untuk chart Living Room
      currentBedroom2HumidityForLivingRoomChart = humValue;

      // Panggil update chart Living Room jika kedua nilai sudah tersedia
      if (
        currentBedroom2TemperatureForLivingRoomChart !== null &&
        currentBedroom2HumidityForLivingRoomChart !== null
      ) {
        updateLivingRoomDhtChartData(
          currentBedroom2TemperatureForLivingRoomChart,
          currentBedroom2HumidityForLivingRoomChart
        );
        currentBedroom2TemperatureForLivingRoomChart = null; // Reset untuk menunggu pasangan data berikutnya
        currentBedroom2HumidityForLivingRoomChart = null;
      }
    } else {
      console.warn("Invalid humidity payload:", payload);
    }
  }

  // Handle Smoke Detector status for Kitchen
  if (topic === "kitchen/gas/alarm") {
    const smokeStatusElement = document.querySelector(".smoke-status");
    if (smokeStatusElement) {
      if (payload === "1") {
        smokeStatusElement.textContent = "Smoke Detected!";
        smokeStatusElement.classList.add("smoke-alert");
        smokeStatusElement.style.color = "red";
      } else {
        smokeStatusElement.textContent = "No Smoke";
        smokeStatusElement.classList.remove("smoke-alert");
        smokeStatusElement.style.color = "green";
      }
    }
  }
});

// Saat DOMContentLoaded: Inisialisasi chart dan event listener
window.addEventListener("DOMContentLoaded", () => {
  // Load data dari storage untuk chart Living Room
  loadLivingRoomDhtDataFromStorage();
  // Inisialisasi chart Living Room dengan data yang dimuat (jika ada)
  initializeLivingRoomDhtChart();

  // Event listener untuk tombol switch
  document.querySelectorAll("[data-topic]").forEach((el) => {
    el.addEventListener("change", () => {
      const topicSet = el.getAttribute("data-topic");
      const topicStatus = el.getAttribute("data-topic-status");
      const value = el.checked ? "1" : "0";

      // Publish ke topic /set
      client.publish(topicSet, value, { retain: true });

      // Publish juga ke /status (optional jika perangkat tidak feedback otomatis)
      if (topicStatus) {
        client.publish(topicStatus, value, { retain: true });
      }
    });
  });
});

client.on("error", (err) => {
  console.error("MQTT error:", err);
});

client.on("close", () => {
  console.log("MQTT disconnected.");
});

client.on("reconnect", () => {
  console.log("MQTT attempting to reconnect...");
});
