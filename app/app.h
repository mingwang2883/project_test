#ifndef _APP_H_
#define _APP_H_


#include "xutil.h"
#include "xconfig.h"
#include "config.h"

#define DEFAULT_CFG_FILE                "./flvpusher_cfg.txt"

using xutil::status_t;

namespace flvpusher {

class MediaSink;
class MediaPusher;
class RtspSink;

class App {
public:
  ~App();
  App();

  int init(char *server_addr);
  int rtsp_push_server(int32_t timestamp, byte *data, uint32_t length,int is_video);
  void cleanup();

private:
  int load_cfg(std::string cfg_file = DEFAULT_CFG_FILE);
  void usage() const;

  int prepare();

private:
  static xutil::RecursiveMutex mutex;

  std::string m_liveurl;

  MediaSink *m_sink;
};

}

#endif /* end of _APP_H_ */
