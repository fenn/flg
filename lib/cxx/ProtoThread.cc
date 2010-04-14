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


#include <string.h>
#include <stdio.h>

#include <ProtoThread.hh>

#include <Bus.hh>

ProtoThread::ProtoThread(Bus *b, Link *l) :
	Thread()
{
	vector<Board *>::iterator i;

	bus = b;
	link = l;
	page = 1;
	bus->lock[page].lock();

	proto = new Proto(link, this, NULL, 0, 0, true);

	minLedAddr = 0xff;
	maxLedAddr = 0x00;
	for (i = bus->boards.begin(); i != bus->boards.end(); i++) {
		Board *b = *i;

		if (b->lightOuts.size() > 0) {
			if (b->addr > maxLedAddr)
				maxLedAddr = b->addr;
			if (b->addr < minLedAddr)
				minLedAddr = b->addr;
		}
	}
	if (minLedAddr < maxLedAddr) {
		ledData = new uint32_t[maxLedAddr - minLedAddr + 1];
	} else {
		ledData = NULL;
	}
}

ProtoThread::~ProtoThread()
{
	delete proto;
}

void ProtoThread::sendBoard(Board *b)
{
	vector<DigitalOut *>::iterator di;
	vector<AnalogOut *>::iterator ai;

	if (b->digitalOuts.size() > 0) {
		uint8_t val = 0;
		for (di = b->digitalOuts.begin();
		     di != b->digitalOuts.end();
		     di++) {
			if ((*di) && (*di)->state[page])
				val |= 1 << (*di)->index;
		}
		proto->setRelay(b->addr, val);
		proto->clearRelay(b->addr, ~val);
	}

	for (ai = b->analogOuts.begin();
	     ai != b->analogOuts.end();
	     ai++) {
		if (*ai)
			proto->setDpot(b->addr, (*ai)->index, (*ai)->val[page]);
	}
}

void ProtoThread::sendLeds(void)
{
	vector<Board *>::iterator i;

	if (ledData == NULL)
		return;

	memset(ledData, 0, sizeof(uint32_t) * (maxLedAddr - minLedAddr + 1));
	for (i = bus->boards.begin(); i != bus->boards.end(); i++) {
		Board *b = *i;

		if (b->lightOuts.size() > 0) {
			LightOut *l = b->lightOuts[0];
			ledData[b->addr - minLedAddr] =
				l->red[page] | (l->green[page] << 8) |
				(l->blue[page] << 16);
		}
	}

	proto->setLights(minLedAddr, ledData, maxLedAddr - minLedAddr + 1);
	proto->flush();
}

void ProtoThread::pollBoard(Board *b)
{
	vector<AnalogIn *>::iterator ai;

	if (b->digitalIns.size() > 0)
		getDigitalIns(b);

	for (ai = b->analogIns.begin();
	     ai != b->analogIns.end();
	     ai++) {
		if (*ai)
			getAnalogIn(*ai);
	}

}

bool ProtoThread::getDigitalIns(Board *b)
{
	int retries = nRetries;
	vector<DigitalIn *>::iterator di;

	do {
		dataValid = false;
		proto->getSwitch(b->addr);
		proto->flush();
		proto->waitForMsg(timeoutUs);
		if (dataValid) {
			for (di = b->digitalIns.begin();
			     di != b->digitalIns.end();
			     di++) {
				if (*di)
					(*di)->state[page] = data & (1 << (*di)->index);
			}
			return true;
		}
	} while (--retries);

	fprintf(stderr, "digital in timeout %02x\n", b->addr);
	return false;
}

bool ProtoThread::getAnalogIn(AnalogIn *in)
{
	int retries = nRetries;
	uint16_t val;

	do {
		dataValid = false;
		proto->getAdcLo(in->board->addr, in->index);
		proto->flush();
		proto->waitForMsg(timeoutUs);
		if (!dataValid)
			continue;
		val = data;

		dataValid = false;
		proto->getAdcHi(in->board->addr, in->index);
		proto->flush();
		proto->waitForMsg(timeoutUs);
		if (!dataValid)
			continue;
		val |= data << 8;

		in->val[page] = val;
		return true;
	} while(--retries);

	fprintf(stderr, "analog in timeout %02x %d\n", in->board->addr, in->index);

	return false;
}


int ProtoThread::run(void)
{
	vector<Board *>::iterator bi;

	while (1) {
		for(bi = bus->boards.begin();
		    bi != bus->boards.end();
		    bi++) {
			sendBoard(*bi);
		}
		proto->flush();

		sendLeds();

		for(bi = bus->boards.begin();
		    bi != bus->boards.end();
		    bi++) {
			pollBoard(*bi);
		}

		bus->lock[page].unlock();
		page = !page;
		bus->lock[page].lock();
	}

        return 0;
}

void ProtoThread::relay(uint8_t idx, uint8_t state)
{
}

void ProtoThread::light(uint8_t idx, uint8_t val)
{
}

void ProtoThread::dpot(uint8_t idx, uint8_t val)
{
}

void ProtoThread::resp(struct proto_packet *pkt)
{
	data = pkt->val;
	dataValid = true;
}
