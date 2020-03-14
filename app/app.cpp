#include <libgen.h>
#include <memory>
#include <vector>
#include <cstdlib>
#include <getopt.h>
#include <xlog.h>
#include <xuri.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#include "app.h"
#include "config.h"
#include "rtsp_source.h"
#include "rtsp_sink.h"

using namespace xutil;
using namespace xconfig;
using namespace xuri;

namespace flvpusher {

//App *App::app = NULL;
RecursiveMutex App::mutex;

App::App() :
  m_sink(NULL)
{

}

App::~App()
{
  cleanup();
}

int get_ip_from_dns(char *dns, char *ip)
{
    char *ptr,**pptr;
    struct hostent *hptr;
    char str[32];
    /* 取得命令后第一个参数，即要解析的域名或主机名 */
    ptr = dns;
    /* 调用gethostbyname()。调用结果都存在hptr中 */
    if ((hptr = gethostbyname(ptr)) == NULL)
    {
        printf("gethostbyname error for host:%s\n", ptr);
        return -1; /* 如果调用gethostbyname发生错误，返回1 */
    }
    /* 将主机的规范名打出来 */
//    printf("official hostname:%s\n",hptr->h_name);
    /* 主机可能有多个别名，将所有别名分别打出来 */
    //for (pptr = hptr->h_aliases; *pptr != NULL; pptr++)
    //    printf(" alias:%s\n",*pptr);
    /* 根据地址类型，将地址打出来 */
    switch(hptr->h_addrtype)
    {
        case AF_INET:
        case AF_INET6:
            pptr=hptr->h_addr_list;
            /* 将刚才得到的所有地址都打出来。其中调用了inet_ntop()函数 */
            //for(;*pptr!=NULL;pptr++)
            sprintf (ip, "%s", inet_ntop(hptr->h_addrtype, *pptr, str, sizeof(str)));
//            printf(" address:%s !\n", ip);
            return 0;
        default:
            printf("unknown address type\n");
            break;
    }
    return -1;
}

int App::init(char *server_addr)
{
  char ip_addr[20] = {0};
  char dns_name[48] = {0};
  char sdp_name[48] = {0};
  char rtsp_server[48] = {0};

  if (load_cfg() < 0) {
    printf("load_cfg() < 0 \n");
    return -1;
  }

  sscanf(server_addr,"%*[^/]//%[^:]",dns_name);
  sscanf(server_addr,"%*[^/]//%*[^:]:%s",sdp_name);
  int ret = get_ip_from_dns(dns_name,ip_addr);
  if (ret != 0)
  {
      LOGE("get_ip_from_dns error\n");
      return -1;
  }
  sprintf(rtsp_server,"rtsp://%s:%s",ip_addr,sdp_name);
  printf("rtsp_server:%s\n",rtsp_server);
  m_liveurl = rtsp_server;

  if (prepare() < 0) {
    return -1;
  }

  return 0;
}

void App::cleanup()
{
  SAFE_DELETE(m_sink);
}

int App::load_cfg(std::string cfg_file)
{
  if (!is_path_absolute(STR(cfg_file))) {
    cfg_file = sprintf_("%s%c%s",
                        STR(dirname_(abs_program)), DIRSEP, STR(cfg_file));
  }
  if (!is_file(cfg_file)) {
    // It's ok if config file is missing
    return 0;
  }

  return 0;
}

int App::prepare()
{
    if (!m_liveurl.empty()) {
      if (start_with(m_liveurl, "rtsp://")) {
        if (!end_with(m_liveurl, ".sdp")) {
          LOGE("Invalid rtsp live url: \"%s\"\n", STR(m_liveurl));
          return -1;
      }
      m_sink = new RtspSink();
    }
    if (m_sink->connect(m_liveurl) < 0)
    {
      printf("connect %s error \n",STR(m_liveurl));
      return -1;
    }
  }
//  LOGE("APP prepare\n");

  return 0;
}

int App::rtsp_push_server(int32_t timestamp, byte *data, uint32_t length,int is_video)
{

//  printf("rtsp_push_server\n");
    if (is_video == 1) {
          m_sink->send_video(timestamp,data,length);
    } else {
          m_sink->send_audio(timestamp,data,length);
    }

  return 0;
}

}
