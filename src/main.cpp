// custom build and feature flags
#ifdef DEBUG
	#define OPENGL_DEBUG        1
	#define FULLSCREEN          0
	#define DESPERATE           0
	#define BREAK_COMPATIBILITY 0
#else
	#define OPENGL_DEBUG        0
	#define FULLSCREEN          1
	#define DESPERATE           0
	#define BREAK_COMPATIBILITY 0
#endif

#define TWO_PASS    1
#define USE_MIPMAPS 1
#define USE_AUDIO   1
#define NO_UNIFORMS 0
#define USE_INSTANSSI_LIGHTS 1

#include "definitions.h"
#if OPENGL_DEBUG
	#include "debug.h"
#endif


#include "glext.h"
#include "shaders/fragment.inl"
#if TWO_PASS
	#include "shaders/post.inl"
#endif

#if USE_INSTANSSI_LIGHTS
	#include<stdio.h>
	#include<winsock2.h>

	#pragma comment(lib,"ws2_32.lib") //Winsock Library

	#define LIGHT_SERVER "192.168.69.10"  //address of the server
	//#define LIGHT_SERVER "10.0.0.46"  //address of the server
    #define LIGHT_PORT 9909
	#define LIGHT_COUNT 24
	#define LIGHT_BUFFER_SIZE (1 + (LIGHT_COUNT * 6))
	//#define LIGHT_PACKET_EVERY_N_SAMPLES 2205;
	#define LIGHT_PACKET_INTERVAL 0.05
#endif

// Satisfy linker cries

extern "C"
{
	int _fltused = 1;
}

extern "C"
{
	// https://www.codeproject.com/articles/383185/sse-accelerated-case-insensitive-substring-search
	int __isa_enabled = 0;
}

extern "C"
{
	int __favor = 0;
}


#pragma code_seg(".envelope")
extern "C" float get_Envelope(int instrument)
{
	return (&_4klang_envelope_buffer)[((MMTime.u.sample >> 8) << 5) + 2 * instrument];
}

#if USE_INSTANSSI_LIGHTS
// https://instanssi.org/2018/valot/
void allLightsToRGB(char* packet, int r, int g, int b)
{
	packet[0] = 1; // Version of spec = 1
	for (int iLight = 0; iLight < LIGHT_COUNT; iLight++)
	{
		int segmentI = 1 + iLight * 6;
		packet[segmentI + 0] = 1; // effect type = 0 - light
		packet[segmentI + 1] = iLight; // light index
		packet[segmentI + 2] = 0; // type of light = 0 - rgb
		packet[segmentI + 3] = min(r, 255); // r
		packet[segmentI + 4] = min(g, 255); // g
		packet[segmentI + 5] = min(b, 255); // b
	}
}
#endif


