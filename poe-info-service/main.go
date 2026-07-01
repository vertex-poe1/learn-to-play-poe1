package main

import (
	"flag"
	"fmt"
	"io"
	"log"
	"os"
	"path/filepath"
	"time"

	"github.com/MovingCairn/poe-info-service/config"
	"github.com/MovingCairn/poe-info-service/internal/proto"
	"github.com/MovingCairn/poe-info-service/internal/server"
)

func main() {
	exe, _ := os.Executable()
	fileCfg := config.Load(filepath.Dir(exe))

	var (
		logPath    = flag.String("log-path", "", "Path to Client.txt (e.g. C:\\Games\\PoE\\logs\\Client.txt)")
		dbPath     = flag.String("db-path", "", "Path to l2p SQLite database")
		port       = flag.Int("port", fileCfg.Port, "TCP port to listen on")
		bind       = flag.String("bind", fileCfg.Bind, "Bind address (default 127.0.0.1)")
		cacheDir   = flag.String("cache-dir", defaultCacheDir(), "Directory for SQLite DB and state files")
		serviceLog = flag.String("service-log", os.Getenv("L2P_SERVICE_LOG"), "Path to service debug log file")
		showVer    = flag.Bool("version", false, "Print version and exit")
	)
	flag.Parse()

	if *showVer {
		fmt.Println(proto.Version)
		return
	}

	if *serviceLog != "" {
		f, err := os.OpenFile(*serviceLog, os.O_APPEND|os.O_CREATE|os.O_WRONLY, 0644)
		if err == nil {
			log.SetOutput(io.MultiWriter(os.Stderr, f))
			defer f.Close()
		} else {
			log.Printf("warn: cannot open service log %q: %v", *serviceLog, err)
		}
	}

	cfg := server.Config{
		Version:   proto.Version,
		StartTime: time.Now().Unix(),
		Bind:      *bind,
		Port:      *port,
		CacheDir:  *cacheDir,
		LogPath:   *logPath,
		DbPath:    *dbPath,
	}

	log.Printf("starting v%s on %s:%d db=%q logPath=%q",
		cfg.Version, cfg.Bind, cfg.Port, cfg.DbPath, cfg.LogPath)

	if err := server.Run(cfg); err != nil {
		log.Fatalf("fatal: %v", err)
	}
}

func defaultCacheDir() string {
	if appData := os.Getenv("APPDATA"); appData != "" {
		return filepath.Join(appData, "poe-info-service")
	}
	home, _ := os.UserHomeDir()
	return filepath.Join(home, ".poe-info-service")
}
