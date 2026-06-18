set windows-shell := ["powershell.exe", "-NoLogo", "-Command"]

# Build for Windows natively
build-win:
    cargo build --release

# Build for Linux via WSL
build-linux:
    wsl cargo build --release

# Run the project locally in dev mode
run:
    cargo build --bins
    cargo run

# Safely update dependencies ensuring they are at least 7 days old
update:
    cargo cooldown update
