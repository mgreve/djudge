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
#include "error.hh"
#include "globals.hh"
#include "jobmanager.hh"
#include "results.hh"
#include "validation.hh"
#include <iostream>
#include <sstream>
#include <stdlib.h>
using namespace std;
pthread_mutex_t ASyncClient::cookieMapMutex;
std::map<std::string, ASyncClient *> ASyncClient::cookieMap;

bool Client::handleCommand(const std::string & cmd, PackageSocket & s) {
	if(cmd == "status") {
		s.write(XSTR(RUN_SUCCESS));
		ostringstream res;
		res << "The overlord is up and running" << endl
			<< "Drones:       " << JobManager::drones.size() << endl
			<< "Free:         " << JobManager::freeDrones.size() << endl
			<< "Pending jobs: " << JobManager::jobQueue.size();
		s.write(res.str());
	} else if(cmd == "" || cmd == "leave")
		return false;
	else if(cmd == "identify") {
		//TODO implement me
		string name=s.readString(1024);
		string password=s.readString(1024);
		s.write(XSTR(RUN_SUCCESS));
		s.write("success");
	} else if(cmd == "list") {
		pthread_mutex_lock(&entriesMutex);
		std::set<std::string> e(entries.begin(),entries.end());
		pthread_mutex_unlock(&entriesMutex);
		for(std::set<std::string>::iterator i=e.begin(); i != e.end(); ++i)
			s.write((*i));
		s.write("");
	} else if(cmd == "dispose") {
		string name=s.readString(ENTRY_NAME_LENGTH);
		if(!validateEntryName(name)) {
			s.write(XSTR(RUN_INVALID_ENTRY));
			s.write("Invalid entry name");
			return true;
		}
		pthread_mutex_lock(&entriesMutex);
		entries.erase(name);
		pthread_mutex_unlock(&entriesMutex);
		JobManager::addJob(dispose,this,name);
		unlink((entriesPath+"/"+name).c_str());		
		s.write(XSTR(RUN_SUCCESS));
		s.write("Lazily disposing entry");
	} else if(cmd == "push") {
		string name=s.readString(ENTRY_NAME_LENGTH);
		if(!validateEntryName(name)) {
			s.write(XSTR(RUN_INVALID_ENTRY));
			s.write("Invalid entry name");
			return true;
		}
		pthread_mutex_lock(&entriesMutex);
		bool found=entries.count(name)>0;
		pthread_mutex_unlock(&entriesMutex);
		if(!found) {
			s.write(XSTR(RUN_INVALID_ENTRY));
			s.write("The entry does not exist");
			return true;
		}
		JobManager::addJob(push,this,name);
		s.write(XSTR(RUN_SUCCESS));
		s.write("Lasily pushing entry");
	} else if(cmd == "judge") {
		string name=s.readString(ENTRY_NAME_LENGTH);
		string lang=s.readString(LANG_NAME_LENGTH);
		char buff[64];
		strcpy(buff,"/tmp/codeXXXXXX");
		int f = mkstemp(buff);
		if(f == -1) THROW_PE("mkstemp() failed");
		s.readFD(f);
		close(f);
		pthread_mutex_lock(&entriesMutex);
		bool found=validateEntryName(name) && entries.count(name)>0;
		pthread_mutex_unlock(&entriesMutex);
		if(!found) {
			s.write(XSTR(RUN_INVALID_ENTRY));
			s.write("The entry not found");
			unlink(buff);
			return true;
		}
		uint64_t id = JobManager::addJob(judge,this,name,lang,buff);
		jobHook(s,id);
	} else if(cmd == "import") {
		string name=s.readString(ENTRY_NAME_LENGTH);
		char buff[64];
		strcpy(buff,(entriesPath+"/.tmpXXXXXX").c_str());
		int f = mkstemp(buff);
		if(f==-1) THROW_PE("mkstemp() failed");
		s.readFD(f);
		close(f);
		if(!validateEntryName(name)) {
			s.write(XSTR(RUN_INVALID_ENTRY));
			s.write("Invalid entry name");
			unlink(buff);
			return true;
		}
		pthread_mutex_lock(&entriesMutex);
		bool found=entries.count(name)>0;
		pthread_mutex_unlock(&entriesMutex);
		if(found) {
			s.write(XSTR(RUN_INVALID_ENTRY));
			s.write("An entry with that name allredy exists");
			unlink(buff);
			return true;
		} 
		uint64_t id = JobManager::addJob(import,this,name,buff);
		jobHook(s,id);
	} else {
		s.write(XSTR(RUN_BAD_COMMAND));
		s.write("Unknown command "+cmd);
	}
	return true;
}

void Client::run_(PackageSocket & s) {
	while(s.canRead()) {
		string cmd = s.readString(COMMAND_LENGTH);
		if(!handleCommand(cmd,s)) break;
	}
}

ASyncClient::ASyncClient() {
	pthread_mutex_init(&resultMutex,NULL);
}

ASyncClient::~ASyncClient() {
	pthread_mutex_destroy(&resultMutex);
}

void ASyncClient::run(PackageSocket & s) {
	string cookie = s.readString(COOKIE_LENGTH);
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

void ASyncClient::jobHook(PackageSocket & s, uint64_t jobid) {
	ostringstream r;
	r << jobid;
	s.write(XSTR(RUN_PENDING));
	s.write(r.str());
}

void ASyncClient::jobDone(Job * j) {
	pthread_mutex_lock(&resultMutex);
	results.push_back(j);
	pthread_mutex_unlock(&resultMutex);
}

bool ASyncClient::handleCommand(const std::string & cmd, PackageSocket & s) {
	if(cmd == "pullresults") {
		pthread_mutex_lock(&resultMutex);
		std::vector<Job*> res(results.begin(), results.end());
		results.clear();
		pthread_mutex_unlock(&resultMutex);		
		for(size_t i=0; i < res.size(); ++i) {
			ostringstream id,r;
			id << res[i]->id;
			s.write(id.str());
			r << res[i]->result;
			s.write(r.str());
			s.write(results[i]->msg);
			delete(results[i]);
		}
		s.write("");
		return true;
	} else
		return Client::handleCommand(cmd,s);
}

SyncClient::SyncClient() {
	pthread_mutex_init(&resultMutex,NULL);
	pthread_cond_init(&resultCond,NULL);
	job=NULL;
}

SyncClient::~SyncClient() {
	pthread_mutex_destroy(&resultMutex);
	pthread_cond_destroy(&resultCond);
}

void SyncClient::run(PackageSocket & s) {
	SyncClient c;
	c.run_(s);
}

void SyncClient::jobHook(PackageSocket & s, uint64_t) {
	pthread_mutex_lock(&resultMutex);
	while(job == NULL) pthread_cond_wait(&resultCond,&resultMutex);
	Job * j=job;
	job=NULL;
	pthread_mutex_unlock(&resultMutex);
	ostringstream r;
	r << j->result;
	s.write(r.str());
	s.write(j->msg);
	delete j;
}

void SyncClient::jobDone(Job * j) {
	pthread_mutex_lock(&resultMutex);
	job=j;
	pthread_cond_signal(&resultCond);
	pthread_mutex_unlock(&resultMutex);
}

