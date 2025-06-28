#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <stdint.h>

// OpenSSL includes
#include <openssl/opensslv.h>
#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/rand.h>
#include <openssl/err.h>
#include <openssl/bn.h>

#define MAX_MESSAGE_SIZE 245  // RSA 2048-bit can encrypt max 245 bytes
#define RSA_KEYSIZE 2048
#define ENCRYPTED_SIZE 256    // RSA 2048-bit encrypted output is always 256 bytes

// Function prototypes
EVP_PKEY* load_public_key(const char* filename);
EVP_PKEY* load_private_key(const char* filename);
int generate_key_pair(const char* public_file, const char* private_file);
int encrypt_message(EVP_PKEY* pkey, const char* message, unsigned char* encrypted_data);
int decrypt_message(EVP_PKEY* pkey, const unsigned char* encrypted_data, char* output, int max_len);

// Generate RSA key pair using EVP (OpenSSL 3.0+ compatible)
int generate_key_pair(const char* public_file, const char* private_file) {
    EVP_PKEY_CTX* ctx = NULL;
    EVP_PKEY* pkey = NULL;
    FILE* fp = NULL;
    int ret = 0;

    // Create key generation context
    ctx = EVP_PKEY_CTX_new_id(EVP_PKEY_RSA, NULL);
    if (!ctx) {
        printf("Failed to create EVP_PKEY_CTX\n");
        goto cleanup;
    }

    // Initialize key generation
    if (EVP_PKEY_keygen_init(ctx) <= 0) {
        printf("Failed to initialize key generation\n");
        goto cleanup;
    }

    // Set key size to 2048 bits
    if (EVP_PKEY_CTX_set_rsa_keygen_bits(ctx, RSA_KEYSIZE) <= 0) {
        printf("Failed to set key size\n");
        goto cleanup;
    }

    // Generate key pair
    if (EVP_PKEY_keygen(ctx, &pkey) <= 0) {
        printf("Failed to generate key pair\n");
        goto cleanup;
    }

    // Save public key
    fp = fopen(public_file, "wb");
    if (!fp) {
        printf("Failed to open public key file\n");
        goto cleanup;
    }

    if (PEM_write_PUBKEY(fp, pkey) != 1) {
        printf("Failed to write public key\n");
        goto cleanup;
    }
    fclose(fp);
    fp = NULL;

    // Save private key
    fp = fopen(private_file, "wb");
    if (!fp) {
        printf("Failed to open private key file\n");
        goto cleanup;
    }

    if (PEM_write_PrivateKey(fp, pkey, NULL, NULL, 0, NULL, NULL) != 1) {
        printf("Failed to write private key\n");
        goto cleanup;
    }

    ret = 1;
    printf("RSA key pair generated successfully\n");
    printf("Public key: %s\n", public_file);
    printf("Private key: %s\n", private_file);

cleanup:
    if (fp) fclose(fp);
    EVP_PKEY_free(pkey);
    EVP_PKEY_CTX_free(ctx);
    return ret;
}

// Load public key from file
EVP_PKEY* load_public_key(const char* filename) {
    FILE* fp = fopen(filename, "rb");
    if (!fp) {
        printf("Unable to open public key file %s\n", filename);
        return NULL;
    }

    EVP_PKEY* pkey = PEM_read_PUBKEY(fp, NULL, NULL, NULL);
    fclose(fp);

    if (!pkey) {
        printf("Unable to load public key\n");
        ERR_print_errors_fp(stderr);
    }

    return pkey;
}

// Load private key from file
EVP_PKEY* load_private_key(const char* filename) {
    FILE* fp = fopen(filename, "rb");
    if (!fp) {
        printf("Unable to open private key file %s\n", filename);
        return NULL;
    }

    EVP_PKEY* pkey = PEM_read_PrivateKey(fp, NULL, NULL, NULL);
    fclose(fp);

    if (!pkey) {
        printf("Unable to load private key\n");
        ERR_print_errors_fp(stderr);
    }

    return pkey;
}

