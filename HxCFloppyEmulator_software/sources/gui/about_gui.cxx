/*
//
// Copyright (C) 2006-2025 Jean-François DEL NERO
//
// This file is part of HxCFloppyEmulator.
//
// HxCFloppyEmulator may be used and distributed without restriction provided
// that this copyright statement is not removed from the file and that any
// derivative work contains the original copyright notice and the associated
// disclaimer.
//
// HxCFloppyEmulator is free software; you can redistribute it
// and/or modify  it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// HxCFloppyEmulator is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
//   See the GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with HxCFloppyEmulator; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
//
*/

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <time.h>
#include <stdint.h>

extern "C"
{
	#include "libhxcfe.h"
	#include "usb_hxcfloppyemulator.h"
	#include "version.h"
}

#include "fl_includes.h"

#include "about_gui.h"
#include "license_gui.h"

#include "main.h"
#include "gui_strings.h"

#define AUDIO_RATE    44100
#define AUDIO_CHANNEL 2

extern s_gui_context * guicontext;

static char buffer2[1024*16];
static char buffer1[1024*16];

int audiostarted;
int demostate;
uintro_context * gb_ui_context;

#ifdef WIN32
static HWAVEOUT shwd;
static WAVEFORMATEX pwfx;
static WAVEHDR pwhOut1;
static WAVEHDR pwhOut2;

void CALLBACK SoundHandlerProc(HWAVEOUT hwo,UINT uMsg,DWORD * dwInstance,DWORD *  dwParam1,DWORD *  dwParam2)
{
	if(audiostarted)
	{
		switch(uMsg)
		{
			case WOM_OPEN:
				break;
			case WOM_DONE:
				uintro_getnext_soundsample(gb_ui_context,(short*)((struct wavehdr_tag *)dwParam1)->lpData,sizeof(buffer2)/2);
				waveOutWrite(shwd,(struct wavehdr_tag *)dwParam1,sizeof(pwhOut2));
				break;
			case WOM_CLOSE:
				break;
		}
	}
	return;
}
#else
	#ifdef LINUX_AUDIOSUPPORT
	#include <pulse/pulseaudio.h>

	pa_threaded_mainloop *mainloop;
	pa_mainloop_api *mainloop_api;
	pa_context *context;
	pa_stream *stream;
	pa_sample_spec sample_specifications;
	pa_channel_map map;
	pa_stream_flags_t stream_flags;
	pa_buffer_attr buffer_attr;

	void context_state_cb(pa_context* context, void* mainloop)
	{
		pa_threaded_mainloop_signal((pa_threaded_mainloop *)mainloop, 0);
	}

	void stream_state_cb(pa_stream *s, void *mainloop)
	{
		pa_threaded_mainloop_signal((pa_threaded_mainloop *)mainloop, 0);
	}

	void stream_write_cb(pa_stream *stream, size_t requested_bytes, void *userdata)
	{
		size_t bytes_remaining = requested_bytes;

		while (bytes_remaining > 0)
		{
			uint8_t *buffer = NULL;
			size_t bytes_to_fill = AUDIO_RATE;

			if (bytes_to_fill > bytes_remaining)
				bytes_to_fill = bytes_remaining;

			pa_stream_begin_write(stream, (void**) &buffer, &bytes_to_fill);

			uintro_getnext_soundsample(gb_ui_context,(short*)buffer,bytes_to_fill/2);

			pa_stream_write(stream, buffer, bytes_to_fill, NULL, 0LL, PA_SEEK_RELATIVE);

			bytes_remaining -= bytes_to_fill;
		}
	}

	void stream_success_cb(pa_stream *stream, int success, void *userdata)
	{
		return;
	}
	#endif

	#ifdef __APPLE__
	#include "AudioToolbox/AudioToolbox.h"

	AudioQueueRef queue;
	AudioQueueBuffer *abuf;
	OSStatus status;
	AudioStreamBasicDescription fmt = { 0 };
	AudioQueueBufferRef buf_ref, buf_ref2;

	void callback (void *ptr, AudioQueueRef queue, AudioQueueBufferRef buf_ref)
	{
		OSStatus status;
		AudioQueueBuffer *buf = buf_ref;

		uintro_getnext_soundsample(gb_ui_context,(short*)buf->mAudioData, buf->mAudioDataByteSize / 2);

		status = AudioQueueEnqueueBuffer (queue, buf_ref, 0, NULL);
	}

	#endif

