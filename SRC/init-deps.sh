#!/usr/bin/env bash
## LDS Software Setup Wizard - Dependency Manager

check_distro_and_install_deps(){
    # Source the file to get variables like $ID directly
    if [ -f /etc/os-release ]; then
        . /etc/os-release
    else
        printf "Error: Cannot detect distribution. /etc/os-release missing.\n"
        exit 1
    fi

    case $ID in
        ubuntu|debian|pop|mint)
            printf "Installing GTKmm 3.0 dependencies for Debian-based system ($ID)...\n"
            sudo apt-get update > /dev/null && sudo apt-get install -y binutils libgtkmm-3.0-dev > /dev/null
            printf "Done.\n"
            ;;
        arch|manjaro|endeavouros)
            printf "Installing GTKmm 3.0 dependencies for Arch-based system ($ID)...\n"
            sudo pacman -S --noconfirm --needed base-devel binutils gtkmm3 > /dev/null
            printf "Done.\n"
            ;;
        fedora)
            printf "Installing GTKmm 3.0 dependencies for Fedora...\n"
            sudo dnf install -y gcc-c++ binutils gtkmm30-devel > /dev/null
            printf "Done.\n"
            ;;
        *)
            printf "Unsupported distribution: $ID. Please install gtkmm-3.0 and binutils manually using your system's package manager.\n"
            exit 1
            ;;
    esac
}

read -p "Install GTKmm build dependencies for the LDS Software Setup Wizard? (y/N): " yn
case $yn in
    [Yy]*)
        check_distro_and_install_deps
        ;;
    *)
        printf "Operation cancelled. No dependencies were installed.\n"
        exit 0
        ;;
esac