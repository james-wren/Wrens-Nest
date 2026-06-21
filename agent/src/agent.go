package main

import (
	"bufio"
	"crypto/aes"
	"crypto/cipher"
	"encoding/base64"
	"encoding/json"
	"fmt"
	"log"
	"net/http"
	"os"
	"strconv"
	"time"
)

type Config struct {
	Uid int    `json:"uid"`
	Key string `json:"key"`
}

func decryptAES(b64IV string, b64Text string, b64Key string) ([]byte, error) {
	key, err := base64.StdEncoding.DecodeString(b64Key)
	if err != nil {
		return nil, err
	}

	iv, err := base64.StdEncoding.DecodeString(b64IV)
	if err != nil {
		return nil, err
	}

	ct, err := base64.StdEncoding.DecodeString(b64Text)
	if err != nil {
		return nil, err
	}

	block, err := aes.NewCipher(key)
	if err != nil {
		return nil, err
	}

	mode := cipher.NewCBCDecrypter(block, iv)
	mode.CryptBlocks(ct, ct)

	padLen := int(ct[len(ct)-1])
	return ct[:len(ct)-padLen], nil
}

func main() {
	data, err := os.ReadFile("agent_config_transfer.json")

	if err != nil {
		log.Fatal(err)
	}

	var config Config
	json.Unmarshal(data, &config)
	var ip string

	{
		resp, err := http.Get("http://127.0.0.1:1690/wait/" + strconv.Itoa(config.Uid))
		if err != nil {
			log.Println("Failes to reach proxy server, trying agian")
		}

		scanner := bufio.NewScanner(resp.Body)
		for scanner.Scan() {
			line := scanner.Text()
			if line != "" {
				fmt.Println("Got Client IP:", line)
				ip = line
				break
			}
		}
		if err := scanner.Err(); err != nil {
			log.Println("Scanner error:", err)
		}

		log.Println("Connecting to client at " + ip)
		resp.Body.Close()
	}

	time.Sleep(time.Second * 5)

	resp, err := http.Get("http://" + ip + "/server")

	var msg struct {
		IV   string `json:"iv"`
		Text string `json:"text"`
	}

	scanner := bufio.NewScanner(resp.Body)
	for scanner.Scan() {
		line := scanner.Text()
		json.Unmarshal([]byte(line), &msg)
		plaintext, err := decryptAES(msg.IV, msg.Text, config.Key)

		if err != nil {
			log.Println("Failed to decrypt line")
		}

		fmt.Println(plaintext)
	}
	if err := scanner.Err(); err != nil {
		log.Println("Scanner error:", err)
	}
}
