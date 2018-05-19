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


#ifdef ENABLE_MPATROL
#include <mpatrol.h>
#endif

#include <osipparser2/internal.h>
#include <osipparser2/osip_port.h>
#include <osipparser2/osip_parser.h>
#include <osipparser2/sdp_message.h>

#include "/home/oren/GIT/klee/str.klee/klee/include/klee/klee.h"

void klee_make_symbolic(void *addr,size_t count, const char *message);

int test_message (char *msg, size_t len, int verbose, int clone);
static void usage (void);

static void
usage ()
{
  fprintf (stderr, "Usage: ./torture_test torture_file number [-v] [-c]\n");
  exit (1);
}

static int
read_binary (char **msg, int *len, FILE * torture_file)
{
  *msg = (char *) osip_malloc (100000); /* msg are under 10000 */

  *len = fread (*msg, 1, 100000, torture_file);
  return ferror (torture_file) ? -1 : 0;
}

static char *
read_text (int num, FILE * torture_file)
{
  char *marker;
  char *tmp;
  char *tmpmsg;
  char *msg;
  int i;
  static int num_test = 0;

  i = 0;
  tmp = (char *) osip_malloc (500);
  marker = fgets (tmp, 500, torture_file);      /* lines are under 500 */
  while (marker != NULL && i < num) {
    if (0 == strncmp (tmp, "|", 1))
      i++;
    marker = fgets (tmp, 500, torture_file);
  }
  num_test++;

  msg = (char *) osip_malloc (100000);  /* msg are under 10000 */
  if (msg == NULL) {
    fprintf (stderr, "Error! osip_malloc failed\n");
    return NULL;
  }
  tmpmsg = msg;

  if (marker == NULL) {
    fprintf (stderr, "Error! The message's number you specified does not exist\n");
    return NULL;                /* end of file detected! */
  }
  /* this part reads an entire message, separator is "|" */
  /* (it is unlinkely that it will appear in messages!) */
  while (marker != NULL && strncmp (tmp, "|", 1)) {
    osip_strncpy (tmpmsg, tmp, strlen (tmp));
    tmpmsg = tmpmsg + strlen (tmp);
    marker = fgets (tmp, 500, torture_file);
  }

  osip_free (tmp);
  return msg;
}

#include <assert.h>

void markString(char *p){}

void klee_make_symbolic(void *addr,size_t count, const char *message){}

extern void BREAKPOINT(int serial);

int main (int argc, char **argv)
{
  int success = 1;

  int verbose = 0;              /* 1: verbose, 0 (or nothing: not verbose) */
  int clone = 0;                /* 1: verbose, 0 (or nothing: not verbose) */
  int binary = 0;
  FILE *torture_file;
  //char *msg;
  char msg[10];
  // int pos;
  int len;

  BREAKPOINT(0);

  //for (pos = 3; pos < argc; pos++) {
  //  if (0 == strncmp (argv[pos], "-v", 2))
  //    verbose = 1;
  //  else if (0 == strncmp (argv[pos], "-c", 2))
  //    clone = 1;
  //  else if (0 == strncmp (argv[pos], "-b", 2))
  //    binary = 1;
  //  else
  //    usage ();
  //}

  //if (argc < 3) {
  //  usage ();
  //}

  //torture_file = fopen (argv[1], "r");
  //if (torture_file == NULL) {
  //  usage ();
  //}

  /* initialize parser */
  parser_init ();

  BREAKPOINT(1);
  
  //if (binary) {
  //  if (read_binary (&msg, &len, torture_file) < 0)
  //    return -1;
  //}
  //else {
  //  msg = read_text (atoi (argv[2]), torture_file);
  //  if (!msg)
  //    return -1;
  //  len = strlen (msg);
  //}

  klee_make_symbolic(msg,10,"MsG");
  //msg[0]='a';
  //msg[1]='b';
  //msg[3]='c';
  msg[9]= 0 ;
  
  len = strlen(msg);

  BREAKPOINT(2);

  success = test_message (msg, len, verbose, clone);
  //if (verbose) {
  //  fprintf (stdout, "test %s : ============================ \n", argv[2]);
  //  fwrite (msg, 1, len, stdout);

  //  if (0 == success)
  //    fprintf (stdout, "test %s : ============================ OK\n", argv[2]);
  //  else
  //    fprintf (stdout, "test %s : ============================ FAILED\n", argv[2]);
  //}

  //osip_free (msg);
  //fclose (torture_file);
#ifdef __linux
  if (success)
    exit (EXIT_FAILURE);
  else
    exit (EXIT_SUCCESS);
#endif
  return success;
}

