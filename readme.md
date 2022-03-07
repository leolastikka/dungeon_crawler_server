# Dungeon crawler server

## Description and info
A simple UDP server for a realtime pseudo 3D dungeon crawler game, written in C language.

Build and run:
```
sudo apt install build-essentials
git clone https://github.com/leolastikka/dungeon_crawler_server.git
cd dungeon_crawler_server
make
./build/server
```
## Configuration and API
Server configuration in main.c:
```
PORT 6666
MAX_CONNECTIONS 3
UDP_BUFFER_SIZE 512
CLIENT_TIMEOUT_SEC 3
GAME_FPS 10
```

### Client messages
| name |type | data (datatype) |
| - | - | - |
| MSG_CLIENT_CONNECT | 1 | [type (uint8), client id (uint32)] |
| MSG_CLIENT_DISCONNECT | 2 | [type (uint8), client id (uint32)] |
| MSG_CLIENT_KEEPALIVE | 3 | [type (uint8), client id (uint32)] |
| MSG_CLIENT_INPUT | 4 | [type (uint8), client id (uint32), input bits (uint8)] |

### Server messages
| name |type | data (datatype) |
| - | - | - |
| MSG_SERVER_CLIENT | 5 | [type (uint8), client id (uint32), entity id (uint32)] |
| MSG_SERVER_ADD | 6 | [type (uint8), entity type (uint8), entity id (uint32), pos x (float), pos y (float)] |
| MSG_SERVER_REMOVE | 7 | [type (uint8), entity id (uint32)] |
| MSG_SERVER_MOVE | 8 | [type (uint8), entity id (uint32), pos x (float), pos y (float), vel x (float), vel y (float)] |
| MSG_SERVER_ACK | 254 | [type (uint8)] |
| MSG_SERVER_FULL | 255 | [type (uint8)] |
