# 🌐 qubip-mqtt-linux-client-openssl

This program tries to connect to an MQTT broker via TLS v1.3 encryption.

This example demonstrates how to establish a secure connection to an MQTT broker using TLS v1.3 encryption.

It subsequently establishes a Modbus TCP connection to a PLC (or any Modbus TCP master), reads a holding register, and transmits its value to the MQTT broker. The register is then incremented by one, updated on the PLC, and this cycle repeats continuously.

## 🛠️ To Compile:

### Clone the Repository 📁

```bash
git clone --recurse-submodules https://github.com/smartfactoryrepo/qubip-mqtt-linux-client-openssl.git
```

### Give Execute Permissions 🔑

```bash
cd openssl-3.2.2
chmod +x Configure
```

### Configure OpenSSL ⚙️

```bash
./Configure --prefix=/usr/local/ssl --openssldir=/usr/local/ssl '-Wl,-rpath,$(LIBRPATH)'
```

### Build OpenSSL 🔨

```bash
make -j8
```

### Return to Main Directory and Compile 📦

```bash
cd ..
make -j8
```

### You're all set! 🚀

# 🛠️ Command to Execute the Program

## 🚀 How to Run

```bash
./main --ca-cert cert/ca.crt --client-cert cert/client.crt --client-key cert/client.key --modbus-ip 192.168.168.134 --modbus-port 5002 --modbus-register 10 --mqtt-broker 192.168.101.63 --mqtt-port 1883 --mqtt-device-name LINUX_DEVICE --mqtt-topic 2023/test
```

## 📝 Parameter Description

- `--ca-cert cert/ca.crt`
  - **Certificate Authority (CA) Certificate 🛡️**: Specifies the path to the CA certificate used to verify the 🔒 TLS connection.

- `--client-cert cert/client.crt`
  - **Client Certificate 📜**: Specifies the path to the client certificate, used to authenticate the device 📱 to the server.

- `--client-key cert/client.key`
  - **Client Private Key 🔑**: Specifies the path to the private key associated with the client certificate, used to establish a secure 🔒 connection.

- `--modbus-ip 192.168.168.134`
  - **Modbus IP Address 🌐**: IP address of the Modbus server to connect to for reading 📖 or writing ✍️ registers.

- `--modbus-port 5002`
  - **Modbus Port 🔌**: Port number of the Modbus server to use for the connection.

- `--modbus-register 10`
  - **Modbus Register 📋**: Register number to read 📖 or write ✍️ during communication.

- `--mqtt-broker 192.168.101.63`
  - **MQTT Broker 📡**: IP address of the MQTT broker to which the device connects to publish 📤 or subscribe to messages.

- `--mqtt-port 1883`
  - **MQTT Port 🔌**: Port number used to connect to the MQTT broker.

- `--mqtt-device-name LINUX_DEVICE`
  - **MQTT Device Name 📛**: Identifier name of the device used for MQTT communications.

- `--mqtt-topic 2023/test`
  - **MQTT Topic 📝**: MQTT topic where the device publishes data 📊. It can also be used for subscriptions or notifications 🔔.
