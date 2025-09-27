#include <stdlib.h>
#include <string.h>
#include "mqtt.h"
#include "pack.h"
static size_t unpack_mqtt_connect(const unsigned char *, union mqtt_header *, union mqtt_packet *);
static size_t unpack_mqtt_publish(const unsigned char *, union mqtt_header *, union mqtt_packet *);
static size_t unpack_mqtt_subscribe(const unsigned char *, union mqtt_header *, union mqtt_packet *);
static size_t unpack_mqtt_unsubscribe(const unsigned char *, union mqtt_header *, union mqtt_packet *);
static size_t unpack_mqtt_ack(const unsigned char *, union mqtt_header *, union mqtt_packet *);
static unsigned char *pack_mqtt_header(const union mqtt_header *);
static unsigned char *pack_mqtt_ack(const union mqtt_packet *);

static unsigned char *pack_mqtt_connack(const union mqtt_packet *);
static unsigned char *pack_mqtt_suback(const union mqtt_packet *);
static unsigned char *pack_mqtt_publish(const union mqtt_packet *);

/*
 * MQTT v3.1.1 standard, Remaining length field on the fixed header can be at
 * most 4 bytes.
 */
static const int MAX_LEN_BYTES = 4;

/*
 * Encode Remaining Length on a MQTT packet header, comprised of Variable
 * Header and Payload if present. It does not take into account the bytes
 * required to store itself. Refer to MQTT v3.1.1 algorithm for the

 * implementation.
 */
int mqtt_encode_length(unsigned char *buf, size_t len) {
    int bytes = 0;
    do {
        if (bytes + 1 > MAX_LEN_BYTES)
            return bytes;
        short d = len % 128;
        len /= 128;
        /* if there are more digits to encode, set the top bit of this digit */
        if (len > 0)
            d |= 128;
        buf[bytes++] = d;
    } while (len > 0);
    return bytes;
}

/*
 * Decode Remaining Length comprised of Variable Header and Payload if
 * present. It does not take into account the bytes for storing length. Refer
 * to MQTT v3.1.1 algorithm for the implementation suggestion.
 *
 * TODO Handle case where multiplier > 128 * 128 * 128
 */
unsigned long long mqtt_decode_length(const unsigned char **buf) {
    char c;
    int multiplier = 1;
    unsigned long long value = 0LL;
    do {
        c = **buf;
        value += (c & 127) * multiplier;
        multiplier *= 128;
        (*buf)++;
    } while ((c & 128) != 0);
    return value;
}




/* NOTE:
 * MQTT unpacking functions
 */
/*
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
*/
/*
 |----------------------------------------------------------------|
 |   Bit    |  7  |  6  |  5  |  4  |  3  |  2  |  1  |   0       |  <-- Fixed Header
 |----------|-----------------------|-----------------------------|
 | Byte 1   |       0001 (CONNECT)  | 0 | 00 (QoS) | 0 (retain)   | 
 |          | 0x10                                                |
 |----------|-----------------------------------------------------|
 | Byte 2   | Remaining Length = 22+5+5+6 = 38 (example)          |
 |          | 0x26                                                |
 |----------|-----------------------------------------------------|  <-- Variable Header
 | Byte 3   | Protocol Name Length MSB = 0x00                     |
 | Byte 4   | Protocol Name Length LSB = 0x04                     |
 | Byte 5   | 'M' = 0x4D                                          |
 | Byte 6   | 'Q' = 0x51                                          |
 | Byte 7   | 'T' = 0x54                                          |
 | Byte 8   | 'T' = 0x54                                          |
 | Byte 9   | Protocol Level = 4                                  |
 | Byte 10  | Connect Flags = 0b11000010                          |
 |          | 7=username=1, 6=password=1, 5=will_retain=0         |
 |          | 4-3=will_QoS=00, 2=will=0, 1=clean_session=1        |
 | Byte 11  | Keepalive MSB = 0x00                                |
 | Byte 12  | Keepalive LSB = 0x3C (60 sec)                       |
 |----------|-----------------------------------------------------|  <-- Payload
 | Byte 13  | Client ID Length MSB = 0x00                         |
 | Byte 14  | Client ID Length LSB = 0x06                         |
 | Byte 15  | 'd' = 0x64                                          |
 | Byte 16  | 'a' = 0x61                                          |
 | Byte 17  | 'n' = 0x6E                                          |
 | Byte 18  | 'z' = 0x7A                                          |
 | Byte 19  | 'a' = 0x61                                          |
 | Byte 20  | 'n' = 0x6E                                          |
 | Byte 21  | Username Length MSB = 0x00                          |
 | Byte 22  | Username Length LSB = 0x05                          |
 | Byte 23  | 'h' = 0x68                                          |
 | Byte 24  | 'e' = 0x65                                          |
 | Byte 25  | 'l' = 0x6C                                          |
 | Byte 26  | 'l' = 0x6C                                          |
 | Byte 27  | 'o' = 0x6F                                          |
 | Byte 28  | Password Length MSB = 0x00                          |
 | Byte 29  | Password Length LSB = 0x05                          |
 | Byte 30  | 'n' = 0x6E                                          |
 | Byte 31  | 'a' = 0x61                                          |
 | Byte 32  | 'c' = 0x63                                          |
 | Byte 33  | 'h' = 0x68                                          |
 | Byte 34  | 'o' = 0x6F                                          |
 |__________|_____________________________________________________|
*/

