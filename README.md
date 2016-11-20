# Webster

Webster is a static content multithreaded webserver written in C based on epoll.
It supports tls and non-encrypted HTTP 1.0 or HTTP 1.1 get requests.

## Dependencies

The apllication depends on libpthread and libopenssl.

## Installation

1. Download the source:<br />
	```bash
	$ git clone https://github.com/FlorinPetriuc/Webster.git
	```
	
2. Compile:<br />
	```bash
	$ make
	```
	
3. Run:<br />
	```bash
    cd build/bin
	$ sudo ./webster -wdir=<your website directory> -logfile=<log file path> -workers=<number of workers> -port=<http server port> -sPort=<https server port> -certificate=<ssl certificate path with RSA key and chain authority> -no_ssl=<0 or 1>
	```
	
## Usage

Setup Webster as a daemon on your system and run using the appropriate wdir and port parameters.
Optionally you can set a log file to monitor the application and number of workers.<br />
Default website directory is "."<br />
Default logfile is stdout<br />
Default number of workers is 8<br />
Default port number is 80<br />
Default sPort number is 443<br />
Default certificate path is "./certificate.crt"<br />
Default no_ssl value is 0<br />

Parameters port and sPort may be configured to have the same value. Webster can distinguish unencrypted requests from TLS requests.