int
test_message (char *msg, size_t len, int verbose, int clone)
{
  osip_message_t *sip;

  {
    char *result;

    /* int j=10000; */
    int j = 1;

    if (verbose)
      fprintf (stdout, "Trying %i sequentials calls to osip_message_init(), osip_message_parse() and osip_message_free()\n", j);

    BREAKPOINT(3);

    while (j != 0) {
      j--;
      osip_message_init (&sip);

      fprintf(stdout,">> LIBOSIP BREAKPOINT[4]\n");

      if (osip_message_parse (sip, msg, len) != 0) {
        fprintf (stdout, "LIBOSIP ERROR: failed while parsing!\n");
        osip_message_free (sip);
        return -1;
      }
      osip_message_free (sip);
    }

    osip_message_init (&sip);
    if (osip_message_parse (sip, msg, len) != 0) {
      fprintf (stdout, "LIBOSIP ERROR: failed while parsing!\n");
      osip_message_free (sip);
      return -1;
    }
    else {
      int i;
      size_t length;

#if 0
      sdp_message_t *sdp;
      osip_body_t *oldbody;
      int pos;

      pos = 0;
      while (!osip_list_eol (&sip->bodies, pos)) {
        oldbody = (osip_body_t *) osip_list_get (&sip->bodies, pos);
        pos++;
        sdp_message_init (&sdp);
        i = sdp_message_parse (sdp, oldbody->body);
        if (i != 0) {
          fprintf (stdout, "ERROR: Bad SDP!\n");
        }
        else
          fprintf (stdout, "SUCCESS: Correct SDP!\n");
        sdp_message_free (sdp);
        sdp = NULL;
      }
#endif

      osip_message_force_update (sip);
      i = osip_message_to_str (sip, &result, &length);
      if (i == -1) {
        fprintf (stdout, "ERROR: failed while printing message!\n");
        osip_message_free (sip);
        return -1;
      }
      else {
        if (verbose)
          fwrite (result, 1, length, stdout);
        if (clone) {
          /* create a clone of message */
          /* int j = 10000; */
          int j = 1;

          if (verbose)
            fprintf (stdout, "Trying %i sequentials calls to osip_message_clone() and osip_message_free()\n", j);
          while (j != 0) {
            osip_message_t *copy;

            j--;
            i = osip_message_clone (sip, &copy);
            if (i != 0) {
              fprintf (stdout, "ERROR: failed while creating copy of message!\n");
            }
            else {
              char *tmp;
              size_t length;

              osip_message_force_update (copy);
              i = osip_message_to_str (copy, &tmp, &length);
              if (i != 0) {
                fprintf (stdout, "ERROR: failed while printing message!\n");
              }
              else {
                if (0 == strcmp (result, tmp)) {
                  if (verbose)
                    printf ("The osip_message_clone method works perfectly\n");
                }
                else
                  printf ("ERROR: The osip_message_clone method DOES NOT works\n");
                if (verbose) {
                  printf ("Here is the copy: \n");
                  fwrite (tmp, 1, length, stdout);
                  printf ("\n");
                }

                osip_free (tmp);
              }
              osip_message_free (copy);
            }
          }
          if (verbose)
            fprintf (stdout, "sequentials calls: done\n");
        }
        osip_free (result);
      }
      osip_message_free (sip);
    }
  }
  return 0;
}
