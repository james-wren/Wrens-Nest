package main

import (
	"fmt"
	"net/http"
)

func main() {
	http.HandleFunc("/", handleRequest)

	if err := http.ListenAndServe(":1690", nil); err != nil {
		fmt.Printf("Server error %v\n", err)
	}
}

func handleRequest(w http.ResponseWriter, r *http.Request) {
	if r.Method != http.MethodPost {
		http.Error(w, "Method Not Allowed", http.StatusMethodNotAllowed)

		return
	}

	w.Header().Set("Content-Type", "text/plain")

	w.WriteHeader(http.StatusOK)

	w.Write([]byte("Server up and running"))
}
