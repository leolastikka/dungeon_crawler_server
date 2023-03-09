# Dungeon crawler server

## Description and info
TCP server for a dungeon crawler game. Written in C language.

Build and run:
```
sudo apt install build-essentials
git clone https://github.com/leolastikka/dungeon_crawler_server.git
cd dungeon_crawler_server
make
build/server
```
Server is currently compiled with debugging information (-g flag in makefile) to be used with gdb.

## Configuration
Server default configuration in main.c:
```
const int DEFAULT_PORT = 8080;
const int GAME_FPS = 10;
const int MAX_CLIENTS = 4;
const int MAX_WAITING_CONNECTIONS = 4;
const char *PORT_ENV_NAME = "PORT";
const int TCP_BUFFER_SIZE = 256;
```
## API

### Client messages
| name |type | data |
| - | - | - |
| MSG_CLIENT_BROADCAST_ECHO | 50 | [type (uint8) |

### Server messages
| name |type | data |
| - | - | - |
| MSG_SERVER_FULL | 255 | [type (uint8)] |