static size_t unpack_mqtt_connect(const unsigned char *buf,
                                  union mqtt_header *hdr,
                                  union mqtt_packet *pkt) {
    struct mqtt_connect connect = { .header = *hdr };
    pkt->connect = connect;
    const unsigned char *init = buf;
    /* NOTE:
     * Second byte of the fixed header, contains the length of remaining bytes
     * of the connect packet
     */
    size_t len = mqtt_decode_length(&buf);

    /* NOTE:
     * For now we ignore checks on protocol name and reserved bits, just skip
     * to the 8th byte
     */
    buf = init + 8;
    /* Read variable header byte flags */
    pkt->connect.byte = unpack_u8((const uint8_t **) &buf);
    /* Read keepalive MSB and LSB (2 bytes word) */
    pkt->connect.payload.keepalive = unpack_u16((const uint8_t **) &buf);
    /* Read CID length (2 bytes word) */
    uint16_t cid_len = unpack_u16((const uint8_t **) &buf);

    /* Read the client id */
    if (cid_len > 0) {
        pkt->connect.payload.client_id = malloc(cid_len + 1);
        unpack_bytes((const uint8_t **) &buf, cid_len,
                     pkt->connect.payload.client_id);
    }
    /* Read the will topic and message if will is set on flags */
    if (pkt->connect.bits.will == 1) {
        unpack_string16(&buf, &pkt->connect.payload.will_topic);
        unpack_string16(&buf, &pkt->connect.payload.will_message);

    }
    /* Read the username if username flag is set */
    if (pkt->connect.bits.username == 1)
        unpack_string16(&buf, &pkt->connect.payload.username);

    /* Read the password if password flag is set */
    if (pkt->connect.bits.password == 1)
        unpack_string16(&buf, &pkt->connect.payload.password);
    return len;
}



