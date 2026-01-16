#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <ESPmDNS.h>
#include <SPIFFS.h>
#include <ESP32Servo.h>

// ====================== 硬件引脚定义（ESP32-S3 SuperMini） ======================
// 电机驱动引脚（使用L298N或TB6612）
#define MOTOR_A1 14  // 右电机正转
#define MOTOR_A2 15  // 右电机反转
#define MOTOR_B1 16  // 左电机正转
#define MOTOR_B2 17  // 左电机反转

// 电机使能引脚（PWM速度控制）
#define MOTOR_A_EN 18  // 右电机速度
#define MOTOR_B_EN 19  // 左电机速度

// 舵机引脚
#define SERVO_PIN 13  // 舵机信号线

// WS2812 LED灯带
#define LED_PIN 21
#define LED_COUNT 8
Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

// 超声波传感器
#define TRIG_PIN 39
#define ECHO_PIN 40

// 按键引脚
#define BUTTON_PIN 0  // BOOT按钮

// ====================== WiFi 热点配置 ======================
const char* apSSID = "ESP32-SmartCar";
const char* apPassword = "12345678";
const IPAddress localIP(192, 168, 4, 1);
const IPAddress gateway(192, 168, 4, 1);
const IPAddress subnet(255, 255, 255, 0);

DNSServer dnsServer;
WebServer server(80);

// ====================== 全局变量 ======================
int carSpeed = 200;    // PWM速度 0-255
int servoAngle = 90;   // 舵机角度 0-180
bool obstacleAvoidance = false;  // 避障模式
Servo steeringServo;   // 舵机对象