#endif

int startAudioOut()
{
	if(!audiostarted)
	{
		memset(buffer1,0,sizeof(buffer1));
		memset(buffer2,0,sizeof(buffer2));

		#ifdef WIN32

		if(waveOutGetNumDevs()!=0)
		{
			pwfx.wFormatTag=1;
			pwfx.nChannels = AUDIO_CHANNEL;
			pwfx.nSamplesPerSec = AUDIO_RATE;
			pwfx.nBlockAlign = 2 * AUDIO_CHANNEL;
			pwfx.nAvgBytesPerSec = pwfx.nSamplesPerSec * 2 * AUDIO_CHANNEL;
			pwfx.wBitsPerSample = 16;
			pwfx.cbSize = 0;

			waveOutOpen(&shwd,WAVE_MAPPER,&pwfx,(DWORD_PTR)&SoundHandlerProc,0,CALLBACK_FUNCTION);
			pwhOut1.lpData=(char*)buffer1;
			pwhOut1.dwBufferLength=sizeof(buffer1);
			pwhOut1.dwFlags=0;
			pwhOut1.dwLoops=0;

			pwhOut2.lpData=(char*)buffer2;
			pwhOut2.dwBufferLength=sizeof(buffer2);
			pwhOut2.dwFlags=0;
			pwhOut2.dwLoops=0;

			waveOutPrepareHeader(shwd, &pwhOut1, sizeof(pwhOut1));
			waveOutPrepareHeader(shwd, &pwhOut2, sizeof(pwhOut2));

			waveOutWrite(shwd,&pwhOut1,sizeof(pwhOut1));
			waveOutWrite(shwd,&pwhOut2,sizeof(pwhOut2));

			audiostarted = 1;
		}

		#else
		#ifdef LINUX_AUDIOSUPPORT
		mainloop = pa_threaded_mainloop_new();
		if(!mainloop)
			goto audio_init_error;

		mainloop_api = pa_threaded_mainloop_get_api(mainloop);
		if(!mainloop_api)
			goto audio_init_error;

		context = pa_context_new(mainloop_api, "HxCFloppyEmulator - About");
		if(!context)
			goto audio_init_error;

		pa_context_set_state_callback(context, &context_state_cb, mainloop);

		pa_threaded_mainloop_lock(mainloop);

		pa_threaded_mainloop_start(mainloop);

		pa_context_connect(context, NULL, PA_CONTEXT_NOAUTOSPAWN, NULL);

		// Wait for the context to be ready
		for(;;)
		{
			pa_context_state_t context_state = pa_context_get_state(context);
			if (context_state == PA_CONTEXT_READY)
				break;
			pa_threaded_mainloop_wait(mainloop);
		}

		sample_specifications.format = PA_SAMPLE_S16NE;
		sample_specifications.rate = AUDIO_RATE;
		sample_specifications.channels = 2;

		pa_channel_map_init_stereo(&map);

		stream = pa_stream_new(context, "Playback", &sample_specifications, &map);
		if(!stream)
			goto audio_init_error;

		pa_stream_set_state_callback(stream, stream_state_cb, mainloop);
		pa_stream_set_write_callback(stream, stream_write_cb, mainloop);

		// Recommended settings, i.e. server uses sensible values
		buffer_attr.maxlength = (uint32_t) -1;
		buffer_attr.tlength = (uint32_t) -1;
		buffer_attr.prebuf = (uint32_t) -1;
		buffer_attr.minreq = (uint32_t) -1;

		stream_flags = (pa_stream_flags_t)(PA_STREAM_START_CORKED | PA_STREAM_INTERPOLATE_TIMING | PA_STREAM_NOT_MONOTONIC | PA_STREAM_AUTO_TIMING_UPDATE | PA_STREAM_ADJUST_LATENCY);

		pa_stream_connect_playback(stream, NULL, &buffer_attr, stream_flags, NULL, NULL);

		// Wait for the stream to be ready
		for(;;)
		{
			pa_stream_state_t stream_state = pa_stream_get_state(stream);
			if (stream_state == PA_STREAM_READY) break;
			pa_threaded_mainloop_wait(mainloop);
		}

		pa_threaded_mainloop_unlock(mainloop);

		// Uncork the stream so it will start playing
		pa_stream_cork(stream, 0, stream_success_cb, mainloop);

		audiostarted = 1;

		return 0;

audio_init_error:
		if(mainloop)
			pa_threaded_mainloop_stop(mainloop);

		if(stream)
		{
			pa_stream_disconnect(stream);
			pa_stream_unref(stream);
			stream = NULL;
		}

		if(context)
		{
			pa_context_disconnect(context);
			pa_context_unref(context);
			context = NULL;
		}

		if(mainloop)
		{
			pa_threaded_mainloop_free(mainloop);
			mainloop = NULL;
		}

		#endif

		#ifdef __APPLE__

		fmt.mSampleRate = AUDIO_RATE;
		fmt.mFormatID = kAudioFormatLinearPCM;
		fmt.mFormatFlags = kAudioFormatFlagIsSignedInteger | kAudioFormatFlagIsPacked;
		fmt.mFramesPerPacket = 1;
		fmt.mChannelsPerFrame = AUDIO_CHANNEL;
		fmt.mBytesPerPacket = fmt.mBytesPerFrame = 2 * AUDIO_CHANNEL;
		fmt.mBitsPerChannel = 16;

		status = AudioQueueNewOutput(&fmt, callback, NULL, CFRunLoopGetCurrent(), kCFRunLoopCommonModes, 0, &queue);

		if (status == kAudioFormatUnsupportedDataFormatError)
			goto audio_init_error;

		status = AudioQueueAllocateBuffer (queue, AUDIO_RATE/2, &buf_ref);
		abuf = buf_ref;

		abuf->mAudioDataByteSize = AUDIO_RATE/2;

		callback (NULL, queue, buf_ref);

		status = AudioQueueAllocateBuffer (queue, AUDIO_RATE/2, &buf_ref2);

		abuf = buf_ref2;
		abuf->mAudioDataByteSize = AUDIO_RATE/2;

		callback (NULL, queue, buf_ref2);

		status = AudioQueueSetParameter (queue, kAudioQueueParam_Volume, 1.0);
		status = AudioQueueStart (queue, NULL);

		audiostarted = 1;

		return 0;

audio_init_error:
		return 0;

		#endif

		return 0;

		#endif
	}

	return 0;
}

