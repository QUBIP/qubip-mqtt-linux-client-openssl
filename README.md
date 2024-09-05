
# 🌐 qubip-mqtt-linux-client-openssl

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
