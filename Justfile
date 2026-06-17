# Build for Windows natively
build-win:
    cargo build --release

# Build for Linux via WSL
build-linux:
    wsl cargo build --release

# Run the project locally in dev mode
run:
    cargo run
