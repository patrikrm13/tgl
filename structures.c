/* 
    This file is part of tgl-library
    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.
    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.
    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
    Copyright Vitaly Valtman 2013-2015
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <assert.h>
#include <string.h>
#include <strings.h>
#include "tgl-structures.h"
#include "mtproto-common.h"
//#include "telegram.h"
#include "tree.h"
#include <openssl/aes.h>
#include <openssl/bn.h>
#include <openssl/sha.h>
#include "queries.h"
#include "tgl-binlog.h"
#include "tgl-methods-in.h"
#include "updates.h"
#include "mtproto-client.h"

#include "tgl.h"
#include "auto.h"
#include "auto/auto-types.h"
#include "auto/auto-skip.h"
#include "auto/auto-fetch-ds.h"
#include "auto/auto-free-ds.h"

#define sha1 SHA1

struct random2local {
  long long random_id;
  int local_id;
};

static int id_cmp (struct tgl_message *M1, struct tgl_message *M2);
#define peer_cmp(a,b) (tgl_cmp_peer_id (a->id, b->id))
#define peer_cmp_name(a,b) (strcmp (a->print_name, b->print_name))

static int random_id_cmp (struct random2local *L, struct random2local *R) {
  if (L->random_id < R->random_id) { return -1; }
  if (L->random_id > R->random_id) { return 1; }
  return 0;
}

static int photo_id_cmp (struct tgl_photo *L, struct tgl_photo *R) {
  if (L->id < R->id) { return -1; }
  if (L->id > R->id) { return 1; }
  return 0;
}

static int document_id_cmp (struct tgl_document *L, struct tgl_document *R) {
  if (L->id < R->id) { return -1; }
  if (L->id > R->id) { return 1; }
  return 0;
}

static int webpage_id_cmp (struct tgl_webpage *L, struct tgl_webpage *R) {
  if (L->id < R->id) { return -1; }
  if (L->id > R->id) { return 1; }
  return 0;
}

DEFINE_TREE(peer,tgl_peer_t *,peer_cmp,0)
DEFINE_TREE(peer_by_name,tgl_peer_t *,peer_cmp_name,0)
DEFINE_TREE(message,struct tgl_message *,id_cmp,0)
DEFINE_TREE(random_id,struct random2local *, random_id_cmp,0)
DEFINE_TREE(photo,struct tgl_photo *,photo_id_cmp,0)
DEFINE_TREE(document,struct tgl_document *,document_id_cmp,0)
DEFINE_TREE(webpage,struct tgl_webpage *,webpage_id_cmp,0)


char *tgls_default_create_print_name (struct tgl_state *TLS, tgl_peer_id_t id, const char *a1, const char *a2, const char *a3, const char *a4) {
  const char *d[4];
  d[0] = a1; d[1] = a2; d[2] = a3; d[3] = a4;
  static char buf[10000];
  buf[0] = 0;
  int i;
  int p = 0;
  for (i = 0; i < 4; i++) {
    if (d[i] && strlen (d[i])) {
      p += tsnprintf (buf + p, 9999 - p, "%s%s", p ? "_" : "", d[i]);
      assert (p < 9990);
    }
  }
  char *s = buf;
  while (*s) {
    if (((unsigned char)*s) <= ' ') { *s = '_'; }
    if (*s == '#') { *s = '@'; }
    s++;
  }
  s = buf;
  int fl = strlen (s);
  int cc = 0;
  while (1) {
    tgl_peer_t *P = tgl_peer_get_by_name (TLS, s);
    if (!P || !tgl_cmp_peer_id (P->id, id)) {
      break;
    }
    cc ++;
    assert (cc <= 9999);
    tsnprintf (s + fl, 9999 - fl, "#%d", cc);
  }
  return tstrdup (s);
}

enum tgl_typing_status tglf_fetch_typing_new (struct tl_ds_send_message_action *DS_SMA) {
  if (!DS_SMA) { return 0; }
  switch (DS_SMA->magic) {
  case CODE_send_message_typing_action:
    return tgl_typing_typing;
  case CODE_send_message_cancel_action:
    return tgl_typing_cancel;
  case CODE_send_message_record_video_action:
    return tgl_typing_record_video;
  case CODE_send_message_upload_video_action:
    retu
