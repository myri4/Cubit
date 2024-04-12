#pragma once
#include <miniaudio.h>
#include <wc/Utils/Log.h>

namespace wc 
{
	struct AudioDevice
	{
		void Create(const ma_device_config& config)
		{
			if (ma_device_init(nullptr, &config, &m_Handle) != MA_SUCCESS) 
			{
				WC_CORE_ERROR("Failed to initialize device");
				return;
			}
		}

		void Destroy() { ma_device_uninit(&m_Handle); }

		void Start() { ma_device_start(&m_Handle); }
	private:
		ma_device m_Handle = {};
	};

	struct Sound
	{
		Sound() = default;
		Sound(ma_sound instance) { m_Handle = instance; }

		void Start() { ma_sound_start(&m_Handle); }
		void Stop() { ma_sound_stop(&m_Handle); }

		void SetVolume(float volume) { ma_sound_set_volume(&m_Handle, volume); }
		float GetVolume() { ma_sound_get_volume(&m_Handle); }

		void SetPan(float pan) { ma_sound_set_pan(&m_Handle, pan); }
		float GetPan() { ma_sound_get_pan(&m_Handle); }

		void SetPitch(float pitch) { ma_sound_set_pitch(&m_Handle, pitch); }
		float GetPitch() { ma_sound_get_pitch(&m_Handle); }

		void SeekFrame(uint64_t frame) { ma_sound_seek_to_pcm_frame(&m_Handle, frame); }

		void SetStartFrameTime(uint64_t frame) { ma_sound_set_start_time_in_pcm_frames(&m_Handle, frame); }
		void SetStopFrameTime(uint64_t frame) { ma_sound_set_stop_time_in_pcm_frames(&m_Handle, frame); }

		void SetStartTime(uint64_t milisecond) { ma_sound_set_start_time_in_milliseconds(&m_Handle, milisecond); }
		void SetStopTime(uint64_t milisecond) { ma_sound_set_stop_time_in_milliseconds(&m_Handle, milisecond); }

		void SetFade(float volBeg, float volEnd, uint64_t frameLength) { ma_sound_set_fade_in_pcm_frames(&m_Handle, volBeg, volEnd, frameLength); }

		void SetLooping(bool loop) { ma_sound_set_looping(&m_Handle, loop); }

		bool Playing() { return ma_sound_is_playing(&m_Handle); }
		bool AtEnd() { return ma_sound_at_end(&m_Handle); }
		bool Looping() { return ma_sound_is_looping(&m_Handle); }
	private:
		ma_sound m_Handle = {};
	};

	struct AudioEngine
	{
		void Create()
		{
			if (ma_engine_init(nullptr, &m_Engine) != MA_SUCCESS)
			{
				WC_CORE_ERROR("Failed to initialize audio engine.");
				return;
			}
		}

		Sound CreateSound(const std::string& location, uint32_t flags = 0) 
		{
			ma_result result;
			ma_sound sound;

			result = ma_sound_init_from_file(&m_Engine, location.c_str(), flags, nullptr, nullptr, &sound);
			if (result != MA_SUCCESS) 
			{
				WC_CORE_ERROR("Could not find sound at location {}", location.c_str());
				return Sound();
			}

			return Sound(sound);
		}

		void Play(const std::string& path)
		{
			ma_engine_play_sound(&m_Engine, path.c_str(), NULL);
		}

		//void GetFrame() { ma_engine_set_time_in_pcm_frames(&m_Engine); }

		void Destroy() { ma_engine_uninit(&m_Engine); }

	private:
		ma_engine m_Engine = {};
	};
}