// Encrypt message - returns the size of encrypted data, or 0 on failure
int encrypt_message(EVP_PKEY* pkey, const char* message, unsigned char* encrypted_data) {
    EVP_PKEY_CTX* ctx = NULL;
    size_t outlen = ENCRYPTED_SIZE;
    int msg_len = strlen(message);
    int ret = 0;

    if (msg_len > MAX_MESSAGE_SIZE) {
        printf("Message too long for RSA encryption (max %d bytes)\n", MAX_MESSAGE_SIZE);
        return 0;
    }

    // Create encryption context
    ctx = EVP_PKEY_CTX_new(pkey, NULL);
    if (!ctx) {
        printf("Failed to create encryption context\n");
        goto cleanup;
    }

    // Initialize encryption
    if (EVP_PKEY_encrypt_init(ctx) <= 0) {
        printf("Failed to initialize encryption\n");
        goto cleanup;
    }

    // Set padding
    if (EVP_PKEY_CTX_set_rsa_padding(ctx, RSA_PKCS1_PADDING) <= 0) {
        printf("Failed to set padding\n");
        goto cleanup;
    }

    // Encrypt the message
    if (EVP_PKEY_encrypt(ctx, encrypted_data, &outlen,
                        (unsigned char*)message, msg_len) <= 0) {
        printf("RSA encryption failed\n");
        ERR_print_errors_fp(stderr);
        goto cleanup;
    }

    ret = (int)outlen;  // Return the actual encrypted size

cleanup:
    EVP_PKEY_CTX_free(ctx);
    return ret;
}

// Decrypt message - returns the length of decrypted message, or 0 on failure
int decrypt_message(EVP_PKEY* pkey, const unsigned char* encrypted_data, char* output, int max_len) {
    EVP_PKEY_CTX* ctx = NULL;
    size_t outlen = 512; // Đặt buffer đủ lớn cho RSA 2048-bit
    size_t encrypted_len = ENCRYPTED_SIZE;
    int ret = 0;
    unsigned char temp_buffer[512]; // Buffer tạm đủ lớn

    // Create decryption context
    ctx = EVP_PKEY_CTX_new(pkey, NULL);
    if (!ctx) {
        printf("Failed to create decryption context\n");
        goto cleanup;
    }

    // Initialize decryption
    if (EVP_PKEY_decrypt_init(ctx) <= 0) {
        printf("Failed to initialize decryption\n");
        goto cleanup;
    }

    // Set padding
    if (EVP_PKEY_CTX_set_rsa_padding(ctx, RSA_PKCS1_PADDING) <= 0) {
        printf("Failed to set padding\n");
        goto cleanup;
    }

    // Decrypt trực tiếp vào buffer tạm
    if (EVP_PKEY_decrypt(ctx, temp_buffer, &outlen,
                        encrypted_data, encrypted_len) <= 0) {
        printf("RSA decryption failed\n");
        ERR_print_errors_fp(stderr);
        goto cleanup;
    }
    // Kiểm tra xem có đủ chỗ trong output buffer không
    if (outlen >= (size_t)max_len) {
        printf("Decrypted message too long for output buffer\n");
        goto cleanup;
    }

    // Copy kết quả vào output buffer
    memcpy(output, temp_buffer, outlen);
    output[outlen] = '\0';
    ret = (int)outlen;

    cleanup:
        EVP_PKEY_CTX_free(ctx);
    return ret;
}

