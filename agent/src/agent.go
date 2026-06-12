package main

import (
	"encoding/json"
	"fmt"
	"log"
	"os"
)

func main() {
	type Config struct {
		Uid int `json:"uid"`
	}

	fmt.Println("Reading File")
	data, err := os.ReadFile("agent_config_transfer.json")

	fmt.Println("File read")
	if err != nil {
		log.Fatal(err)
	}

	var config Config
	fmt.Println("Unmarshaling")
	json.Unmarshal(data, &config)
	fmt.Println("Unmarshalled")
	fmt.Println(config.Uid)
}
