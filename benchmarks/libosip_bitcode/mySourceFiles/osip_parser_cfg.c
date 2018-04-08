/*
  The oSIP library implements the Session Initiation Protocol (SIP -rfc3261-)
  Copyright (C) 2001-2012 Aymeric MOIZARD amoizard@antisip.com
  
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
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include <osipparser2/internal.h>

#include <osipparser2/osip_port.h>
#include <osipparser2/osip_parser.h>
#include "parser.h"

static __osip_message_config_t pconfig[NUMBER_OF_HEADERS];

/* The size of the hash table seems large for a limited number of possible entries
 * The 'problem' is that the header name are too much alike for the osip_hash() function
 * which gives a poor deviation.
 * Anyway, this mechanism improves the search time (from binary seach (log(n)) to 1).
 */

#ifndef HASH_TABLE_SIZE
#ifdef __amd64__
#define HASH_TABLE_SIZE 150
#else
#define HASH_TABLE_SIZE 150     /* set this to the hash table size, 150 is the
                                   first size where no conflicts occur */
#endif

static int hdr_ref_table[HASH_TABLE_SIZE];      /* the hashtable contains indices to the pconfig table    */

/*
  list of compact header:
  i: Call-ID   => ok
  m: Contact   => ok
  e: Content-Encoding   => ok
  l: Content-Length   => ok
  c: Content-Type   => ok
  f: From   => ok
  s: Subject   => NOT A SUPPORTED HEADER! will be
                 available in the list of unknown headers
  t: To   => ok
  v: Via   => ok
*/
/* This method must be called before using the parser */
int
parser_init (void)
{
  int i = 0;

#ifndef MINISIZE
  pconfig[i].hname = ACCEPT;
  pconfig[i].ignored_when_invalid = 1;
  pconfig[i++].setheader = (&osip_message_set_accept);
  pconfig[i].hname = ACCEPT_ENCODING;
  pconfig[i].ignored_when_invalid = 1;
  pconfig[i++].setheader = (&osip_message_set_accept_encoding);
  pconfig[i].hname = ACCEPT_LANGUAGE;
  pconfig[i].ignored_when_invalid = 1;
  pconfig[i++].setheader = (&osip_message_set_accept_language);
  pconfig[i].hname = ALERT_INFO;
  pconfig[i].ignored_when_invalid = 1;
  pconfig[i++].setheader = (&osip_message_set_alert_info);
  pconfig[i].hname = ALLOW;
  pconfig[i].ignored_when_invalid = 1;
  pconfig[i++].setheader = (&osip_message_set_allow);
  pconfig[i].hname = AUTHENTICATION_INFO;
  pconfig[i].ignored_when_invalid = 1;
  pconfig[i++].setheader = (&osip_message_set_authentication_info);
#endif
  pconfig[i].hname = AUTHORIZATION;
  pconfig[i].ignored_when_invalid = 1;
  pconfig[i++].setheader = (&osip_message_set_authorization);
  pconfig[i].hname = CONTENT_TYPE_SHORT;        /* "l" */
  pconfig[i].ignored_when_invalid = 0;
  pconfig[i++].setheader = (&osip_message_set_content_type);
  pconfig[i].hname = CALL_ID;
  pconfig[i].ignored_when_invalid = 0;
  pconfig[i++].setheader = (&osip_message_set_call_id);
#ifndef MINISIZE
  pconfig[i].hname = CALL_INFO;
  pconfig[i].ignored_when_invalid = 1;
  pconfig[i++].setheader = (&osip_message_set_call_info);
#endif
  pconfig[i].hname = CONTACT;
  pconfig[i].ignored_when_invalid = 0;
  pconfig[i++].setheader = (&osip_message_set_contact);
#ifndef MINISIZE
  pconfig[i].hname = CONTENT_ENCODING;
  pconfig[i].ignored_when_invalid = 1;
  pconfig[i++].setheader = (&osip_message_set_content_encoding);
#endif
  pconfig[i].hname = CONTENT_LENGTH;
  pconfig[i].ignored_when_invalid = 0;
  pconfig[i++].setheader = (&osip_message_set_content_length);
  pconfig[i].hname = CONTENT_TYPE;
  pconfig[i].ignored_when_invalid = 0;
  pconfig[i++].setheader = (&osip_message_set_content_type);
  pconfig[i].hname = CSEQ;
  pconfig[i].ignored_when_invalid = 0;
  pconfig[i++].setheader = (&osip_message_set_cseq);
#ifndef MINISIZE
  pconfig[i].hname = CONTENT_ENCODING_SHORT;    /* "e" */
  pconfig[i].ignored_when_invalid = 1;
  pconfig[i++].setheader = (&osip_message_set_content_encoding);
  pconfig[i].hname = ERROR_INFO;
  pconfig[i].ignored_when_invalid = 1;
  pconfig[i++].setheader = (&osip_message_set_error_info);
#endif
  pconfig[i].hname = FROM_SHORT;        /* "f" */
  pconfig[i].ignored_when_invalid = 0;
  pconfig[i++].setheader = (&osip_message_set_from);
  pconfig[i].hname = FROM;
  pconfig[i].ignored_when_invalid = 0;
  pconfig[i++].setheader = (&osip_message_set_from);
  pconfig[i].hname = CALL_ID_SHORT;     /* "i" */
  pconfig[i].ignored_when_invalid = 0;
  pconfig[i++].setheader = (&osip_message_set_call_id);
  pconfig[i].hname = CONTENT_LENGTH_SHORT;      /* "l" */
  pconfig[i].ignored_when_invalid = 0;
  pconfig[i++].setheader = (&osip_message_set_content_length);
  pconfig[i].hname = CONTACT_SHORT;     /* "m" */
  pconfig[i].ignored_when_invalid = 0;
  pconfig[i++].setheader = (&osip_message_set_contact);
  pconfig[i].hname = MIME_VERSION;
  pconfig[i].ignored_when_invalid = 1;
  pconfig[i++].setheader = (&osip_message_set_mime_version);
  pconfig[i].hname = PROXY_AUTHENTICATE;
  pconfig[i].ignored_when_invalid = 1;
  pconfig[i++].setheader = (&osip_message_set_proxy_authenticate);
#ifndef MINISIZE
  pconfig[i].hname = PROXY_AUTHENTICATION_INFO;
  pconfig[i].ignored_when_invalid = 1;
  pconfig[i++].setheader = (&osip_message_set_proxy_authentication_info);
#endif
  pconfig[i].hname = PROXY_AUTHORIZATION;
  pconfig[i].ignored_when_invalid = 1;
  pconfig[i++].setheader = (&osip_message_set_proxy_authorization);
  pconfig[i].hname = RECORD_ROUTE;
  pconfig[i].ignored_when_invalid = 1;  /* best effort - but should be 0 */
  pconfig[i++].setheader = (&osip_message_set_record_route);
  pconfig[i].hname = ROUTE;
  pconfig[i].ignored_when_invalid = 1;  /* best effort - but should be 0 */
  pconfig[i++].setheader = (&osip_message_set_route);
  pconfig[i].hname = TO_SHORT;
  pconfig[i].ignored_when_invalid = 0;
  pconfig[i++].setheader = (&osip_message_set_to);
  pconfig[i].hname = TO;
  pconfig[i].ignored_when_invalid = 0;
  pconfig[i++].setheader = (&osip_message_set_to);
  pconfig[i].hname = VIA_SHORT;
  pconfig[i].ignored_when_invalid = 0;
  pconfig[i++].setheader = (&osip_message_set_via);
  pconfig[i].hname = VIA;
  pconfig[i].ignored_when_invalid = 0;
  pconfig[i++].setheader = (&osip_message_set_via);
  pconfig[i].hname = WWW_AUTHENTICATE;
  pconfig[i].ignored_when_invalid = 1;
  pconfig[i++].setheader = (&osip_message_set_www_authenticate);

  /* build up hash table for fast header lookup */

  /* initialize the table */
  for (i = 0; i < HASH_TABLE_SIZE; i++) {
    hdr_ref_table[i] = -1;      /* -1 -> no entry */
  }

  for (i = 0; i < NUMBER_OF_HEADERS; i++) {
    static int OrenIshShalom=0;
    unsigned long hash;

    fprintf(stdout,">> LIBOSIP BREAKPOINT[3][%d]\n",OrenIshShalom++);

    /* calculate hash value using lower case */
    /* Fixed: do not lower constant... osip_tolower( pconfig[i].hname ); */
    hash = osip_hash (pconfig[i].hname);

    hash = hash % HASH_TABLE_SIZE;

    if (hdr_ref_table[hash] == -1) {
      /* store reference(index) to pconfig table */
      hdr_ref_table[hash] = i;
    }
    else {
      /* oops, conflict!-> change the hash table or use another hash function size */
      OSIP_TRACE (osip_trace (__FILE__, __LINE__, OSIP_ERROR, NULL, "conflict with current hashtable size\n"));
      return OSIP_UNDEFINED_ERROR;
    }
  }

  return OSIP_SUCCESS;
}

/* improved look-up mechanism
   precondition: hname is all lowercase */
int
__osip_message_is_known_header (const char *hname)
{
  unsigned long hash;
  int result = -1;

  int index;

  hash = osip_hash (hname);
  hash = hash % HASH_TABLE_SIZE;
  index = hdr_ref_table[hash];

  if ((index != -1) && (0 == strcmp (pconfig[index].hname, hname))) {
    result = index;
  }

  return result;
}

#endif

/* This method calls the method that is able to parse the header */
int
__osip_message_call_method (int i, osip_message_t * dest, const char *hvalue)
{
  int err;

  err = pconfig[i].setheader (dest, hvalue);
  if (err < 0) {
    OSIP_TRACE (osip_trace (__FILE__, __LINE__, OSIP_WARNING, NULL, "Could not set header: %s: %s\n", pconfig[i].hname, hvalue));
  }
  if (pconfig[i].ignored_when_invalid == 1)
    return OSIP_SUCCESS;
  return err;
}