/* mqtt pusblish struct 
struct mqtt_publish {
    union mqtt_header header;
    unsigned short pkt_id;
    unsigned short topic_len;
    unsigned char *topic;
    unsigned short payloadlen;

    unsigned char *payload;
};
*/
/*
 |----------------------------------------------------------------|
 |   Bit    |  7  |  6  |  5  |  4  |  3  |  2  |  1  |   0       |  <-- Fixed Header
 |----------|-----------------------|-----------------------------|
 | Byte 1   | MQTT Type = 3 (PUBLISH)| DUP=0 | QoS=01 | RETAIN=0  |
 |          | 0x32                                                |
 |----------|-----------------------------------------------------|
 | Byte 2   | Remaining Length = Variable Header + Payload = 17   |
 |          | 0x11                                                |
 |----------|-----------------------------------------------------|  <-- Variable Header
 | Byte 3   | Topic Length MSB = 0x00                             |
 | Byte 4   | Topic Length LSB = 0x0B (11 bytes)                  |
 | Byte 5   | 's' = 0x73                                          |
 | Byte 6   | 'e' = 0x65                                          |
 | Byte 7   | 'n' = 0x6E                                          |
 | Byte 8   | 's' = 0x73                                          |
 | Byte 9   | 'o' = 0x6F                                          |
 | Byte 10  | 'r' = 0x72                                          |
 | Byte 11  | '/' = 0x2F                                          |
 | Byte 12  | 't' = 0x74                                          |
 | Byte 13  | 'e' = 0x65                                          |
 | Byte 14  | 'm' = 0x6D                                          |
 | Byte 15  | 'p' = 0x70                                          |
 |----------|-----------------------------------------------------|
                                                  <-- Packet Identifier (for QoS 1 or 2)
 | Byte 16  | Packet Identifier MSB = 0x00                        |
 | Byte 17  | Packet Identifier LSB = 0x01                        |
 |----------|-----------------------------------------------------|  <-- Payload
 | Byte 18  | '2' = 0x32                                          |
 | Byte 19  | '5' = 0x35                                          |
 | Byte 20  | '.' = 0x2E                                          |
 | Byte 21  | '3' = 0x33                                          |
 | Byte 22  | 'C' = 0x43                                          |
 |----------------------------------------------------------------|
*/

static size_t unpack_mqtt_publish(const unsigned char *buf,
                                  union mqtt_header *hdr,
                                  union mqtt_packet *pkt) {
    struct mqtt_publish publish = { .header = *hdr };

    pkt->publish = publish;
    /* NOTE:
     * Second byte of the fixed header, contains the length of remaining bytes
     * of the connect packet
     */
    size_t len = mqtt_decode_length(&buf);
    /* Read topic length and topic of the soon-to-be-published message */
    pkt->publish.topic_len = unpack_string16(&buf, &pkt->publish.topic);
    uint16_t message_len = len;
    /* Read packet id */
    if (publish.header.bits.qos > AT_MOST_ONCE) {
        pkt->publish.pkt_id = unpack_u16((const uint8_t **) &buf);
        message_len -= sizeof(uint16_t);
    }
    /* NOTE:
     * Message len is calculated subtracting the length of the variable header
     * from the Remaining Length field that is in the Fixed Header
     */
    message_len -= (sizeof(uint16_t) + pkt->publish.topic_len);
    pkt->publish.payloadlen = message_len;
    pkt->publish.payload = malloc(message_len + 1);
    unpack_bytes((const uint8_t **) &buf, message_len, pkt->publish.payload);
    return len;
}



/*
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
*/

/*
 |-------------------------------------------------------------|
 |  Bit     |  7  |  6  |  5  |  4  |  3  |  2  |  1  |   0    |  <-- Fixed Header
 |----------|-----------------------|--------------------------|
 | Byte 1   |      MQTT type 8      |     Flags = 0010         |
 |          |   (SUBSCRIBE = 1000)  |   (required for SUB)     |
 |----------|--------------------------------------------------|
 | Byte 2   |                                                  |
 |   .      |               Remaining Length                   |
 |   .      |     (calculated below: 2 + topic1 + topic2)      |
 |----------|--------------------------------------------------|  <-- Variable Header
 | Byte 3   |         Packet Identifier MSB                    |
 | Byte 4   |         Packet Identifier LSB  (0x01)            |
 |----------|--------------------------------------------------|  <-- Payload
 | Byte 5   |                Topic1 Length MSB                 |
 | Byte 6   |                Topic1 Length LSB (19)            |
 | Byte 7   |                                                  |
 |   .      |         "sensor/temperature"                     |
 | Byte 25  |                                                  |
 | Byte 26  |             Requested QoS (00000001 = QoS 1)     |
 |----------|--------------------------------------------------|
 | Byte 27  |                Topic2 Length MSB                 |
 | Byte 28  |                Topic2 Length LSB (6)             |
 | Byte 29  |                                                  |
 |   .      |               "alerts"                           |
 | Byte 34  |                                                  |
 | Byte 35  |             Requested QoS (00000000 = QoS 0)     |
 |----------|--------------------------------------------------|
*/