#ifndef EDITOR_CONTROLS
void entrypoint(void)
#else
#include "song.h"
int CALLBACK WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
#endif
{
	#if USE_INSTANSSI_LIGHTS
		// https://www.binarytides.com/udp-socket-programming-in-winsock/
		struct sockaddr_in si_other;
		int s, slen = sizeof(si_other);
		char packet[LIGHT_BUFFER_SIZE];
		WSADATA wsa;
		char udpInfo[2048];
		float lastLightPacketAt = 0.0; // s

		//Initialise winsock
		if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
		{
			#if DEBUG
			//sprintf(udpInfo, "Failed. Error Code : %d", WSAGetLastError());
			MessageBox(NULL, udpInfo, "", 0x00000000L);
			ExitProcess(1);
			#endif
		}

		//create socket
		if ((s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == SOCKET_ERROR)
		{
			#if DEBUG
			//sprintf(udpInfo, "socket() failed with error code : %d", WSAGetLastError());
			MessageBox(NULL, "socket creation failed", "", 0x00000000L);
			ExitProcess(1);
			#endif
		}

		memset((char *)&si_other, 0, sizeof(si_other));
		si_other.sin_family = AF_INET;
		si_other.sin_port = htons(LIGHT_PORT);
		si_other.sin_addr.S_un.S_addr = inet_addr(LIGHT_SERVER);

		//if (sendto(s, packet, LIGHT_BUFFER_SIZE, 0, (struct sockaddr *) &si_other, slen) == SOCKET_ERROR)
		allLightsToRGB(packet, 60, 0, 0);
		sendto(s, packet, LIGHT_BUFFER_SIZE, 0, (struct sockaddr *) &si_other, slen);
	#endif

	// initialize window
	#if FULLSCREEN
		ChangeDisplaySettings(&screenSettings, CDS_FULLSCREEN);
		ShowCursor(0);
		const HDC hDC = GetDC(CreateWindow((LPCSTR)0xC018, 0, WS_POPUP | WS_VISIBLE, 0, 0, XRES, YRES, 0, 0, 0, 0));
	#else
		#ifdef EDITOR_CONTROLS
			HWND window = CreateWindow("static", 0, WS_POPUP | WS_VISIBLE, 0, 0, XRES, YRES, 0, 0, 0, 0);
			HDC hDC = GetDC(window);
		#else
			HDC hDC = GetDC(CreateWindow("static", 0, WS_POPUP | WS_VISIBLE, 0, 0, XRES, YRES, 0, 0, 0, 0));
		#endif
	#endif	

	// initalize opengl
	SetPixelFormat(hDC, ChoosePixelFormat(hDC, &pfd), &pfd);
	wglMakeCurrent(hDC, wglCreateContext(hDC));
	
	PID_QUALIFIER int pid = ((PFNGLCREATESHADERPROGRAMVPROC)wglGetProcAddress("glCreateShaderProgramv"))(GL_FRAGMENT_SHADER, 1, &fragment);
	#if TWO_PASS
		PID_QUALIFIER int pi2 = ((PFNGLCREATESHADERPROGRAMVPROC)wglGetProcAddress("glCreateShaderProgramv"))(GL_FRAGMENT_SHADER, 1, &post);
	#endif

	#if OPENGL_DEBUG
		shaderDebug(fragment, FAIL_KILL);
		#if TWO_PASS
			shaderDebug(post, FAIL_KILL);
		#endif
	#endif

	// initialize sound
	#ifndef EDITOR_CONTROLS
		#if USE_AUDIO
			CreateThread(0, 0, (LPTHREAD_START_ROUTINE)_4klang_render, lpSoundBuffer, 0, 0);
			waveOutOpen(&hWaveOut, WAVE_MAPPER, &WaveFMT, NULL, 0, CALLBACK_NULL);
			waveOutPrepareHeader(hWaveOut, &WaveHDR, sizeof(WaveHDR));
			waveOutWrite(hWaveOut, &WaveHDR, sizeof(WaveHDR));
		#endif
	#else
		long double position = 0.0;
		song track(L"audio.wav");
		track.play();
		start = timeGetTime();
	#endif

	// main loop
	do
	{
		#if !(DESPERATE)
			// do minimal message handling so windows doesn't kill your application
			// not always strictly necessary but increases compatibility a lot
			MSG msg;
			PeekMessage(&msg, 0, 0, 0, PM_REMOVE);
		#endif

		// render with the primary shader
		((PFNGLUSEPROGRAMPROC)wglGetProcAddress("glUseProgram"))(pid);
		#ifndef EDITOR_CONTROLS
			// if you don't have an audio system figure some other way to pass time to your shader
			#if USE_AUDIO
				waveOutGetPosition(hWaveOut, &MMTime, sizeof(MMTIME));
				// it is possible to upload your vars as vertex color attribute (gl_Color) to save one function import

				float timeNow = float(MMTime.u.sample) / SAMPLE_RATE;
				float kick = get_Envelope(0); // kick
				float snare = get_Envelope(1); // snare
				#if NO_UNIFORMS
					glColor3ui(MMTime.u.sample, 0, 0);
				#else
					// remember to divide your shader time variable with the SAMPLE_RATE (44100 with 4klang)
					((PFNGLUNIFORM1IPROC)wglGetProcAddress("glUniform1i"))(0, MMTime.u.sample);
					float songInfo[2];
					songInfo[0] = kick;
					songInfo[1] = snare;
					float misc[2];
					misc[0] = float(MMTime.u.sample) / 22000.0f;
					misc[1] = float(MMTime.u.sample) / 11000.0f;
					((PFNGLUNIFORM1FVPROC)wglGetProcAddress("glUniform2fv"))(1, 1, songInfo); //
					((PFNGLUNIFORM1FVPROC)wglGetProcAddress("glUniform2fv"))(2, 1, misc); //


				#endif
				#if USE_INSTANSSI_LIGHTS
					float timeSinceLastPacket = timeNow - lastLightPacketAt;
					if (timeSinceLastPacket >= LIGHT_PACKET_INTERVAL)
					{
						if (timeNow >= 19.0 && snare > 0.2) {
							int l = int(snare * 255.0);
							allLightsToRGB(packet, l, l, l);
						}
						else {
							allLightsToRGB(packet, 60, 0, 0);
						}
						sendto(s, packet, LIGHT_BUFFER_SIZE, 0, (struct sockaddr *) &si_other, slen);
						lastLightPacketAt = timeNow;
					}
				#endif
			#endif
		#else
			refreshShaders(pid, pi2);
			position = track.getTime();
			((PFNGLUNIFORM1IPROC)wglGetProcAddress("glUniform1i"))(0, ((int)(position*44100.0)));
		#endif
		glRects(-1, -1, 1, 1);

		// render "post process" using the opengl backbuffer
		#if TWO_PASS
			glBindTexture(GL_TEXTURE_2D, 1);
			#if USE_MIPMAPS
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
				glCopyTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 0, 0, XRES, YRES, 0);
				((PFNGLGENERATEMIPMAPPROC)wglGetProcAddress("glGenerateMipmap"))(GL_TEXTURE_2D);
			#else
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
				glCopyTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 0, 0, XRES, YRES, 0);
			#endif
			((PFNGLACTIVETEXTUREPROC)wglGetProcAddress("glActiveTexture"))(GL_TEXTURE0);
			((PFNGLUSEPROGRAMPROC)wglGetProcAddress("glUseProgram"))(pi2);
			((PFNGLUNIFORM1IPROC)wglGetProcAddress("glUniform1i"))(0, 0);
			glRects(-1, -1, 1, 1);
		#endif

		SwapBuffers(hDC);

		// pausing and seeking enabled in debug mode
		#ifdef EDITOR_CONTROLS
			if(GetAsyncKeyState(VK_MENU))
			{
				double seek = 0.0;
				if(GetAsyncKeyState(VK_DOWN)) track.pause();
				if(GetAsyncKeyState(VK_UP))   track.play();
				if(GetAsyncKeyState(VK_RIGHT) && !GetAsyncKeyState(VK_SHIFT)) seek += 1.0;
				if(GetAsyncKeyState(VK_LEFT)  && !GetAsyncKeyState(VK_SHIFT)) seek -= 1.0;
				if(GetAsyncKeyState(VK_RIGHT) && GetAsyncKeyState(VK_SHIFT))  seek += 0.1;
				if(GetAsyncKeyState(VK_LEFT)  && GetAsyncKeyState(VK_SHIFT))  seek -= 0.1;
				if(position+seek != position)
				{
					position += seek;
					track.seek(position);
				}
			}
		#endif
	} while(!GetAsyncKeyState(VK_ESCAPE) && (MMTime.u.sample / SAMPLE_RATE) <= 42.0
		#if USE_AUDIO
			&& MMTime.u.sample < MAX_SAMPLES
		#endif
	);

	#if USE_INSTANSSI_LIGHTS
		closesocket(s);
		WSACleanup();
	#endif

	ExitProcess(0);
}