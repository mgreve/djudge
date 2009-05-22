//-*- mode: c++; tab-width: 4; indent-tabs-mode: t; c-file-style: "stroustrup"; -*-
/*
 * djudge, an online judge solution.
 * Copyright (C) 2009 Jakob Truelsen
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <boost/program_options.hpp>
#include <iostream>
#include "error.hh"
#include "packagesocket.hh"
#include "string.h"
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <string>
#include <arpa/inet.h>
#include <signal.h>
#include "client.hh"
#include "drone.hh"
namespace po = boost::program_options;
using namespace std;

// class Drone {
// private:
// 	std::vector<Job *> jobs;
// 	pthread_cond_t not_empty;
// 	pthread_mutex_t mutex;
// 	PackageSocket s;
// 	void run() {
// 		try {
// 			while(true) {
// 				struct timespec t;
// 				clock_gettime(CLOCK_REALTIME, &t);
// 				t.tv_sec += 5;
// 				pthread_cond_timedwait(&not_empty, &mutex, &t);
// 				if(jobs.empty()) {
// 					pthread_mutex_unlock(&mutex);
// 					s.write("ping");
// 					if(s.readString(1024) != "pong") THROW_E("Did not send pong");
// 					pthread_mutex_lock(&mutex);
// 					continue;
// 				}
// 				Job * j = jobs.back();
// 				jobs.pop_back();
// 				pthread_mutex_unlock(&mutex);
// 				try {
// 					//Forward the job					
// 				} catch(std::exception & e) {
// 					pthread_mutex_lock(&mutex);
// 					jobs.push_back(j);
// 					pthread_mutex_unlock(&mutex);
// 					throw e;
// 				}
// 				pthread_mutex_lock(&mutex);
// 			}
// 		} catch(CommonException & e) {
// 			//Destroy drone here
// 		}
// 	}
// };

int s;
void handler(int) {
	cerr << "Overlord Parachute Deployed" << endl;
	close(s);
	exit(1);
};

void* dispatch(void * _) {
	ptrdiff_t cs = reinterpret_cast<ptrdiff_t>(_);
	printf("Connect\n");
	try {
		PackageSocket ss(cs);
		string type=ss.readString(20);
		if(type == "syncclient") {
			ss.write("success");
			SyncClient::run(ss);
		} else if(type == "asyncclient") {
			ss.write("success");
			ASyncClient::run(ss);
		} else if(type == "drone") {
			ss.write("success");
			Drone::run(ss);
		} else {
			ss.write("invalid type");
		}
	} catch(std::exception& e) {
		cerr << "Exception: " << e.what() << endl;
	}
	close(s);
	return NULL;
}

void runServer(int port) {
	signal(SIGHUP, handler);
	signal(SIGINT, handler);
	signal(SIGQUIT, handler);
	signal(SIGILL, handler);
	signal(SIGABRT, handler);
	signal(SIGFPE, handler);
	signal(SIGSEGV, handler);
	signal(SIGPIPE, handler);
	signal(SIGTERM, handler);
	sockaddr_in a;
	s = socket(AF_INET,SOCK_STREAM,0);
	a.sin_family = AF_INET;
	a.sin_addr.s_addr = INADDR_ANY;
	a.sin_port = htons(port);
	if( bind(s,(struct sockaddr*)&a, sizeof(a)) == -1) THROW_PE("bind() failed");
	listen(s, 128);
	while(true) {
		socklen_t _=sizeof(a);
		ptrdiff_t ss =accept(s,(struct sockaddr*)&a,&_);
		pthread_t thread;
		pthread_create(&thread,NULL, dispatch, reinterpret_cast<void *>(ss));
	}
};



/*class Server {
private:
	std::vector<Drone *> freeDrones;
	};*/


int main(int argc, char ** argv) {
	int port;
	po::options_description cmdline("Drone overload");

	cmdline.add_options() 
		("help,h", "Display the message.")
		("port,p", po::value<int>(&port), "The port to bind to.");
   	po::variables_map vm;
	try {
		po::store(po::parse_command_line(argc,argv, cmdline), vm);
		po::notify(vm);
		if (vm.count("help")) {
			cout << cmdline << endl;
			exit(0);
		}
	} catch(std::exception & e) {
		cerr << "Argument error: " << e.what() << endl;
		cerr << cmdline << endl;
		exit(1);
	}
	runServer(1234);
};
