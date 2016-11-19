# Webster

Webster is a static content multithreaded webserver written in C based on epoll.
It supports HTTP 1.0 and HTTP 1.1.

## Dependencies

The apllication depends on libpthread.

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
	$ sudo ./webster -wdir=<your website directory> -logfile=<log file path> -workers=<number of workers>
	```
	
## Usage

Setup Webster as a daemon on your system and run using the appropriate wdir parameter.
Optionally you can set a log file to monitor the application and number of workers.
Default website directory is "."<br />
Default logfile is stdout<br />
Default number of workers is 8<br />
