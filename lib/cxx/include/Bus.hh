/*
 * Copyright 2010 Erik Gilling
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef __Bus_hh__
#define __Bus_hh__

#include <stdint.h>

#include <string>
#include <vector>
#include <map>
using namespace std;

#include <Board.hh>
#include <Thread.hh>
#include <ProtoThread.hh>

class Bus {
public:
	string name;

	vector<Board *> boards;
	map<uint8_t, Board *> addrMap;

	Thread::Mutex lock[2];
	ProtoThread *thread;
};


#endif /* __Bus_hh__ */