static size_t unpack_mqtt_subscribe(const unsigned char *buf,
                                    union mqtt_header *hdr,
                                    union mqtt_packet *pkt) {

    struct mqtt_subscribe subscribe = { .header = *hdr };
    /* NOTE:
     * Second byte of the fixed header, contains the length of remaining bytes
     * of the connect packet
     */
    size_t len = mqtt_decode_length(&buf);
    size_t remaining_bytes = len;
    /* Read packet id */
    subscribe.pkt_id = unpack_u16((const uint8_t **) &buf);
    remaining_bytes -= sizeof(uint16_t);
    /* NOTE:
     * Read in a loop all remaining bytes specified by len of the Fixed Header.
     * From now on the payload consists of 3-tuples formed by:
     *  - topic length
     *  - topic filter (string)
     *  - qos
     */
    int i = 0;
    while (remaining_bytes > 0) {
        /* Read length bytes of the first topic filter */
        remaining_bytes -= sizeof(uint16_t);
        /* We have to make room for additional incoming tuples */
        subscribe.tuples = realloc(subscribe.tuples,
                                   (i+1) * sizeof(*subscribe.tuples));
        // it might seem weird but the function unpack_string16 takes the topic_len in 
        // unpacks it , mallocs space for that topic string and unpacks it and then 
        // returns the topic_len
        subscribe.tuples[i].topic_len =
            unpack_string16(&buf, &subscribe.tuples[i].topic);
        remaining_bytes -= subscribe.tuples[i].topic_len;
        subscribe.tuples[i].qos = unpack_u8((const uint8_t **) &buf);
        remaining_bytes -= sizeof(uint8_t); // not len
        i++;
    }
    subscribe.tuples_len = i;
    pkt->subscribe = subscribe;
    return len;
}

/*
struct mqtt_unsubscribe {
  union mqtt_header header;
  unsigned short pkt_id;
  unsigned short tuples_len;
  struct {
    unsigned short topic_len;
    unsigned char *topic;
  } *tuples;
};

 */
/*
 |-------------------------------------------------------------|
 |  Bit    |  7  |  6  |  5  |  4  |  3  |  2  |  1  |   0     |  <-- Fixed Header
 |----------|-----------------------|--------------------------|
 | Byte 1   |      MQTT type 10     |     Flags = 0010         |
 |          |  (UNSUBSCRIBE = 1010) |   (must be 0010 for QoS1)|
 |----------|--------------------------------------------------|
 | Byte 2   |         Remaining Length = 18                    |
 |----------|--------------------------------------------------|  <-- Variable Header
 | Byte 3   |         Packet Identifier MSB = 0x00             |
 | Byte 4   |         Packet Identifier LSB = 0x10             |
 |----------|--------------------------------------------------|  <-- Payload
 | Byte 5   |         Topic1 Length MSB = 0x00                 |
 | Byte 6   |         Topic1 Length LSB = 0x0C (12)            |
 | Byte 7   |                                                  |
 |   .      |         "chat/general"                           |
 | Byte 18  |                                                  |
 | Byte 19  |             Topic2 Length MSB = 0x00             |
 | Byte 20  |             Topic2 Length LSB = 0x06 (6)         |
 | Byte 21  |                                                  |
 |   .      |         "alerts"                                 |
 | Byte 26  |                                                  |
 |-------------------------------------------------------------|
*/


static size_t unpack_mqtt_unsubscribe(const unsigned char *buf,
                                      union mqtt_header *hdr,
                                      union mqtt_packet *pkt) {

  struct mqtt_unsubscribe unsubscribe = { .header = *hdr};
    /* NOTE:
     * Second byte of the fixed header, contains the length of remaining bytes
     * of the connect packet
     */

  size_t len = mqtt_decode_length(&buf);
  size_t remaining_bytes = len;

  unsubscribe.pkt_id = unpack_u16((const uint8_t **) &buf);
  remaining_bytes -= sizeof(uint16_t);
  /* NOTE:
     * Read in a loop all remaining bytes specified by len of the Fixed Header.
     * From now on the payload consists of 3-tuples formed by:
     *  - topic length
     *  - topic filter (string)
     *  - qos
  */

  int i = 0;
  while (remaining_bytes > 0){

    /* Read length bytes of the first topic filter */
    remaining_bytes -= sizeof(uint16_t);

    /* We have to make room for additional incoming tuples */
    unsubscribe.tuples = realloc(unsubscribe.tuples,
                                 (i+1) * sizeof(*unsubscribe.tuples));

    unsubscribe.tuples[i].topic_len = unpack_string16(&buf
                                                      , &unsubscribe.tuples[i].topic);
    remaining_bytes -= unsubscribe.tuples[i].topic_len;
    i++;
  }

  unsubscribe.tuples_len = i;
  pkt->unsubscribe = unsubscribe;
  return len;
}

