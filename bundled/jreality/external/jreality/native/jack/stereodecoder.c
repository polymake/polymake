/**
 * Quick and dirty stereo decoder for first-order Ambisonics.
 **/

#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include <jack/jack.h>

#define INPORTS 4
#define OUTPORTS 2

jack_port_t* input_ports[INPORTS];
jack_port_t* output_ports[OUTPORTS];

int process(jack_nframes_t nframes, void *arg) {
  jack_default_audio_sample_t *outl = (jack_default_audio_sample_t *)
        jack_port_get_buffer(output_ports[0], nframes);
  jack_default_audio_sample_t *outr = (jack_default_audio_sample_t *)
        jack_port_get_buffer(output_ports[1], nframes);
  jack_default_audio_sample_t *inw = (jack_default_audio_sample_t *)
        jack_port_get_buffer(input_ports[0], nframes);
  jack_default_audio_sample_t *iny = (jack_default_audio_sample_t *)
        jack_port_get_buffer(input_ports[2], nframes);

  while (nframes--) {
    *outl++ = *inw + *iny;
    *outr++ = *inw++ - *iny++;
  }
  
  return 0;      
}

void jack_shutdown(void *arg) {
  exit(1);
}

int main(int argc, char *argv[]) {
  jack_client_t *client;
  const char **ports;
  const char* name = (argc>1) ? argv[1] : "StereoDecoder";
  char s[63];
  int i;

  if ((client = jack_client_open(name, JackNullOption, NULL)) == 0) {
    fprintf(stderr, "jack server not running?\n");
    return 1;
  }

  jack_set_process_callback(client, process, 0);
  jack_on_shutdown(client, jack_shutdown, 0);

  for(i=0; i<INPORTS; i++) {
    sprintf(s, "input_%d", i);
    input_ports[i] = jack_port_register(client, s,
          JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput, 0);
  }

  for(i=0; i<OUTPORTS; i++) {
    sprintf(s, "output_%d", i);
    output_ports[i] = jack_port_register(client, s,
          JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);
  }

  if (jack_activate(client)) {
    fprintf(stderr, "cannot activate client");
    return 1;
  }

  if ((ports = jack_get_ports(client, NULL, NULL, JackPortIsPhysical|JackPortIsInput)) == NULL) {
    fprintf(stderr, "Cannot find any physical playback ports\n");
    return 1;
  }

  for(i=0; i<OUTPORTS; i++) {
    if (ports[i]==NULL ||
          jack_connect(client, jack_port_name(output_ports[i]), ports[i])) {
      fprintf(stderr, "cannot connect output ports\n");
    }
  }

  free(ports);

  while (1) {
    sleep(10);
  }
}
