# RSA Encrypted Mailbox System

### Cài đặt dependencies (Ubuntu/Debian):
```bash
sudo apt-get update
sudo apt-get install build-essential linux-headers-$(uname -r) libssl-dev
```

### Cài đặt dependencies (CentOS/RHEL):
```bash
sudo yum groupinstall "Development Tools"
sudo yum install kernel-devel openssl-devel
```

## 🛠️ Biên dịch và cài đặt

### 1. Biên dịch tất cả:
```bash
make all
```

### 2. Cài đặt kernel module:
```bash
sudo make install
```

### 3. Tạo RSA key pair:
```bash
./rsa_mailbox /dev/mailbox0 0 public_key.pem generate_keys
```

## 🚀 Sử dụng

### Phương án 1: RSA Encrypted Communication

**Terminal 1 - Writer (mã hóa):**
```bash
./rsa_mailbox /dev/mailbox0 0 public_key.pem
```

**Terminal 2 - Reader (giải mã):**
```bash
./rsa_mailbox /dev/mailbox1 1 private_key.pem
```

### Phương án 2: Plain Text Communication

**Terminal 1 - Writer:**
```bash
./test_interactive /dev/mailbox0 0
```

**Terminal 2 - Reader:**
```bash
./test_interactive /dev/mailbox1 1
```

### Demo tự động:
```bash
chmod +x demo.sh
./demo.sh
```

## 📁 Cấu trúc files

```
.
├── mailbox.c           # Kernel driver
├── test_interactive.c  # Plain text user program
├── rsa_mailbox.c      # RSA encrypted user program
├── Makefile           # Build system
├── demo.sh           # Demo script
└── README.md         # Documentation
```

## 🔐 RSA Encryption Details

- **Key size**: 2048-bit
- **Padding**: PKCS1 padding
- **Max message size**: 245 bytes (due to RSA padding)
- **Encrypted output**: 256 bytes per message
- **Key format**: PEM format

## 🧪 Testing

### Kiểm tra module status:
```bash
make status
```

### Xem kernel logs:
```bash
make logs
# hoặc
dmesg | tail -20
```

### Test scenario mẫu:

1. **Process A**: Ghi "Hello"
2. **Process B**: Ghi "World"
3. **Process C**: Đọc 3 bytes → "Hel"
4. **Process C**: Đọc 7 bytes → "loWorld"

## 🔧 Troubleshooting

### Lỗi module không load được:
```bash
# Kiểm tra kernel logs
dmesg | grep mailbox

# Đảm bảo có quyền
sudo chmod 666 /dev/mailbox*
```

### Lỗi OpenSSL:
```bash
# Kiểm tra thư viện
ldd rsa_mailbox

# Reinstall OpenSSL development
sudo apt-get install --reinstall libssl-dev
```

### Permission denied:
```bash
# Thêm user vào group
sudo usermod -a -G dialout $USER
# hoặc chạy với sudo
sudo ./rsa_mailbox /dev/mailbox0 0 public_key.pem
```

## 🧹 Dọn dẹp

### Gỡ bỏ module:
```bash
make uninstall
```

### Xóa build files:
```bash
make clean
```

## ⚠️ Lưu ý bảo mật

1. **Private key security**: Bảo vệ private key cẩn thận
2. **Key file permissions**:
   ```bash
   chmod 600 private_key.pem
   chmod 644 public_key.pem
   ```
3. **Message size limit**: RSA chỉ mã hóa được tối đa 245 bytes
4. **Production use**: Cân nhắc sử dụng hybrid encryption (RSA + AES) cho tin nhắn dài

## 🔍 Architecture

```
┌─────────────────┐    ┌─────────────────┐
│   Process A     │    │   Process B     │
│   (Writer)      │    │   (Reader)      │
│                 │    │                 │
│ ┌─────────────┐ │    │ ┌─────────────┐ │
│ │ RSA Encrypt │ │    │ │ RSA Decrypt │ │
│ └─────────────┘ │    │ └─────────────┘ │
└─────────┬───────┘    └─────────┬───────┘
          │                      │
          ▼                      ▼
    /dev/mailbox0          /dev/mailbox1
          │                      │
          └──────────┬───────────┘
                     ▼
            ┌─────────────────┐
            │ Kernel Driver   │
            │                 │
            │ ┌─────────────┐ │
            │ │ Shared FIFO │ │
            │ │    Buffer   │ │
            │ └─────────────┘ │
            └─────────────────┘
```
```shell
sudo fuser /dev/mailbox0
```