// ====================== 网页界面HTML ======================
const char* MAIN_page = R"rawliteral(
<!DOCTYPE html>
<html lang="zh-CN">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>ESP32 智能小车控制</title>
    <style>
        * { margin: 0; padding: 0; box-sizing: border-box; }
        body { 
            font-family: Arial, sans-serif; 
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            min-height: 100vh;
            padding: 20px;
            color: white;
        }
        .container {
            max-width: 800px;
            margin: 0 auto;
            background: rgba(255, 255, 255, 0.1);
            backdrop-filter: blur(10px);
            border-radius: 20px;
            padding: 30px;
            box-shadow: 0 8px 32px rgba(0, 0, 0, 0.3);
        }
        header {
            text-align: center;
            margin-bottom: 30px;
        }
        h1 {
            font-size: 2.5em;
            margin-bottom: 10px;
            text-shadow: 2px 2px 4px rgba(0, 0, 0, 0.3);
        }
        .status {
            display: flex;
            justify-content: space-around;
            margin: 20px 0;
            flex-wrap: wrap;
        }
        .status-item {
            background: rgba(255, 255, 255, 0.2);
            padding: 15px;
            border-radius: 10px;
            text-align: center;
            min-width: 150px;
            margin: 5px;
        }
        .control-panel {
            display: grid;
            grid-template-columns: 1fr 1fr;
            gap: 20px;
            margin: 30px 0;
        }
        @media (max-width: 600px) {
            .control-panel {
                grid-template-columns: 1fr;
            }
        }
        .joystick-area {
            background: rgba(255, 255, 255, 0.15);
            padding: 20px;
            border-radius: 15px;
            text-align: center;
        }
        #joystick {
            width: 200px;
            height: 200px;
            background: rgba(255, 255, 255, 0.1);
            border-radius: 50%;
            margin: 20px auto;
            position: relative;
            touch-action: none;
        }
        .joystick-head {
            width: 60px;
            height: 60px;
            background: #4CAF50;
            border-radius: 50%;
            position: absolute;
            top: 50%;
            left: 50%;
            transform: translate(-50%, -50%);
            box-shadow: 0 4px 15px rgba(0, 0, 0, 0.2);
        }
        .controls {
            background: rgba(255, 255, 255, 0.15);
            padding: 20px;
            border-radius: 15px;
        }
        .control-group {
            margin: 20px 0;
        }
        label {
            display: block;
            margin-bottom: 8px;
            font-weight: bold;
        }
        input[type="range"] {
            width: 100%;
            height: 10px;
            -webkit-appearance: none;
            background: rgba(255, 255, 255, 0.2);
            border-radius: 5px;
            outline: none;
        }
        input[type="range"]::-webkit-slider-thumb {
            -webkit-appearance: none;
            width: 25px;
            height: 25px;
            background: #4CAF50;
            border-radius: 50%;
            cursor: pointer;
        }
        .buttons {
            display: grid;
            grid-template-columns: repeat(2, 1fr);
            gap: 10px;
            margin: 20px 0;
        }
        button {
            padding: 15px;
            border: none;
            border-radius: 10px;
            background: rgba(255, 255, 255, 0.2);
            color: white;
            font-size: 1.1em;
            cursor: pointer;
            transition: all 0.3s;
            backdrop-filter: blur(5px);
        }
        button:hover {
            background: rgba(255, 255, 255, 0.3);
            transform: translateY(-2px);
        }
        button:active {
            transform: translateY(0);
        }
        .action-btn {
            background: linear-gradient(45deg, #FF416C, #FF4B2B);
        }
        .toggle-btn {
            background: linear-gradient(45deg, #2196F3, #21CBF3);
        }
        .led-control {
            display: flex;
            justify-content: center;
            gap: 10px;
            margin: 20px 0;
        }
        .led-btn {
            width: 50px;
            height: 50px;
            border-radius: 50%;
            border: none;
            cursor: pointer;
        }
        .data-panel {
            background: rgba(255, 255, 255, 0.1);
            padding: 20px;
            border-radius: 15px;
            margin-top: 20px;
        }
        .data-item {
            display: flex;
            justify-content: space-between;
            margin: 10px 0;
            padding: 10px;
            background: rgba(255, 255, 255, 0.05);
            border-radius: 8px;
        }
    </style>
</head>
<body>
    <div class="container">
        <header>
            <h1>🚗 ESP32 智能小车控制</h1>
            <p>IP: 192.168.4.1 | 信号强度: <span id="rssi">--</span>dBm</p>
        </header>

        <div class="status">
            <div class="status-item">
                <h3>🔋 电量</h3>
                <p id="battery">--%</p>
            </div>
            <div class="status-item">
                <h3>📡 距离</h3>
                <p id="distance">-- cm</p>
            </div>
            <div class="status-item">
                <h3>🌡️ 温度</h3>
                <p id="temperature">--°C</p>
            </div>
            <div class="status-item">
                <h3>🚀 速度</h3>
                <p id="speed">--</p>
            </div>
        </div>

        <div class="control-panel">
            <div class="joystick-area">
                <h3>🎮 方向控制</h3>
                <div id="joystick">
                    <div class="joystick-head"></div>
                </div>
                <p id="joystick-status">X: 0, Y: 0</p>
            </div>

            <div class="controls">
                <div class="control-group">
                    <label for="speedControl">🚀 速度控制: <span id="speedValue">50%</span></label>
                    <input type="range" id="speedControl" min="0" max="100" value="50">
                </div>

                <div class="control-group">
                    <label for="servoControl">🎯 舵机角度: <span id="servoValue">90°</span></label>
                    <input type="range" id="servoControl" min="0" max="180" value="90">
                </div>

                <div class="buttons">
                    <button class="action-btn" onclick="controlCar('forward')">⬆️ 前进</button>
                    <button class="action-btn" onclick="controlCar('backward')">⬇️ 后退</button>
                    <button class="action-btn" onclick="controlCar('left')">⬅️ 左转</button>
                    <button class="action-btn" onclick="controlCar('right')">➡️ 右转</button>
                </div>

                <div class="buttons">
                    <button onclick="controlCar('stop')">🛑 停止</button>
                    <button class="toggle-btn" id="avoidanceBtn" onclick="toggleAvoidance()">
                        ⚠️ 避障模式: 关
                    </button>
                </div>

                <div class="led-control">
                    <button class="led-btn" style="background: #FF0000;" onclick="setLED('red')"></button>
                    <button class="led-btn" style="background: #00FF00;" onclick="setLED('green')"></button>
                    <button class="led-btn" style="background: #0000FF;" onclick="setLED('blue')"></button>
                    <button class="led-btn" style="background: #FFFFFF;" onclick="setLED('white')"></button>
                    <button class="led-btn" style="background: #FF9900;" onclick="setLED('rainbow')">🌈</button>
                    <button class="led-btn" style="background: #000000;" onclick="setLED('off')">关</button>
                </div>
            </div>
        </div>

        <div class="data-panel">
            <h3>📊 实时数据</h3>
            <div class="data-item">
                <span>WiFi连接:</span>
                <span id="wifiStatus">已连接</span>
            </div>
            <div class="data-item">
                <span>运行时间:</span>
                <span id="uptime">0s</span>
            </div>
            <div class="data-item">
                <span>内存使用:</span>
                <span id="memory">-- KB</span>
            </div>
        </div>
    </div>

    <script>
        let joystick = document.getElementById('joystick');
        let joystickHead = joystick.querySelector('.joystick-head');
        let isDragging = false;
        let lastX = 0, lastY = 0;

        // 摇杆控制
        joystick.addEventListener('mousedown', startDrag);
        joystick.addEventListener('touchstart', startDrag);
        document.addEventListener('mousemove', drag);
        document.addEventListener('touchmove', drag);
        document.addEventListener('mouseup', stopDrag);
        document.addEventListener('touchend', stopDrag);

        // 速度控制滑块
        let speedSlider = document.getElementById('speedControl');
        speedSlider.oninput = function() {
            document.getElementById('speedValue').textContent = this.value + '%';
            fetch('/speed?value=' + this.value);
        }

        // 舵机控制滑块
        let servoSlider = document.getElementById('servoControl');
        servoSlider.oninput = function() {
            document.getElementById('servoValue').textContent = this.value + '°';
            fetch('/servo?angle=' + this.value);
        }

        function startDrag(e) {
            isDragging = true;
            updateJoystick(e);
        }

        function drag(e) {
            if (!isDragging) return;
            e.preventDefault();
            updateJoystick(e);
        }

        function stopDrag() {
            if (!isDragging) return;
            isDragging = false;
            joystickHead.style.transform = 'translate(-50%, -50%)';
            document.getElementById('joystick-status').textContent = 'X: 0, Y: 0';
            fetch('/control?cmd=stop');
        }

        function updateJoystick(e) {
            let rect = joystick.getBoundingClientRect();
            let x, y;
            
            if (e.type.includes('touch')) {
                x = e.touches[0].clientX - rect.left;
                y = e.touches[0].clientY - rect.top;
            } else {
                x = e.clientX - rect.left;
                y = e.clientY - rect.top;
            }

            // 限制在圆形内
            let centerX = rect.width / 2;
            let centerY = rect.height / 2;
            let dx = x - centerX;
            let dy = y - centerY;
            let distance = Math.sqrt(dx * dx + dy * dy);
            let maxDistance = centerX;

            if (distance > maxDistance) {
                dx = (dx / distance) * maxDistance;
                dy = (dy / distance) * maxDistance;
                distance = maxDistance;
            }

            joystickHead.style.transform = `translate(${dx}px, ${dy}px)`;
            
            // 计算控制指令
            let normalizedX = Math.round((dx / maxDistance) * 100);
            let normalizedY = Math.round((dy / maxDistance) * 100);
            
            document.getElementById('joystick-status').textContent = 
                `X: ${normalizedX}, Y: ${normalizedY}`;

            // 发送控制命令
            sendJoystickCommand(normalizedX, normalizedY);
        }

        function sendJoystickCommand(x, y) {
            let cmd = 'stop';
            if (y > 30) cmd = 'forward';
            else if (y < -30) cmd = 'backward';
            else if (x > 30) cmd = 'right';
            else if (x < -30) cmd = 'left';
            
            fetch('/control?cmd=' + cmd);
        }

        function controlCar(command) {
            fetch('/control?cmd=' + command);
        }

        function toggleAvoidance() {
            let btn = document.getElementById('avoidanceBtn');
            let isOn = btn.textContent.includes('开');
            fetch('/avoidance?enable=' + (isOn ? 'false' : 'true'));
            btn.textContent = '⚠️ 避障模式: ' + (isOn ? '关' : '开');
        }

        function setLED(color) {
            fetch('/led?color=' + color);
        }

        // 更新传感器数据
        function updateSensorData() {
            fetch('/data')
                .then(response => response.json())
                .then(data => {
                    document.getElementById('distance').textContent = data.distance + ' cm';
                    document.getElementById('battery').textContent = data.battery + '%';
                    document.getElementById('temperature').textContent = data.temperature + '°C';
                    document.getElementById('rssi').textContent = data.rssi;
                    document.getElementById('memory').textContent = data.memory;
                    document.getElementById('uptime').textContent = data.uptime + 's';
                });
        }

        // 每2秒更新一次数据
        setInterval(updateSensorData, 2000);
        updateSensorData();
    </script>
</body>
</html>
)rawliteral";

// ====================== 函数实现 ======================

// 初始化 GPIO
void initGPIO() {
    Serial.begin(115200);
    
    // 初始化电机引脚
    pinMode(MOTOR_A1, OUTPUT);
    pinMode(MOTOR_A2, OUTPUT);
    pinMode(MOTOR_B1, OUTPUT);
    pinMode(MOTOR_B2, OUTPUT);
    pinMode(MOTOR_A_EN, OUTPUT);
    pinMode(MOTOR_B_EN, OUTPUT);
    
    // 初始化超声波引脚
    pinMode(TRIG_PIN, OUTPUT);
    pinMode(ECHO_PIN, INPUT);
    
    // 初始化 LED 灯带
    strip.begin();
    strip.show();
    strip.setBrightness(50);
    
    // 初始化舵机
    steeringServo.attach(SERVO_PIN);
    steeringServo.write(servoAngle);
    
    // 初始化按键
    pinMode(BUTTON_PIN, INPUT_PULLUP);
    
    Serial.println("GPIO 初始化完成");
}

// 设置电机速度
void setMotorSpeed(int leftSpeed, int rightSpeed, bool leftForward = true, bool rightForward = true) {
    // 限制速度范围
    leftSpeed = constrain(leftSpeed, 0, 255);
    rightSpeed = constrain(rightSpeed, 0, 255);
    
    // 左电机
    if (leftForward) {
        digitalWrite(MOTOR_B1, HIGH);
        digitalWrite(MOTOR_B2, LOW);
    } else {
        digitalWrite(MOTOR_B1, LOW);
        digitalWrite(MOTOR_B2, HIGH);
    }
    analogWrite(MOTOR_B_EN, leftSpeed);
    
    // 右电机
    if (rightForward) {
        digitalWrite(MOTOR_A1, HIGH);
        digitalWrite(MOTOR_A2, LOW);
    } else {
        digitalWrite(MOTOR_A1, LOW);
        digitalWrite(MOTOR_A2, HIGH);
    }
    analogWrite(MOTOR_A_EN, rightSpeed);
}

// 设置 LED 颜色
void setLEDColor(uint8_t r, uint8_t g, uint8_t b) {
    for (int i = 0; i < strip.numPixels(); i++) {
        strip.setPixelColor(i, strip.Color(r, g, b));
    }
    strip.show();
}

// 控制小车运动
void controlCar(String command) {
    Serial.println("控制命令: " + command);
    
    if (command == "forward") {
        setMotorSpeed(carSpeed, carSpeed, true, true);
        setLEDColor(0, 255, 0); // 绿色
    } else if (command == "backward") {
        setMotorSpeed(carSpeed, carSpeed, false, false);
        setLEDColor(255, 0, 0); // 红色
    } else if (command == "left") {
        setMotorSpeed(carSpeed/2, carSpeed, true, true);
        setLEDColor(255, 255, 0); // 黄色
    } else if (command == "right") {
        setMotorSpeed(carSpeed, carSpeed/2, true, true);
        setLEDColor(255, 255, 0); // 黄色
    } else if (command == "stop") {
        setMotorSpeed(0, 0);
        setLEDColor(0, 0, 255); // 蓝色
    }
}



// LED 彩虹效果
void rainbowLED() {
    static uint16_t hue = 0;
    for (int i = 0; i < strip.numPixels(); i++) {
        strip.setPixelColor(i, strip.ColorHSV((hue + i * 65536L / strip.numPixels()) & 65535));
    }
    strip.show();
    hue += 256;
}

// 读取超声波距离
float readDistance() {
    digitalWrite(TRIG_PIN, LOW);
    delayMicroseconds(2);
    digitalWrite(TRIG_PIN, HIGH);
    delayMicroseconds(10);
    digitalWrite(TRIG_PIN, LOW);
    
    long duration = pulseIn(ECHO_PIN, HIGH, 30000);
    if (duration == 0) return 999.0;
    
    float distance = duration * 0.034 / 2;
    return distance;
}

// 避障功能
void obstacleAvoidanceTask() {
    if (!obstacleAvoidance) return;
    
    float distance = readDistance();
    if (distance < 20.0) {
        // 前方有障碍物
        controlCar("stop");
        delay(200);
        controlCar("backward");
        delay(300);
        controlCar("left");
        delay(400);
        controlCar("forward");
    }
}

// 初始化 WiFi 热点
void initWiFiAP() {
    Serial.println("正在启动 WiFi 热点...");
    
    //定义一个新名字
    // String apSSID = "esp32-car" +String(random(0, 1000));

    WiFi.mode(WIFI_AP);
    WiFi.softAPConfig(localIP, gateway, subnet);
    WiFi.softAP(apSSID, apPassword);
    
    Serial.print("热点 SSID: ");
    Serial.println(apSSID);
    Serial.print("热点密码: ");
    Serial.println(apPassword);
    Serial.print("IP 地址: ");
    Serial.println(WiFi.softAPIP());
    
    // 启动 mDNS
    if (MDNS.begin("esp32-car")) {
        Serial.println("mDNS 启动成功");
        Serial.println("可通过 http://esp32-car.local 访问");
    }
    
    // 启动 DNS 服务器（用于强制跳转到配置页面）
    dnsServer.start(53, "*", localIP);
}

// Web 服务器路由处理
void handleRoot() {
    server.send(200, "text/html", MAIN_page);
}

void handleControl() {
    if (server.hasArg("cmd")) {
        String cmd = server.arg("cmd");
        controlCar(cmd);
        server.send(200, "text/plain", "OK: " + cmd);
    }
}

void handleSpeed() {
    if (server.hasArg("value")) {
        carSpeed = map(server.arg("value").toInt(), 0, 100, 0, 255);
        server.send(200, "text/plain", "Speed: " + String(carSpeed));
    }
}

void handleServo() {
    if (server.hasArg("angle")) {
        servoAngle = server.arg("angle").toInt();
        steeringServo.write(servoAngle);
        server.send(200, "text/plain", "Servo: " + String(servoAngle));
    }
}

void handleLED() {
    if (server.hasArg("color")) {
        String color = server.arg("color");
        if (color == "red") setLEDColor(255, 0, 0);
        else if (color == "green") setLEDColor(0, 255, 0);
        else if (color == "blue") setLEDColor(0, 0, 255);
        else if (color == "white") setLEDColor(255, 255, 255);
        else if (color == "rainbow") rainbowLED();
        else if (color == "off") setLEDColor(0, 0, 0);
        server.send(200, "text/plain", "LED: " + color);
    }
}

void handleAvoidance() {
    if (server.hasArg("enable")) {
        obstacleAvoidance = (server.arg("enable") == "true");
        server.send(200, "text/plain", obstacleAvoidance ? "避障开启" : "避障关闭");
    }
}

void handleData() {
    // 模拟传感器数据（实际项目需要连接真实传感器）
    String json = "{";
    json += "\"distance\":\"" + String(readDistance()) + "\",";
    json += "\"battery\":\"" + String(random(80, 100)) + "\",";
    json += "\"temperature\":\"" + String(random(20, 35)) + "\",";
    json += "\"rssi\":\"" + String(WiFi.RSSI()) + "\",";
    json += "\"memory\":\"" + String(ESP.getFreeHeap() / 1024) + "\",";
    json += "\"uptime\":\"" + String(millis() / 1000) + "\"";
    json += "}";
    
    server.send(200, "application/json", json);
}

// 初始化 Web 服务器
void initWebServer() {
    server.on("/", handleRoot);
    server.on("/control", handleControl);
    server.on("/speed", handleSpeed);
    server.on("/servo", handleServo);
    server.on("/led", handleLED);
    server.on("/avoidance", handleAvoidance);
    server.on("/data", handleData);
    
    // 处理未找到的页面
    server.onNotFound([]() {
        server.send(404, "text/plain", "404: 页面未找到");
    });
    
    server.begin();
    Serial.println("HTTP 服务器已启动");
}

// ====================== 主程序 ======================
void setup() {
    initGPIO();
    initWiFiAP();
    initWebServer();
    
    // 开机动画
    for (int i = 0; i < 3; i++) {
        setLEDColor(255, 0, 0);
        delay(200);
        setLEDColor(0, 255, 0);
        delay(200);
        setLEDColor(0, 0, 255);
        delay(200);
    }
    setLEDColor(0, 0, 0);
    
    Serial.println("系统初始化完成！");
    Serial.println("请连接 WiFi: " + String(apSSID));
    Serial.println("密码: " + String(apPassword));
    Serial.println("然后访问: http://192.168.4.1");
}

void loop() {
    server.handleClient();
    dnsServer.processNextRequest();
    
    // 避障模式检查
    obstacleAvoidanceTask();
    
    // 检查按钮
    if (digitalRead(BUTTON_PIN) == LOW) {
        delay(50); // 消抖
        if (digitalRead(BUTTON_PIN) == LOW) {
            Serial.println("按钮按下，停止小车");
            controlCar("stop");
            delay(1000);
        }
    }
    
    // 心跳指示灯
    static unsigned long lastBlink = 0;
    if (millis() - lastBlink > 1000) {
        lastBlink = millis();
        strip.setPixelColor(0, millis() % 2000 < 1000 ? 0x00FF00 : 0x000000);
        strip.show();
    }
}