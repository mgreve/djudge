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
#include "client.hh"
using namespace std;
// class SyncClient: public client {
// public:
// 	int code;
// 	std::string msg;
// 	pthread_cond_t cont;
// 	pthread_mutex_t mutex;
// 	virtual void addResponce(uint64_t jobid, int code, const std::string & message) {
// 		pthread_mutex_lock(&mutex);
// 		this->code = code;
// 		this->msg = message;
// 		ptheard_mutex_unlock(&mutex);
// 	};
	
// 	virtual void postCommandHook() {
// 		pthread_mutex_lock(&mutex);
// 		if(code == -1) pthread_cond_wait(&cond,mutex);
// 		char buff[12];
// 		sprintf(buff,"%d",code);
// 		ss.write(buff);
// 		ss.write(msg);
// 		ptheard_mutex_unlock(&mutex);
// 	}
// };

pthread_mutex_t ASyncClient::cookieMapMutex;
std::map<std::string, ASyncClient *> ASyncClient::cookieMap;

void ASyncClient::run(PackageSocket & s) {
	string cookie = s.readString(128);
	pthread_mutex_lock(&cookieMapMutex);
	ASyncClient * c;
	if(cookieMap.count(cookie) == 0) 
		c = cookieMap[cookie] = new ASyncClient();
	else 
		c = cookieMap[cookie];
	pthread_mutex_unlock(&cookieMapMutex);
	c->run_(s);
}


void ASyncClient::init() {
	pthread_mutex_init(&cookieMapMutex, NULL);
}

void SyncClient::run(PackageSocket & s) {
	SyncClient c;
	c.run_(s);
}

void Client::run_(PackageSocket & s) {
	//Foo Bar
}
