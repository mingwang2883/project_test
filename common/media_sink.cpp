#include <xlog.h>

#include "media_sink.h"
#include "raw_parser.h"

namespace flvpusher {

MediaSink::MediaSink() :
  m_vparser(new VideoRawParser),
  m_aparser(new AudioRawParser)
{

}

MediaSink::~MediaSink()
{
  SAFE_DELETE(m_vparser);
  SAFE_DELETE(m_aparser);
}

}
