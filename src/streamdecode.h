#ifndef STREAMDECODE_H_
#define STREAMDECODE_H_

#include <stddef.h>

// consider supplying temporal context to the decoded character
// status ==  0 for a character ; data is character
// status == -1 for an error ; data is error code
typedef int streamdecode_callback(void *userdata, int status, int data);

struct stream_state;
struct audio_state;

enum {
    STREAM_ERR_OK = 0,

    STREAM_ERR_PARITY,

    STREAM_ERR_max
};

int streamdecode_init(struct stream_state **sp, struct audio_state *as, void *ud, streamdecode_callback *cb, int channel);
int streamdecode_process(struct stream_state *s, size_t count, double samples[count]);
void streamdecode_fini(struct stream_state *s);

#endif

