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
#ifndef __biglock_hh__
#define __biglock_hh__
#include <pthread.h>

class Unlock {
public:
	Unlock();
	~Unlock();
};
#define UNLOCK Unlock __unlock_;

#define ULCALL(X) ({UNLOCK; X;})

class Cond {
private:
	pthread_cond_t c;
public:
	Cond();
	~Cond();
	void signal();
	void wait();
	void timedWait(int secs);
};

extern pthread_mutex_t bigLock;
class BigLock {
public:
	BigLock();
	~BigLock();
};
#define BIGLOCK BigLock __biglock__

class Lock {
public:
	Lock();
	~Lock();
};
#define LOCK Lock __lock__
#endif //__biglock_hh__