/*
struct mqtt_ack {
    union mqtt_header header;
    unsigned short pkt_id;
};

typedef struct mqtt_ack mqtt_puback;
typedef struct mqtt_ack mqtt_pubrec;
typedef struct mqtt_ack mqtt_pubrel;
typedef struct mqtt_ack mqtt_pubcomp;
typedef struct mqtt_ack mqtt_unsuback;
typedef union mqtt_header mqtt_pingreq;
typedef union mqtt_header mqtt_pingresp;
typedef union mqtt_header mqtt_disconnect;
*/

/*
 |-------------------------------------------------------------|
 |  Bit     |  7  |  6  |  5  |  4  |  3  |  2  |  1  |   0    |  <-- Fixed Header
 |----------|-----------------------|--------------------------|
 | Byte 1   |      MQTT type 11     |     Flags = 0000         |
 |          |   (UNSUBACK = 1011)   |     (all flags 0)        |
 |----------|--------------------------------------------------|
 | Byte 2   |               Remaining Length = 2               |
 |----------|--------------------------------------------------|  <-- Variable Header
 | Byte 3   |         Packet Identifier MSB = 0x00             |
 | Byte 4   |         Packet Identifier LSB = 0x10             |
 |----------|--------------------------------------------------|
*/

static size_t unpack_mqtt_ack(const unsigned char *buf,
                                      union mqtt_header *hdr,
                                      union mqtt_packet *pkt) {

  struct mqtt_ack ack = { .header = *hdr };
  /* NOTE:
     * Second byte of the fixed header, contains the length of remaining bytes
     * of the connect packet
  */
  size_t len = mqtt_decode_length(&buf);
  ack.pkt_id = unpack_u16((const uint8_t **) &buf);
  pkt->ack = ack;
  return len;
}


typedef size_t mqtt_unpack_handler(const unsigned char *,
                                   union mqtt_header *,
                                   union mqtt_packet *);

/* NOTE:
 * Unpack functions mapping unpacking_handlers positioned in the array based
 * on message type
 */

/*
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
*/
static mqtt_unpack_handler *unpacking_handlers[] = {
    NULL,                   // 0
    unpack_mqtt_connect,    // 1 CONNECT
    NULL,                   // 2 CONNACK
    unpack_mqtt_publish,    // 3 PUBLISH
    unpack_mqtt_ack,        // 4 PUBACK
    unpack_mqtt_ack,        // 5 PUBREC
    unpack_mqtt_ack,        // 6 PUBREL
    unpack_mqtt_ack,        // 7 PUBCOMP
    unpack_mqtt_subscribe,  // 8 SUBSCRIBE
    NULL,                   // 9 SUBACK
    unpack_mqtt_unsubscribe,// 10 UNSUBSCRIBE
    unpack_mqtt_ack,        // 11 UNSUBACK
    NULL,                   // 12 PINGREQ
    NULL,                   // 13 PINGRESP
    NULL                    // 14 DISCONNECT
};

int unpack_mqtt_packet(const unsigned char *buf,
                       union mqtt_packet *pkt) {
  int rc = 0;

  /* Read first byte of the fixed header */
  unsigned char type = *buf;
  union mqtt_header header = {
    .byte = type
  };
  if (header.bits.type == DISCONNECT
    || header.bits.type == PINGREQ 
    || header.bits.type == PINGRESP){
    pkt->header = header;
  }else{
    /* Call the appropriate unpack handler based on the message type */
    rc = unpacking_handlers[header.bits.type](++buf, &header, pkt);
  }
  return rc;
}


