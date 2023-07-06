#include "media_pusher.h"
#include "media_sink.h"

using namespace xmedia;

namespace flvpusher {

MediaPusher::MediaPusher(const std::string &input, MediaSink *&sink) :
  m_input(input),
  m_sink(sink),
  m_itime_base((AVRational) {1001, 24000}),
  m_start_time(0)
{
}

MediaPusher::~MediaPusher()
{
  m_dvf.close();
  m_daf.close();
}

int MediaPusher::dump_video(const std::string &path, bool append)
{
  if (path.empty()) return 0;

  if (m_dvf.is_opened())
    m_dvf.close();

  return m_dvf.open(STR(path), append ? "ab+" : "wb+") ? 0 : -1;
}

int MediaPusher::dump_audio(const std::string &path, bool append)
{
  if (path.empty()) return 0;

  if (m_daf.is_opened())
    m_daf.close();

  return m_daf.open(STR(path), append ? "ab+" : "wb+") ? 0 : -1;
}

int MediaPusher::mux2ts(const std::string &tspath)
{
  m_tspath = tspath;
  return 0;
}

int MediaPusher::on_frame(const int32_t ts,
                          const byte *dat, const uint32_t dat_len, int is_video,
                          uint32_t composition_time)
{
  if (is_video && m_dvf.is_opened()) {
    if (!m_dvf.write_buffer(dat, dat_len))
      return -1;
  }

  if (!is_video && m_daf.is_opened()) {
    if (!m_daf.write_buffer(dat, dat_len))
      return -1;
  }

#if 0
  if (!m_tspath.empty()) {
    printf("!m_tspath.empty() \n");
    if (!m_tsmuxer.is_opened()) {
      AutoLock _l(m_mutex);
      if (!m_tsmuxer.is_opened()) {
        // itime_base for pcr generation
        if (m_tsmuxer.set_file(m_tspath, m_itime_base) < 0)
          return -1;
      }
    }

    if (m_tsmuxer.write_frame(ts, dat, dat_len, is_video,
                              composition_time) < 0)
      return -1;
  }
#endif

  return 0;
}

int MediaPusher::frame_wait_done(int *timestamp)
{
  if (m_start_time == 0) {
    m_start_time = get_time_now();
    m_start_timestamp = *timestamp;
  }

  *timestamp -= m_start_timestamp;

  while (!interrupt_cb()) {
    uint64_t now = get_time_now();
    if ((int) (now - m_start_time) >= *timestamp) {
      // frame wait done
      break;
    }
    // a short sleep then check again
    sleep_(10);
  }

  return interrupt_cb() ? -1 : 0;
}

}