int stopAudioOut()
{
	if(audiostarted)
	{
		audiostarted = 0;

		#ifdef WIN32

		waveOutReset(shwd);
		waveOutBreakLoop(shwd);
		waveOutBreakLoop(shwd);

		waveOutUnprepareHeader(shwd,&pwhOut1,sizeof(WAVEHDR));
		waveOutUnprepareHeader(shwd,&pwhOut2,sizeof(WAVEHDR));

		waveOutClose(shwd);

		#else

		#ifdef LINUX_AUDIOSUPPORT

		if(mainloop)
			pa_threaded_mainloop_stop(mainloop);

		if(stream)
		{
			pa_stream_disconnect(stream);
			pa_stream_unref(stream);
			stream = NULL;
		}

		if(context)
		{
			pa_context_disconnect(context);
			pa_context_unref(context);
			context = NULL;
		}

		if(mainloop)
		{
			pa_threaded_mainloop_free(mainloop);
			mainloop = NULL;
		}
		#endif

		#ifdef __APPLE__
		AudioQueueStop (queue, true);
		AudioQueueDispose(queue, true);
		#endif

		#endif
	}

	return 0;
}

static void tick(void *v)
{
	About_box *window;
	unsigned char * ptr1;
	int i,j,k;

	window=(About_box *)v;

	if(window->shown())
	{
		demostate = 1;
		startAudioOut();
		window->make_current();
		uintro_getnextframe(window->ui_context);

		ptr1=(unsigned char*)window->ui_context->framebuffer;
		k=0;
		j=0;
		for(i=0;i<window->xsize*window->ysize;i++)
		{
			ptr1[j++]=ptr1[k+2];
			ptr1[j++]=ptr1[k+1];
			ptr1[j++]=ptr1[k+0];
			k=k+4;
		}

		fl_draw_image((unsigned char *)window->ui_context->framebuffer, window->xpos_size, window->ypos_size, window->xsize, window->ysize, 3, 0);
	}
	else
	{
		if(demostate)
		{
			demostate = 0;
			uintro_reset(window->ui_context);
		}
		stopAudioOut();
	}


	Fl::repeat_timeout(0.02, tick, v);
}

void close(Fl_Widget *w, void * t)
{
	stopAudioOut();
	w->parent()->hide();
}

void create_license_window(Fl_Widget *, void *)
{
	new License_box();
	return ;
}

