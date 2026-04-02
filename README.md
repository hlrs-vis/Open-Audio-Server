# Open Audio Server

This project is separated into two components: server and client.

The server portion contains the source code and build system necessary to 
compile and run the Open Audio Server (OAS). The client component contains a
client API to help communicate with the server. There are also client examples 
that can be compiled and played to demonstrate the server's functionality. 

## Information from original project

| ---------------- | --- |
| *Project Author* | Shree Chowkwale |
| *Email*          | shree.chowkwale (AT) gmail |
| *Project Wiki*   | [http://ivl.calit2.net/wiki/index.php/OpenAL_Audio_Server](https://github.com/IVCenter/Open-Audio-Server/wiki) |
| *Github*         | [https://github.com/CalVR/Open-Audio-Server](https://github.com/IVCenter/Open-Audio-Server) |

## Changes in this repository
In this repository, it has been updated to use the [COVISE/OpenCOVER/Vistle configuration library](https://github.com/vistle/covconfig), so that it is not necessary to adapt to changes in the [MXML](https://www.msweet.org/mxml/) library for parsing configuration files. After setting the CMake option `TOML_CONFIG` to `OFF`, it should still be compatible with MXML 3.

## More information
For more information, please consult the [Wiki](https://github.com/IVCenter/Open-Audio-Server/wiki) of the [original project](https://github.com/IVCenter/Open-Audio-Server) or see the included file [README.wiki](README.wiki).

