/*
 * Copyright 2009 Erik Gilling
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

#ifndef __Soma_hh__
#define __Soma_hh__

#include <Proto.hh>

class FlameProtoThread;
class LedProtoThread;

class Soma
{
public:
	static const int nLights = 60;
	static const int nRelays = 33;
	static const int nDpots = 4;
	static const int nButtons = 11;
	static const int nKnobs = 4;

private:
	class State {
	public:
		uint32_t lights[nLights];
		bool relays[nRelays];
		uint8_t dpots[nDpots];
		bool buttons[nButtons];
		uint16_t knobs[nKnobs];
	};

	State state[2];

	int flameIdx;
	int ledIdx;

	Thread::Mutex flameLocks[2];
	Thread::Mutex ledLocks[2];

	FlameProtoThread *flameProto;
	LedProtoThread *ledProto;

	bool button(int i) {
		return state[flameIdx].buttons[i];
	}

	uint16_t knob(int i) {
		return state[flameIdx].knobs[i];
	}

	uint32_t light(int i) {
		return state[ledIdx].lights[i];
	}

	void setLight(int i, int r, int g, int b) {
		uint32_t val = (r & 0xff) |
			((g & 0xff) << 8) |
			((b & 0xff) << 16);

		state[ledIdx].lights[i] = val;
	}

	bool relay(int i) {
		return state[flameIdx].relays[i];
	}

	void setRelay(int i, bool val) {
		state[flameIdx].relays[i] = val;
	}

	uint8_t dpot(int i) {
		return state[flameIdx].dpots[i];
	}

	void setDpot(int i, uint8_t val) {
		state[flameIdx].dpots[i] = val;
	}

public:
	Soma();
	~Soma();

	void attachLedLink(Link *l);
	void attachFlameProto(Link *l);

	void run(void);

	bool getRelay(int i) {
		return state[!flameIdx].relays[i];
	}

	uint32_t getLight(int i) {
		return state[!ledIdx].lights[i];
	}

	void setKnob(int i, uint16_t val) {
		state[!flameIdx].knobs[i] = val;
	}

	void setButton(int i, bool val) {
		state[!flameIdx].buttons[i] = val;
	}

	uint8_t getDpot(int i) {
		return state[!flameIdx].dpots[i];
	}

	void flameSync(void);
	void ledSync(void);

	uint8_t getRelayAddr(int i);
	uint8_t getRelayIdx(int i);
	uint8_t getButtonAddr(int i);
	uint8_t getButtonIdx(int i);
	uint8_t getDpotAddr(int i);
	uint8_t getDpotIdx(int i);
	uint8_t getKnobAddr(int i);
	uint8_t getKnobIdx(int i);
	uint8_t getLightAddr(int i);

};

#endif /* __Soma_hh__ */

