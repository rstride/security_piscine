# <p align="center">ft_onion</p>

> *Project to create a web page accessible from the Tor network by setting up a hidden service.*

## Introduction

This project involves creating a web page and making it accessible via the Tor network by setting up a hidden service. The Tor network is a system that enables anonymous communication by directing internet traffic through a free, worldwide, volunteer overlay network. The goal is to run a web server that shows a static webpage accessible through a .onion address and configure it using Nginx. Additionally, SSH access to the server should be enabled for management purposes.

## Install

### Requirements

- Homebrew (for package management on macOS)
- Tor
- Nginx
- SSH

### Steps

1. **Install Nginx and Tor using Homebrew:**
   ```bash
   brew install nginx tor
    ```
2. **Configure Nginx:**
    - Edit the Nginx configuration file:
      ```bash
      sudo nano /usr/local/etc/nginx/nginx.conf
      ```
    - Add the following lines to the `http` block:
      ```nginx
      server {
            listen
            server_name localhost;
            location / {
                root /usr/local/var/www;
                index index.html;
            }
        }
        ```
    - Save and exit the file.
3. **Create the web page:**
    - Create the directory for the web page:
      ```bash
      sudo mkdir /usr/local/var/www
      ```
    - Create the index.html file:
      ```bash
      sudo nano /usr/local/var/www/index.html
      ```
    - Add the following content to the file:
      ```html
      <html>
        <head>
          <title>ft_onion</title>
        </head>
        <body>
          <h1>Welcome to ft_onion!</h1>
        </body>
      </html>
      ```
    - Save and exit the file.
4. **Configure Tor:**
    - Edit the Tor configuration file:
      ```bash
      sudo nano /usr/local/etc/tor/torrc
      ```
    - Add the following lines to the file:
      ```bash
      HiddenServiceDir /usr/local/etc/tor/hidden_service/
      HiddenServicePort 80
        ```
    - Save and exit the file.
5. **Start the Tor service:**
    ```bash
    brew services start tor
    ```
6. **Get the .onion address:**
    ```bash
    cat /usr/local/etc/tor/hidden_service/hostname
    ```
7. **Access the web page:**
    - Open a web browser and enter the .onion address obtained in the previous step.
8. **Enable SSH access:**
    - Edit the SSH configuration file:
      ```bash
      sudo nano /etc/ssh/sshd_config
      ```
    - Change the following line:
      ```bash
      #PasswordAuthentication yes
      ```
      to:
      ```bash
      PasswordAuthentication yes
      ```
    - Save and exit the file.
    - Restart the SSH service:
      ```bash
      sudo service ssh restart
      ```
    - Access the server via SSH:
      ```bash
        ssh username@hostname
        ```
    - Enter the password when prompted.

## Usage
    
    ```bash
    brew services start nginx
    brew services start tor
    ```

    - Open a web browser and enter the .onion address obtained in step 6.
