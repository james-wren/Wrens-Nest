package agenttests

import (
	"os"
	"os/exec"
	"path/filepath"
	"strings"
	"testing"
)

func TestAgentDecryptAES(t *testing.T) {
	repoRoot := filepath.Clean(filepath.Join("..", ".."))
	agentSource, err := os.ReadFile(filepath.Join(repoRoot, "agent", "src", "agent.go"))
	if err != nil {
		t.Fatalf("read agent source: %v", err)
	}

	tmpDir := t.TempDir()
	if err := os.WriteFile(filepath.Join(tmpDir, "agent.go"), agentSource, 0o600); err != nil {
		t.Fatalf("write temporary agent source: %v", err)
	}

	testSource := []byte(`package main

import (
	"crypto/aes"
	"crypto/cipher"
	"encoding/base64"
	"fmt"
	"testing"
)

func pad(data []byte, blockSize int) []byte {
	padLen := blockSize - len(data)%blockSize
	out := make([]byte, len(data)+padLen)
	copy(out, data)
	for i := len(data); i < len(out); i++ {
		out[i] = byte(padLen)
	}
	return out
}

func encryptForTest(t *testing.T, plaintext, key, iv []byte) []byte {
	t.Helper()
	block, err := aes.NewCipher(key)
	if err != nil {
		t.Fatalf("aes.NewCipher: %v", err)
	}
	padded := pad(plaintext, block.BlockSize())
	ciphertext := make([]byte, len(padded))
	cipher.NewCBCEncrypter(block, iv).CryptBlocks(ciphertext, padded)
	return ciphertext
}

func TestDecryptAESRoundTrip(t *testing.T) {
	key := []byte("0123456789abcdef0123456789abcdef")
	iv := []byte("0123456789abcdef")
	plaintext := []byte("status: online")
	ciphertext := encryptForTest(t, plaintext, key, iv)
	encodedIV := base64.StdEncoding.EncodeToString(iv)
	encodedCiphertext := base64.StdEncoding.EncodeToString(ciphertext)
	encodedKey := base64.StdEncoding.EncodeToString(key)

	got, err := decryptAES(
		encodedIV,
		encodedCiphertext,
		encodedKey,
	)
	fmt.Printf(
		"decryptAES response: iv_b64=%s ciphertext_b64=%s key_bytes=%d plaintext=%q decrypted=%q error=%v\n",
		encodedIV,
		encodedCiphertext,
		len(key),
		plaintext,
		got,
		err,
	)
	if err != nil {
		t.Fatalf("decryptAES returned error: %v", err)
	}
	if string(got) != string(plaintext) {
		t.Fatalf("decryptAES = %q, want %q", got, plaintext)
	}
}

func TestDecryptAESRejectsInvalidBase64(t *testing.T) {
	got, err := decryptAES("not-base64", "also-not-base64", "still-not-base64")
	fmt.Printf("decryptAES invalid base64 response: plaintext=%q error=%v\n", got, err)
	if err == nil {
		t.Fatal("decryptAES should fail for invalid base64 input")
	}
}
	`)
	if err := os.WriteFile(filepath.Join(tmpDir, "agent_test.go"), testSource, 0o600); err != nil {
		t.Fatalf("write temporary agent test: %v", err)
	}

	cmd := exec.Command("go", "test", "-v")
	cmd.Dir = tmpDir
	cmd.Env = append(os.Environ(), "GOCACHE="+filepath.Join(tmpDir, "gocache"), "GO111MODULE=off")

	output, err := cmd.CombinedOutput()
	if len(output) > 0 {
		t.Logf("agent go test output:\n%s", strings.TrimSpace(string(output)))
	}
	if err != nil {
		t.Fatalf("go test failed:\n%s", output)
	}
}