// Main interactive program
int main(int argc, char *argv[]) {
    int fd;
    char *device = "/dev/mailbox0";
    char *key_file = NULL;
    int mode = 0; // 0 = write, 1 = read
    EVP_PKEY* pkey = NULL;

    if (argc < 3) {
        printf("Usage: %s <device> <mode> <key_file> [generate_keys|test]\n", argv[0]);
        printf("  device: /dev/mailbox0 or /dev/mailbox1\n");
        printf("  mode: 0 = write (needs public key), 1 = read (needs private key)\n");
        printf("  key_file: public_key.pem or private_key.pem\n");
        printf("  generate_keys: optional - generates new key pair\n");
        printf("  test: optional - run encryption/decryption test\n");
        return 1;
    }

    device = argv[1];
    mode = atoi(argv[2]);

    if (argc >= 4) {
        key_file = argv[3];
    }

    // Generate keys if requested
    if (argc >= 5 && strcmp(argv[4], "generate_keys") == 0) {
        if (!generate_key_pair("public_key.pem", "private_key.pem")) {
            printf("Failed to generate key pair\n");
            return 1;
        }
        return 0;
    }

    if (!key_file) {
        printf("Key file is required\n");
        return 1;
    }

    // Initialize OpenSSL
    ERR_load_crypto_strings();
    OpenSSL_add_all_algorithms();
    OPENSSL_init_crypto(OPENSSL_INIT_LOAD_CRYPTO_STRINGS | OPENSSL_INIT_ADD_ALL_CIPHERS | OPENSSL_INIT_ADD_ALL_DIGESTS, NULL);

    // Load appropriate key
    if (mode == 0) {  // Write mode - need public key
        pkey = load_public_key(key_file);
        if (!pkey) {
            printf("Failed to load public key\n");
            return 1;
        }
    } else {  // Read mode - need private key
        pkey = load_private_key(key_file);
        if (!pkey) {
            printf("Failed to load private key\n");
            return 1;
        }
    }

    printf("Opening device: %s in %s mode\n", device, mode ? "READ" : "WRITE");
    fd = open(device, mode ? O_RDONLY : O_WRONLY);
    if (fd < 0) {
        perror("Failed to open device");
        EVP_PKEY_free(pkey);
        return 1;
    }

    if (mode == 0) {  // Write mode
        char input[MAX_MESSAGE_SIZE + 1];
        unsigned char encrypted_data[ENCRYPTED_SIZE];

        while (1) {
            printf("Enter message (max %d chars, or 'quit' to exit): ", MAX_MESSAGE_SIZE);
            if (!fgets(input, sizeof(input), stdin)) {
                break;
            }

            // Remove newline
            input[strcspn(input, "\n")] = 0;

            if (strcmp(input, "quit") == 0) {
                break;
            }

            // Encrypt message
            int encrypted_len = encrypt_message(pkey, input, encrypted_data);
            if (encrypted_len == 0) {
                printf("Encryption failed\n");
                continue;
            }

            // Write encrypted message to mailbox
            ssize_t bytes_written = write(fd, encrypted_data, encrypted_len);
            if (bytes_written < 0) {
                perror("Write failed");
            } else {
                printf("Encrypted and wrote %zd bytes (original: %zu bytes)\n",
                       bytes_written, strlen(input));
            }
        }
    } else {  // Read mode
        unsigned char encrypted_data[ENCRYPTED_SIZE];
        char decrypted[MAX_MESSAGE_SIZE + 1];

        while (1) {
            printf("Press Enter to read next encrypted message (or Ctrl+C to exit): ");
            getchar();

            printf("Reading encrypted message (may block)...\n");
            ssize_t bytes_read = read(fd, encrypted_data, ENCRYPTED_SIZE);
            if (bytes_read < 0) {
                perror("Read failed");
                continue;
            }

            if (bytes_read != ENCRYPTED_SIZE) {
                printf("Incomplete encrypted message received (%zd bytes, expected %d)\n",
                       bytes_read, ENCRYPTED_SIZE);
                continue;
            }

            // Decrypt message
            int decrypted_len = decrypt_message(pkey, encrypted_data, decrypted, sizeof(decrypted));
            if (decrypted_len > 0) {
                printf("Decrypted message (%d bytes): '%s'\n", decrypted_len, decrypted);
            } else {
                printf("Decryption failed\n");
            }
        }
    }

    close(fd);
    EVP_PKEY_free(pkey);
    EVP_cleanup();
    ERR_free_strings();
    return 0;
}