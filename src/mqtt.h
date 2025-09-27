#ifndef MQTT_H
#define MQTT_H

#include <stdio.h>

#define MQTT_HEADER_LEN 2
#define MQTT_ACK_LEN 4

#define CONNACK_BYTE 0x20
#define PUBLISH_BYTE 0x30
#define PUBACK_BYTE 0x40
#define PUBREC_BYTE 0x50
#define PUBREL_BYTE 0x62
#define PUBCOMP_BYTE 0x70
#define SUBACK_BYTE 0x90
#define UNSUBACK_BYTE 0xB0
#define PINGRESP_BYTE 0xD0

enum packet_type {
    CONNECT = 1,
    CONNACK,
    PUBLISH,
    PUBACK,
    PUBREC,
    PUBREL,
    PUBCOMP,
    SUBSCRIBE,
    SUBACK,
    UNSUBSCRIBE,
    UNSUBACK,
    PINGREQ,
    PINGRESP,
    DISCONNECT
};

enum qos_level {
    AT_MOST_ONCE,
    AT_LEAST_ONCE,
    EXACTLY_ONCE
};

union mqtt_header {
    unsigned char byte;
    struct {
        unsigned char retain : 1;
        unsigned char qos : 2;
        unsigned char dup : 1;
        unsigned char type : 4;
    } bits;
};


struct mqtt_connect {
  union mqtt_header header;
  union {
    unsigned char byte;
    struct {
      int      reserved       : 1;
      unsigned clean_session  : 1;
      unsigned will           : 1;
      unsigned will_qos       : 2;
      unsigned will_retain    : 1;
      unsigned password       : 1;
      unsigned username       : 1;
    } bits;
  };
  struct {
    unsigned short  keepalive;
    unsigned char   *client_id;
    unsigned char   *username;
    unsigned char   *password;
    unsigned char   *will_topic;
    unsigned char   *will_message;
  } payload;
  };

struct mqtt_connack {
  union mqtt_header header;
  union {
    unsigned char byte;
    struct {
      unsigned session_present : 1;
      unsigned reserved : 7;
    } bits;
  };
  unsigned char rc;
};

struct mqtt_subscribe {
  union mqtt_header header;
  unsigned short pkt_id;
  unsigned short tuples_len;
  struct {
    unsigned short topic_len;
    unsigned char *topic;
    unsigned qos;
  } *tuples;
};

struct mqtt_unsubscribe {
  union mqtt_header header;
  unsigned short pkt_id;
  unsigned short tuples_len;
  struct {
    unsigned short topic_len;
    unsigned char *topic;
  } *tuples;
};

struct mqtt_suback {
    union mqtt_header header;
    unsigned short pkt_id;
    unsigned short rcslen;
    unsigned char *rcs;
};

struct mqtt_publish {
    union mqtt_header header;
    unsigned short pkt_id;
    unsigned short topic_len;
    unsigned char *topic;
    unsigned short payloadlen;

    unsigned char *payload;
};

struct mqtt_ack {
    union mqtt_header header;
    unsigned short pkt_id;
};

// The remaining ACK Packets can be obtained by typedef'ing struct ack as 
// demonstrated below : 
typedef struct mqtt_ack mqtt_puback;
typedef struct mqtt_ack mqtt_pubrec;
typedef struct mqtt_ack mqtt_pubrel;
typedef struct mqtt_ack mqtt_pubcomp;
typedef struct mqtt_ack mqtt_unsuback;
typedef union mqtt_header mqtt_pingreq;
typedef union mqtt_header mqtt_pingresp;
typedef union mqtt_header mqtt_disconnect;

union mqtt_packet {
  struct  mqtt_ack          ack;
  union   mqtt_header       header;
  struct  mqtt_connect      connect;
  struct  mqtt_connack      connack;
  struct  mqtt_suback       suback;
  struct  mqtt_publish      publish;
  struct  mqtt_subscribe    subscribe;
  struct  mqtt_unsubscribe  unsubscribe;
};


// now that we are done with the definitions of the packets , 
// we'll need to work now on 4 functions :
// (for the interaction between client and server)
// CLIENT <-------------------> SERVER
// A packing function
// An unpacking function
// supported by two other functions:
// An encoding function
// A decoding fuction

union mqtt_header *mqtt_packet_header(unsigned char);
struct mqtt_ack *mqtt_packet_ack(unsigned char , unsigned short);
struct mqtt_connack *mqtt_packet_connack(unsigned char, unsigned char,
                                         unsigned char);
struct mqtt_suback *mqtt_packet_suback(unsigned char, unsigned short,
                                       unsigned char *, unsigned short);
struct mqtt_publish *mqtt_packet_publish(unsigned char, unsigned short, size_t,
                                         unsigned char *, size_t, unsigned char *);
void mqtt_packet_release(union mqtt_packet *, unsigned);

#endif
