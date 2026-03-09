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
            printf "Installing GTKmm 3.0 dependencies for Debian-based system ($ID)..."
            sudo apt-get update > /dev/null && sudo apt-get install -y binutils libgtkmm-3.0-dev > /dev/null 1&>2
            printf "Done.\n"
            ;;
        arch|manjaro|endeavouros)
            printf "Installing GTKmm 3.0 dependencies for Arch-based system ($ID)..."
            sudo pacman -S --noconfirm --needed base-devel binutils gtkmm3 > /dev/null 1&>2
            printf "Done.\n"
            ;;
        fedora)
            printf "Installing GTKmm 3.0 dependencies for Fedora..."
            sudo dnf install -y gcc-c++ binutils gtkmm30-devel > /dev/null 1&>2
            printf "Done.\n"
            ;;
        ## Added case for NixOS @Req:Woy.MissingPKG
        nixos)
            echo "Note:"
            echo "The Nix Support for dependencies related to the LDS Software Setup Wizard is still a work in progress, expect minor bugs or missing packages."
            echo "This method installs the packages in a imperative manner, this kind of usage is discouraged by the NixOS Community since it directly installs and enables these packages, but we simply want this to compile."
            read -p "Proceed With Dependencies Installation? (y/N)" yn
            case $yn in
                [Yy]*)
                    nix-env -iA nixos.gnumake nixos.pkg-config nixos.gtkmm3 nixos.atkmm nixos.glibmm nixos.libsigcxx nixos.binutils > /dev/null
                    nix-shell -p nixos.gcc > /dev/null 1&>2
                    printf "Installing GTKmm 3.0 dependencies for NixOS (Imperative)...\n"
                    printf "Done.\n"
                    ;;
                *)
                echo "Installation cancelled."
                break
                ;;
            esac
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