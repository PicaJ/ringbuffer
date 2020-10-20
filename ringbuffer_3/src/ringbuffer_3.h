#include <stdint.h>
#include <pthread.h>

#define MAX_VENCODER_PORTS (2)

#define ENC_FIFO_LEVEL  (32)

#define COMPRESSED_SRC_ENC_BUF_LEN      (10*1024*1024)
#define FRAME_BEGIN_FLAG        0x55AA55AA

#define BITSTREAM_FRAME_SIZE    (256)   // ref to BITSTREAM_FRAME_FIFO_SIZE

#define ARRAY_SIZE(a) (sizeof(a) / sizeof(a[0]))


/* invlalid channel ID */
#define ERR_VENC_INVALID_CHNID     1
/* at lease one parameter is illagal ,eg, an illegal enumeration value  */
#define ERR_VENC_ILLEGAL_PARAM     2
/* channel exists */
#define ERR_VENC_EXIST             3
/* channel exists */
#define ERR_VENC_UNEXIST           4
/* using a NULL point */
#define ERR_VENC_NULL_PTR          5
/* try to enable or initialize system,device or channel, before configing attribute */
#define ERR_VENC_NOT_CONFIG        6
/* operation is not supported by NOW */
#define ERR_VENC_NOT_SUPPORT       7
/* operation is not permitted ,eg, try to change stati attribute */
#define ERR_VENC_NOT_PERM          8
/* failure caused by malloc memory */
#define ERR_VENC_NOMEM             9
/* failure caused by malloc buffer */
#define ERR_VENC_NOBUF             10
/* no data in buffer */
#define ERR_VENC_BUF_EMPTY         11
/* no buffer for new data */
#define ERR_VENC_BUF_FULL          12
/* system is not ready,had not initialed or loaded*/
#define ERR_VENC_SYS_NOTREADY      13
/* system is busy*/
#define ERR_VENC_BUSY              14
/* encode timeout */
#define ERR_VENC_TIMEOUT           15
/* component state is same as user wanted */
#define ERR_VENC_SAMESTATE         16
/* component state is transit to invalid state */
#define ERR_VENC_INVALIDSTATE      17
/* component current state can't transit to destination state */
#define ERR_VENC_INCORRECT_STATE_TRANSITION 18
/* Attempting a command that is not allowed during the present state. */
#define ERR_VENC_INCORRECT_STATE_OPERATION 19


typedef enum COMP_RECORD_TYPE
{
    COMP_RECORD_TYPE_NORMAL = 0,
    COMP_RECORD_TYPE_TIMELAPSE, //selected encoded frame interval is long(0.1fps), but play as camera capture frame rate. camera capture in normal frame rate, but select only few to encode.
    COMP_RECORD_TYPE_SLOW,  //encode every input frame, but reset its pts to play as another frame rate. e.g.,frame interval is small(120fps), but play as 30fps.
} COMP_RECORD_TYPE;

typedef struct FRAMEINFOTYPE {
    int64_t     timeStamp;
    int         bufferId;
    int         size;
} FRAMEINFOTYPE;

typedef struct FRAMEDATATYPE {
    FRAMEINFOTYPE   info;
    char*           addrY;
} FRAMEDATATYPE;

typedef struct VIDEOENCBUFFERMANAGER {
    unsigned char           *buffer;
    unsigned int            writePos;
    unsigned int            readPos;
    unsigned int         prefetchPos;
    int         count;
    int         mUnprefetchFrameNum;
    pthread_mutex_t lock;
} VIDEOENCBUFFERMANAGER;

VIDEOENCBUFFERMANAGER *VideoEncBufferInit(void);
void VideoEncBufferDeInit(VIDEOENCBUFFERMANAGER *manager);
int VideoEncBufferPushFrame(VIDEOENCBUFFERMANAGER *manager, FRAMEDATATYPE *frame);
int VideoEncBufferGetFrame(VIDEOENCBUFFERMANAGER *manager, FRAMEDATATYPE *frame);
int VideoEncBufferReleaseFrame(VIDEOENCBUFFERMANAGER *manager, FRAMEDATATYPE *releaseFrame);
