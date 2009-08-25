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

#include <MapFileParser.hh>

#include "Soma.hh"
#include "FlameProtoThread.hh"
#include "LedProtoThread.hh"

Soma::Soma()
{
	int s;
	int i;

	ledIdx = 0;
	flameIdx = 0;

	for (s = 0; s < 2; s++) {
		for(i = 0; i < nLights; i++)
			state[s].lights[i] = 0x0;

		for(i = 0; i < nRelays; i++)
			state[s].relays[i] = false;

		for(i = 0; i < nButtons; i++)
			state[s].buttons[i] = false;

		for(i = 0; i < nKnobs; i++)
			state[s].knobs[i] = 0x0;
	}

	ledProtoThread = NULL;
	flameProtoThread = NULL;

	flameDone = false;
	ledDone = false;
	flameFrameDone = false;
	ledFrameDone = false;

	pthread_mutex_init(&lock, NULL);

	pthread_cond_init(&frameCond, NULL);
	pthread_cond_init(&flameCond, NULL);
	pthread_cond_init(&ledCond, NULL);
}

Soma::~Soma()
{
	if (ledProtoThread)
		delete ledProtoThread;

	if (flameProtoThread)
		delete flameProtoThread;
}


void Soma::attachLedLink(Link *l)
{
	ledProtoThread = new LedProtoThread(this, l);
	ledProtoThread->start();
}

void Soma::attachFlameLink(Link *l)
{
	flameProtoThread = new FlameProtoThread(this, l);
	flameProtoThread->start();
}

bool Soma::loadAttrMap(char *fileName, EffectAttr *attrs, int nAttrs)
{
	MapFileParser p;
	char *key;
	int val;
	int i = 0;

	if (!p.open(fileName))
		return false;

	while (p.read(&key, &val)) {
		if (i >= nAttrs) {
			fprintf(stderr, "too many lines in %s\n", fileName);
			return false;
		}
		attrs[i].name = key;
		attrs[i].val = val;
		i++;
	}

	return true;
}

bool Soma::loadLightMap(char *fileName)
{
	return loadAttrMap(fileName, lightAttrs, nLights);
}

bool Soma::loadRelayMap(char *fileName)
{
	return loadAttrMap(fileName, relayAttrs, nRelays);
}

bool Soma::loadDpotMap(char *fileName)
{
	return loadAttrMap(fileName, dpotAttrs, nDpots);
}

bool Soma::loadButtonMap(char *fileName)
{
	return loadAttrMap(fileName, buttonAttrs, nButtons);
}

bool Soma::loadKnobMap(char *fileName)
{
	return loadAttrMap(fileName, knobAttrs, nKnobs);
}


void Soma::run(void)
{
	struct timeval tv;
	struct timeval last_tv;
	struct timeval tmp_tv;
	struct timeval frametime;
	int i = 0;

	frametime.tv_sec = 0;
	frametime.tv_usec = 10000;

	gettimeofday(&last_tv, NULL);
	while(1) {
		sync();
//		printf("\n\n%d\n\n",i);

		processFrame(i++);

		gettimeofday(&tv, NULL);
		timersub(&tv, &last_tv, &tmp_tv);
		if (timercmp(&tmp_tv, &frametime, <)) {
			timersub(&frametime, &tmp_tv, &tv);
			printf("sleeping for %dus\n", tv.tv_usec);
			gettimeofday(&tmp_tv, NULL);
			usleep(tv.tv_usec);
			gettimeofday(&last_tv, NULL);
			timersub(&last_tv, &tmp_tv, &tv);
			printf("slept for %dus\n", tv.tv_usec);
		} else {
			fprintf(stderr, "frame overrun %d: ", i);
			timersub(&flameSyncTime, &last_tv, &tmp_tv);
			fprintf(stderr, "flameSyncDelay: %dus ",
				tmp_tv.tv_usec);
			timersub(&ledSyncTime, &last_tv, &tmp_tv);
			fprintf(stderr, "ledSyncDelay: %dus ",
				tmp_tv.tv_usec);
			fprintf(stderr, "\n");
		}
		gettimeofday(&last_tv, NULL);
	}
}

void Soma::processFrame(int frame)
{
	int state = (frame / (0x100 + 1000)) % 6;
	int idx = frame % (0x100 + 1000);
	int red = 0;
	int green = 0;
	int blue = 0;
	int i;

	if (idx > 0xff)
		idx = 0xff;

	switch (state) {
	case 0:
		red = 0xff;
		green = idx;
		blue = 0x00;
		break;

	case 1:
		red = 0xff - idx;
		green = 0xff;
		blue = 0x00;
		break;

	case 2:
		red = 0x00;
		green = 0xff;
		blue = idx;
		break;

	case 3:
		red = 0x00;
		green = 0xff - idx;
		blue = 0xff;
		break;

	case 4:
		red = idx;
		green = 0x00;
		blue = 0xff;
		break;

	case 5:
		red = 0xff;
		green = 0x00;
		blue = 0xff - idx;
		break;
	}

	for (i = 0; i < nButtons; i++) {
		setRelay(i * 3 , button(i));
		setRelay(i * 3 + 1, button(i));
		setRelay(i * 3 + 2, button(i));
	}

	if (false && (frame % 100) == 0) {
		for (i = 0; i < nButtons; i++)
			printf("%c ", button(i) ? '1' : '0');

		printf("\n");
	}
	for (i = 0; i < nLights; i++)
		setLight(i, red, green, blue);

}

void Soma::sync(void)
{

	printf("sync locking\n");
	pthread_mutex_lock(&lock);
	printf("%d %d\n", flameDone, ledDone); fflush(stdout);
	while (!flameDone || !ledDone) {
		pthread_cond_wait(&frameCond, &lock);
	}
	flameIdx = !flameIdx;
	ledIdx = !ledIdx;

	ledDone = false;
	flameDone = false;

	flameFrameDone = true;
	ledFrameDone = true;


	pthread_cond_broadcast(&flameCond);
	pthread_cond_broadcast(&ledCond);

	pthread_mutex_unlock(&lock);


}

void Soma::flameSync(void)
{
	gettimeofday(&flameSyncTime, NULL);

	pthread_mutex_lock(&lock);
	flameDone = true;
	pthread_cond_broadcast(&frameCond);


	while (!flameFrameDone) {
		pthread_cond_wait(&flameCond, &lock);
	}

	flameFrameDone = false;

	pthread_mutex_unlock(&lock);
}


void Soma::ledSync(void)
{
	gettimeofday(&ledSyncTime, NULL);

	pthread_mutex_lock(&lock);
	ledDone = true;
	pthread_cond_broadcast(&frameCond);


	while (!ledFrameDone) {
		pthread_cond_wait(&ledCond, &lock);
	}

	ledFrameDone = false;

	pthread_mutex_unlock(&lock);

}

