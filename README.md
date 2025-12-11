<div align="center">

# 💬 ft_irc

### *Internet Relay Chat Server in C++98*

![42 School](https://img.shields.io/badge/42-Beirut-000000?style=for-the-badge&logo=42&logoColor=white)
![C++](https://img.shields.io/badge/C++-00599C?style=for-the-badge&logo=c%2B%2B&logoColor=white)
![IRC](https://img.shields.io/badge/IRC-Protocol-5865F2?style=for-the-badge&logo=discord&logoColor=white)
![HexChat](https://img.shields.io/badge/HexChat-Compatible-4A90E2?style=for-the-badge&logo=hexo&logoColor=white)

### ⚡ Features

🔐 **Password Auth** • 💬 **Multi-Channel** • 👥 **100+ Clients** • 🤖 **IRC Bot** • 📁 **File Transfer** • 🎯 **Operators** • ⚡ **Non-blocking I/O**

</div>

## 📋 Table of Contents

- [🎯 About](#-about)
- [✨ Features](#-features-1)
- [📦 Installation](#-installation)
- [🚀 Usage](#-usage)
- [🔌 Connecting with HexChat](#-connecting-with-hexchat)
- [💬 Commands](#-commands)
- [🔧 Troubleshooting](#-troubleshooting)

## 🎯 About

A fully-functional IRC server written in C++98 for 42 school. Implements the IRC protocol with support for multiple clients, channels, private messaging, operator privileges, plus bonus features like an interactive bot and file transfer system. Compatible with all standard IRC clients, especially **HexChat**.

## ✨ Features

- 🔐 Password authentication (PASS/NICK/USER)
- 💬 Multiple channels with JOIN/PART
- 👥 Private messaging (PRIVMSG)
- 👑 Channel operators with KICK/INVITE/TOPIC/MODE
- 📊 User info commands (WHO/WHOIS)
- 🤖 Interactive bot (`!bot help`, `!bot time`, `!bot users`, `!bot joke`)
- 📁 File transfer system (SENDFILE/GETFILE)
- ⚡ Non-blocking I/O with `poll()`
- 🔒 Channel modes: +i (invite), +t (topic), +k (password), +l (limit), +o (operator)

## 📦 Installation

1. **Clone the repository:**
   ```bash
   git clone https://github.com/yourusername/ft_irc.git
   cd ft_irc
   ```

2. **Compile the server:**
   ```bash
   make
   ```

   This will create the `ircserv` executable.

3. **Clean build files (optional):**
   ```bash
   make clean      # Remove object files
   make fclean     # Remove object files and executable
   make re         # Clean and rebuild
   ```

## 🚀 Usage

### Starting the Server

```bash
./ircserv <port> <password>
```

**Parameters:**
- `<port>`: Port number (1024-49151)
- `<password>`: Server password for authentication

**Example:**
```bash
./ircserv 6667 mySecretPass123
```

**Output:**
```
Server started on port 6667
Waiting for connections...
```

### Stopping the Server

Press `Ctrl+C` or send SIGINT to gracefully shutdown:
```bash
# In another terminal
pkill ircserv
```

## 📂 Project Structure

```
.
├── Makefile                          # Build automation
├── README.md                         # This file
├── server.hpp                        # Main header file
│
├── main.cpp                          # Entry point & argument validation
├── server_setup.cpp                  # Socket creation & setup
├── server_loop.cpp                   # Main event loop with poll()
├── client_handler.cpp                # Client connection handling
│
├── auth_commands.cpp                 # PASS, NICK, USER commands
├── cmd_join.cpp                      # JOIN & PART commands
├── cmd_privmsg.cpp                   # PRIVMSG command
├── cmd_operator.cpp                  # KICK, INVITE, TOPIC, MODE
├── cmd_who.cpp                       # WHO command
├── cmd_whois.cpp                     # WHOIS command
│
├── bot.cpp                           # Interactive bot feature
├── file_transfer.cpp                 # SENDFILE/GETFILE commands
└── utils.cpp                         # Helper functions
```

### Key Components

| File | Description |
|------|-------------|
| `server.hpp` | Classes: `Client`, `Channel`, `ServerEnv` + function declarations |
| `server_setup.cpp` | Socket creation, binding, listening, non-blocking setup |
| `server_loop.cpp` | Main `poll()` loop, new connections, data handling |
| `client_handler.cpp` | Command parsing, client disconnection |
| `auth_commands.cpp` | Authentication flow implementation |
| `cmd_*.cpp` | Individual IRC command handlers |
| `bot.cpp` | Bot command parsing and responses |
| `file_transfer.cpp` | Base64 encoding and file transfer logic |
| `utils.cpp` | String manipulation, message sending, validation |

## 🔌 Connecting with HexChat

### Install HexChat
```bash
# Ubuntu/Debian
sudo apt-get install hexchat

# macOS
brew install --cask hexchat

# Arch
sudo pacman -S hexchat
```

### Quick Setup

1. Open HexChat → Network List → Add
2. Name: `Local IRC Server`
3. Click Edit → Add server: `localhost/6667`
4. Set your nickname and username
5. Server password: (your server password)
6. Click Connect
7. Join a channel: `/join #general`

### Essential Commands
```irc
/server localhost 6667 password      # Connect
/nick YourNick                       # Set nickname
/join #channel                       # Join channel
/msg user hello                      # Private message
/part #channel                       # Leave channel
```

## 💬 Commands

### Authentication (Required)
```irc
PASS mySecretPass123
NICK Alice
USER alice 0 * :Alice Wonderland
```

### Basic Commands
```irc
JOIN #channel [password]             # Join/create channel
PART #channel [message]              # Leave channel
PRIVMSG #channel :message            # Send message
PRIVMSG user :message                # Private message
WHO #channel                         # List users
WHOIS nickname                       # User info
TOPIC #channel [:new topic]          # View/set topic
```

### Operator Commands (requires Operator)
```irc
MODE #channel +i                     # Invite-only
MODE #channel +t                     # Topic lock
MODE #channel +k password            # Set password
MODE #channel +l 50                  # User limit
MODE #channel +o nickname            # Give operator
KICK #channel user [reason]          # Remove user
INVITE user #channel                 # Invite user
```

### Bot Commands
```irc
!bot help                            # Bot commands
!bot time                            # Server time
!bot users                           # Channel user count
!bot joke                            # Programming joke
```

### File Transfer
```irc
SENDFILE user filename :data         # Send file (Base64 encoded)
```
## 🔧 Troubleshooting

### Port Already in Use
```bash
lsof -i :6667              # Check what's using the port
kill -9 <PID>              # Kill the process
./ircserv 6668 pass        # Use different port
```

### Can't Connect
```bash
ps aux | grep ircserv      # Verify server is running
netstat -tuln | grep 6667  # Check if listening
telnet localhost 6667      # Test connection
```

### Authentication Fails
- Ensure PASS matches server password
- Commands must be: PASS → NICK → USER (in order)
- No spaces in password

### Can't Join Channel
- Invite-only (`+i`)? Ask operator for `/INVITE`
- Password protected (`+k`)? Use `/JOIN #channel password`
- Full (`+l`)? Channel has reached user limit

### Compilation Issues
```bash
make fclean && make        # Clean rebuild
g++ --version              # Check compiler (need ≥4.8)
```

---

<div align="center">

*RFC 1459/2812 Compliant • C++98 Standard • Non-blocking I/O with poll()*

![Status](https://img.shields.io/badge/Status-Completed-success?style=flat-square)

</div>