void OpenURLInBrowser(Fl_Widget *,void* u)
{
	#if defined (WIN32)
		char * url;
		url=(char*)u;

		ShellExecute(HWND_DESKTOP, "open", url, NULL, NULL, SW_SHOW);
	#elif defined (OSX)
		char * url;
		char commandString[2048];

		url=(char*)u;

		snprintf(commandString,sizeof(commandString), "open %s", url);
		system(commandString);
	#elif defined (__amigaos4__)
		char * url;
		char commandString[2048];

		url=(char*)u;

		sprintf(commandString, "ibrowse:ibrowse %s", url);
		system(commandString);
	#endif
}

About_box::~About_box()
{
	Fl::remove_timeout(tick,0);

	stopAudioOut();

	uintro_deinit(this->ui_context);
}

#define BUTTONS_BLOCK_XPOS 5
#define BUTTONS_BLOCK_YPOS 100

#define BUTTON_SIZE_X 180
#define BUTTON_SIZE_Y 25

About_box::About_box()
  : Fl_Window(530,240)
{
	int cur_xpos,cur_ypos;

	o = new Fl_Box(5, 5, BUTTON_SIZE_X, BUTTON_SIZE_Y, "HxC Floppy Emulator");
	o->box(FL_DOWN_BOX);

	cur_xpos = BUTTONS_BLOCK_XPOS;
	cur_ypos = BUTTONS_BLOCK_YPOS;

	button_wesite = new Fl_Button(cur_xpos, cur_ypos, BUTTON_SIZE_X, BUTTON_SIZE_Y, "Website");
	button_wesite->callback(OpenURLInBrowser,(void*)"https://hxc2001.com/");

	cur_ypos += BUTTON_SIZE_Y;

	button_forum = new Fl_Button(cur_xpos, cur_ypos, BUTTON_SIZE_X / 2, BUTTON_SIZE_Y, "Forum");
	button_forum->callback(OpenURLInBrowser,(void*)"http://torlus.com/floppy/forum");

	button_facebook = new Fl_Button(cur_xpos + (BUTTON_SIZE_X / 2), cur_ypos, BUTTON_SIZE_X / 2, BUTTON_SIZE_Y, "Facebook");
	button_facebook->callback(OpenURLInBrowser,(void*)"https://www.facebook.com/groups/hxc2001/");

	cur_ypos += BUTTON_SIZE_Y;

	button_releasenotes = new Fl_Button(cur_xpos, cur_ypos, BUTTON_SIZE_X, BUTTON_SIZE_Y, "Latest release notes");
	button_releasenotes->callback(OpenURLInBrowser,(void*)"https://hxc2001.com/download/floppy_drive_emulator/hxcfloppyemulator_soft_release_notes.txt");

	cur_ypos += BUTTON_SIZE_Y;

	button_license = new Fl_Button(cur_xpos, cur_ypos, BUTTON_SIZE_X, BUTTON_SIZE_Y, "Under GPL License");
	button_license->callback(create_license_window,0);

	cur_ypos += BUTTON_SIZE_Y;

	button_ok = new Fl_Button(cur_xpos, cur_ypos, BUTTON_SIZE_X / 2, BUTTON_SIZE_Y, getString(STR_COMMON_OK) ); // Fl_Button* o
	button_ok->callback(close,0);

	o = new Fl_Box(200, 5, 320+6, 200+6);
	o->box(FL_UP_BOX);// Fl_Box* o

	o = new Fl_Box(200, 200+6 + 5, 320+6, BUTTON_SIZE_Y, "hxc2001.com Mail: hxc2001(at)hxc2001.com");
	o->box(FL_DOWN_BOX);

	o = new Fl_Box(5, 35, 180, 60, "Copyright (c) 2006-2025\nJean-FranÃ§ois DEL NERO\n(c) HxC2001");
	o->box(FL_DOWN_BOX);

	xpos_size=200+3;
	ypos_size=5+3;

	xsize=320;
	ysize=200;

	ui_context=uintro_init(xsize,ysize);
	gb_ui_context = ui_context;

	this->end();

	windowname[0]=0;
	strcpy(windowname,NOMFENETRE" - libhxcfe v");
	strcat(windowname,hxcfe_getVersion(guicontext->hxcfe));

	this->label(windowname);

	audiostarted = 0;
	demostate = 0;

	Fl::add_timeout( 0.02, tick, (void*)this);

	return;
}
