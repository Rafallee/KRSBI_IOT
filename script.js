const statusDot = document.getElementById("statusDot");
const statusText = document.getElementById("statusText");

const shootBtn = document.getElementById("btnShoot");

let client = null;
let connected = false;

// MQTT Broker
const MQTT_HOST = "wss://broker.hivemq.com:8884";

// Variabel untuk menyimpan perintah aktif
let activeCommand = {
  x: 0,
  y: 0,
  rot: 0,
};

// =========================
// LOG
// =========================
function addLog(text) {
  const log = document.getElementById("cmdLog");

  if (log.innerHTML.includes("-- AWAITING CONNECTION --")) {
    log.innerHTML = "";
  }

  const div = document.createElement("div");
  div.className = "log-entry";

  div.textContent = `${new Date().toLocaleTimeString()} - ${text}`;

  log.appendChild(div);
  log.scrollTop = log.scrollHeight;
}

// =========================
// MQTT CONNECT
// =========================
function connectMQTT() {
  addLog("CONNECTING MQTT...");

  const options = {
    hostname: 'broker.hivemq.com',
    port: 8884,
    protocol: 'wss',
    path: '/mqtt',
    clientId: 'webClient_' + Math.random().toString(16).substr(2, 8),
    clean: true,
    reconnectPeriod: 1000,
    connectTimeout: 4000,
  };

  client = mqtt.connect(options);

  client.on("connect", () => {
    connected = true;

    if (statusDot) statusDot.classList.add("online");
    if (statusText) statusText.innerText = "ONLINE";

    addLog("MQTT CONNECTED");
  });

  client.on("close", () => {
    connected = false;

    if (statusDot) statusDot.classList.remove("online");
    if (statusText) statusText.innerText = "OFFLINE";

    addLog("MQTT DISCONNECTED");

    setTimeout(() => {
      connectMQTT();
    }, 3000);
  });

  client.on("error", (err) => {
    console.error("MQTT Error:", err);
    addLog("ERROR : " + (err.message || err));
  });

  client.on("offline", () => {
    connected = false;
    if (statusDot) statusDot.classList.remove("online");
    if (statusText) statusText.innerText = "OFFLINE";
    addLog("MQTT OFFLINE");
  });
}

// =========================
// PUBLISH COMMAND
// =========================
function publishCommand(action, speed = 150) {
  if (!connected) return;

  const payload = {
    action: action,
    speed: Number(speed),
  };

  client.publish("rafly/krsbi_iot/cmd", JSON.stringify(payload));

  addLog(`CMD | ACTION:${action} SPEED:${speed}`);
}

// =========================
// FUNGSI GERAK DASAR
// =========================
function maju(speed) {
  publishCommand("maju", speed);
}

function mundur(speed) {
  publishCommand("mundur", speed);
}

function geserKiri(speed) {
  publishCommand("geserKiri", speed);
}

function geserKanan(speed) {
  publishCommand("geserKanan", speed);
}

function rotasiKiri(speed) {
  publishCommand("rotasiKiri", speed);
}

function rotasiKanan(speed) {
  publishCommand("rotasiKanan", speed);
}

function stopRobot() {
  publishCommand("stop", 0);
}

// =========================
// KICK / SHOOT
// =========================
function publishKick() {
  if (!connected) {
    addLog("ERROR: Not connected to MQTT");
    return;
  }

  publishCommand("kick", 0);
  addLog("KICK SENT!");
}

if (shootBtn) {
  shootBtn.addEventListener("click", publishKick);
}

// =========================
// DPAD BUTTON DENGAN LOGIKA MOTOR LANGSUNG
// =========================
const dpadButtons = document.querySelectorAll(".dpad-btn");

dpadButtons.forEach((btn) => {
  btn.addEventListener("pointerdown", (e) => {
    e.preventDefault();

    const command = btn.dataset.command;
    const speed = parseInt(btn.dataset.speed) || 100;

    // Eksekusi perintah berdasarkan data-command
    switch (command) {
      case "maju":
        maju(speed);
        break;
      case "mundur":
        mundur(speed);
        break;
      case "kiri":
        geserKiri(speed);
        break;
      case "kanan":
        geserKanan(speed);
        break;
      case "rotasi-kiri":
        rotasiKiri(speed);
        break;
      case "rotasi-kanan":
        rotasiKanan(speed);
        break;
      case "stop":
        stopRobot();
        break;
      default:
        stopRobot();
    }

    btn.classList.add("active");
  });

  btn.addEventListener("pointerup", () => {
    // Jangan stop jika tombol stop sendiri
    const command = btn.dataset.command;
    if (command !== "stop") {
      stopRobot();
    }
    btn.classList.remove("active");
  });

  btn.addEventListener("pointerleave", () => {
    const command = btn.dataset.command;
    if (command !== "stop") {
      stopRobot();
    }
    btn.classList.remove("active");
  });

  btn.addEventListener("touchend", () => {
    const command = btn.dataset.command;
    if (command !== "stop") {
      stopRobot();
    }
    btn.classList.remove("active");
  });
});

// =========================
// KEYBOARD CONTROLS (WASD + Space)
// =========================
const keysPressed = {};

document.addEventListener("keydown", (e) => {
  const key = e.key.toLowerCase();
  
  // Prevent default browser behavior untuk space
  if (key === " ") {
    e.preventDefault();
  }
  
  keysPressed[key] = true;

  // Handle single key presses
  if (key === "w") {
    maju(200);
    addLog("KEYBOARD: W (MAJU)");
  } else if (key === "s") {
    mundur(200);
    addLog("KEYBOARD: S (MUNDUR)");
  } else if (key === "a") {
    geserKiri(150);
    addLog("KEYBOARD: A (GESER KIRI)");
  } else if (key === "d") {
    geserKanan(150);
    addLog("KEYBOARD: D (GESER KANAN)");
  } else if (key === " ") {
    publishKick();
    addLog("KEYBOARD: SPACE (KICK)");
  }
});

document.addEventListener("keyup", (e) => {
  const key = e.key.toLowerCase();
  keysPressed[key] = false;

  // Stop robot ketika semua key release
  if (key === "w" || key === "s" || key === "a" || key === "d") {
    stopRobot();
    addLog("KEYBOARD: RELEASE (STOP)");
  }
});

// =========================
// START MQTT
// =========================
connectMQTT();
