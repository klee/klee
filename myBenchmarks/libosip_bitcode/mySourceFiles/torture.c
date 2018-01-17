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

#include "/home/oren/GIT/LatestKlee/vanillaKlee/include/klee/klee.h"

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

int
main (int argc, char **argv)
{
  int success = 1;

  int verbose = 0;              /* 1: verbose, 0 (or nothing: not verbose) */
  int clone = 0;                /* 1: verbose, 0 (or nothing: not verbose) */
  int binary = 0;
  FILE *torture_file;
  //char *msg;
  char msg[700];
  // int pos;
  int len;

  fprintf(stdout,">> LIBOSIP BREAKPOINT[0]\n");

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

  klee_make_symbolic(msg,700,"msg");
  
  //msg[  0] = 0x49;
  //msg[  1] = 0x4e;
  //msg[  2] = 0x56;
  //msg[  3] = 0x49;
  //msg[  4] = 0x54;
  //msg[  5] = 0x45;
  //msg[  6] = 0x20;
  //msg[  7] = 0x73;
  //msg[  8] = 0x69;
  //msg[  9] = 0x70;
  //msg[ 10] = 0x3a;
  //msg[ 11] = 0x75;
  //msg[ 12] = 0x73;
  //msg[ 13] = 0x65;
  //msg[ 14] = 0x72;
  //msg[ 15] = 0x40;
  //msg[ 16] = 0x63;
  //msg[ 17] = 0x7f;
  //msg[ 18] = 0x6d;
  //msg[ 19] = 0x61;
  //msg[ 20] = 0x70;
  //msg[ 21] = 0x6e;
  //msg[ 22] = 0x79;
  //msg[ 23] = 0x2e;
  //msg[ 24] = 0x63;
  //msg[ 25] = 0x6f;
  //msg[ 26] = 0x6d;
  //msg[ 27] = 0x20;
  //msg[ 28] = 0x53;
  //msg[ 29] = 0x49;
  //msg[ 30] = 0x50;
  //msg[ 31] = 0x2f;
  //msg[ 32] = 0x32;
  //msg[ 33] = 0x2e;
  //msg[ 34] = 0x30;
  //msg[ 35] = 0xd;
  //msg[ 36] = 0xa;
  //msg[ 37] = 0x54;
  //msg[ 38] = 0x6f;
  //msg[ 39] = 0x3a;
  //msg[ 40] = 0x20;
  //msg[ 41] = 0x73;
  //msg[ 42] = 0x69;
  //msg[ 43] = 0x70;
  //msg[ 44] = 0x3a;
  //msg[ 45] = 0x6a;
  //msg[ 46] = 0x2e;
  //msg[ 47] = 0x75;
  //msg[ 48] = 0x73;
  //msg[ 49] = 0x65;
  //msg[ 50] = 0x72;
  //msg[ 51] = 0x40;
  //msg[ 52] = 0x63;
  //msg[ 53] = 0x6f;
  //msg[ 54] = 0x6d;
  //msg[ 55] = 0x70;
  //msg[ 56] = 0x61;
  //msg[ 57] = 0x6e;
  //msg[ 58] = 0x79;
  //msg[ 59] = 0x2e;
  //msg[ 60] = 0x63;
  //msg[ 61] = 0x6f;
  //msg[ 62] = 0x6d;
  //msg[ 63] = 0xd;
  //msg[ 64] = 0xa;
  //msg[ 65] = 0x46;
  //msg[ 66] = 0x72;
  //msg[ 67] = 0x6f;
  //msg[ 68] = 0x6d;
  //msg[ 69] = 0x3a;
  //msg[ 70] = 0x20;
  //msg[ 71] = 0x73;
  //msg[ 72] = 0x69;
  //msg[ 73] = 0x70;
  //msg[ 74] = 0x3a;
  //msg[ 75] = 0x63;
  //msg[ 76] = 0x72;
  //msg[ 77] = 0x40;
  //msg[ 78] = 0x75;
  //msg[ 79] = 0x6e;
  //msg[ 80] = 0x69;
  //msg[ 81] = 0x76;
  //msg[ 82] = 0x65;
  //msg[ 83] = 0x72;
  //msg[ 84] = 0x73;
  //msg[ 85] = 0x69;
  //msg[ 86] = 0x74;
  //msg[ 87] = 0x79;
  //msg[ 88] = 0x2e;
  //msg[ 89] = 0x65;
  //msg[ 90] = 0x64;
  //msg[ 91] = 0x75;
  //msg[ 92] = 0xd;
  //msg[ 93] = 0xa;
  //msg[ 94] = 0x43;
  //msg[ 95] = 0x61;
  //msg[ 96] = 0x6c;
  //msg[ 97] = 0x6c;
  //msg[ 98] = 0x2d;
  //msg[ 99] = 0x49;
  //msg[100] = 0x44;
  //msg[101] = 0x3a;
  //msg[102] = 0x20;
  //msg[103] = 0x39;
  //msg[104] = 0x20;
  //msg[105] = 0x30;
  //msg[106] = 0x30;
  //msg[107] = 0x20;
  //msg[108] = 0x34;
  //msg[109] = 0x20;
  //msg[110] = 0x31;
  //msg[111] = 0x30;
  //msg[112] = 0x20;
  //msg[113] = 0x30;
  //msg[114] = 0x30;
  //msg[115] = 0x20;
  //msg[116] = 0x33;
  //msg[117] = 0x33;
  //msg[118] = 0x20;
  //msg[119] = 0x49;
  //msg[120] = 0x71;
  //msg[121] = 0x56;
  //msg[122] = 0x49;
  //msg[123] = 0x54;
  //msg[124] = 0x45;
  //msg[125] = 0xd;
  //msg[126] = 0xa;
  //msg[127] = 0x56;
  //msg[128] = 0x69;
  //msg[129] = 0x61;
  //msg[130] = 0x3a;
  //msg[131] = 0x20;
  //msg[132] = 0x53;
  //msg[133] = 0x49;
  //msg[134] = 0x50;
  //msg[135] = 0x2f;
  //msg[136] = 0x32;
  //msg[137] = 0x2e;
  //msg[138] = 0x30;
  //msg[139] = 0x2f;
  //msg[140] = 0x55;
  //msg[141] = 0x44;
  //msg[142] = 0x50;
  //msg[143] = 0x20;
  //msg[144] = 0x31;
  //msg[145] = 0x33;
  //msg[146] = 0x35;
  //msg[147] = 0x2e;
  //msg[148] = 0x31;
  //msg[149] = 0x38;
  //msg[150] = 0x30;
  //msg[151] = 0x2e;
  //msg[152] = 0x31;
  //msg[153] = 0x33;
  //msg[154] = 0x30;
  //msg[155] = 0x2e;
  //msg[156] = 0x31;
  //msg[157] = 0x33;
  //msg[158] = 0x33;
  //msg[159] = 0xd;
  //msg[160] = 0xa;
  //msg[161] = 0x31;
  //msg[162] = 0x32;
  //msg[163] = 0x74;
  //msg[164] = 0x3a;
  //msg[165] = 0x20;
  //msg[166] = 0x3c;
  //msg[167] = 0x73;
  //msg[168] = 0x69;
  //msg[169] = 0x70;
  //msg[170] = 0x3a;
  //msg[171] = 0x6a;
  //msg[172] = 0x70;
  //msg[173] = 0x65;
  //msg[174] = 0x74;
  //msg[175] = 0x65;
  //msg[176] = 0x72;
  //msg[177] = 0x73;
  //msg[178] = 0x6f;
  //msg[179] = 0x6e;
  //msg[180] = 0x40;
  //msg[181] = 0x6c;
  //msg[182] = 0x65;
  //msg[183] = 0x76;
  //msg[184] = 0x65;
  //msg[185] = 0x6c;
  //msg[186] = 0x33;
  //msg[187] = 0x2e;
  //msg[188] = 0x63;
  //msg[189] = 0x6f;
  //msg[190] = 0x43;
  //msg[191] = 0x53;
  //msg[192] = 0x65;
  //msg[193] = 0x71;
  //msg[194] = 0x3a;
  //msg[195] = 0x20;
  //msg[196] = 0x38;
  //msg[197] = 0x33;
  //msg[198] = 0x34;
  //msg[199] = 0x38;
  //msg[200] = 0x20;
  //msg[201] = 0x49;
  //msg[202] = 0x4e;
  //msg[203] = 0x56;
  //msg[204] = 0x49;
  //msg[205] = 0x54;
  //msg[206] = 0x45;
  //msg[207] = 0xd;
  //msg[208] = 0xa;
  //msg[209] = 0x43;
  //msg[210] = 0x6f;
  //msg[211] = 0x6e;
  //msg[212] = 0x74;
  //msg[213] = 0x61;
  //msg[214] = 0x63;
  //msg[215] = 0x74;
  //msg[216] = 0x3a;
  //msg[217] = 0x20;
  //msg[218] = 0x3c;
  //msg[219] = 0x73;
  //msg[220] = 0x69;
  //msg[221] = 0x70;
  //msg[222] = 0x3a;
  //msg[223] = 0x6a;
  //msg[224] = 0x70;
  //msg[225] = 0x65;
  //msg[226] = 0x74;
  //msg[227] = 0x65;
  //msg[228] = 0x72;
  //msg[229] = 0x73;
  //msg[230] = 0x6f;
  //msg[231] = 0x6e;
  //msg[232] = 0x40;
  //msg[233] = 0x6c;
  //msg[234] = 0x65;
  //msg[235] = 0x76;
  //msg[236] = 0x65;
  //msg[237] = 0x6c;
  //msg[238] = 0x33;
  //msg[239] = 0x2e;
  //msg[240] = 0x63;
  //msg[241] = 0x6f;
  //msg[242] = 0x6d;
  //msg[243] = 0x3e;
  //msg[244] = 0xd;
  //msg[245] = 0xa;
  //msg[246] = 0x43;
  //msg[247] = 0x6f;
  //msg[248] = 0x6e;
  //msg[249] = 0x74;
  //msg[250] = 0x65;
  //msg[251] = 0x6e;
  //msg[252] = 0x74;
  //msg[253] = 0x2d;
  //msg[254] = 0x4c;
  //msg[255] = 0x65;
  //msg[256] = 0x6e;
  //msg[257] = 0x67;
  //msg[258] = 0x74;
  //msg[259] = 0x68;
  //msg[260] = 0x3a;
  //msg[261] = 0x20;
  //msg[262] = 0x35;
  //msg[263] = 0x36;
  //msg[264] = 0x32;
  //msg[265] = 0xd;
  //msg[266] = 0xa;
  //msg[267] = 0x43;
  //msg[268] = 0x6f;
  //msg[269] = 0x6e;
  //msg[270] = 0x74;
  //msg[271] = 0x65;
  //msg[272] = 0x6e;
  //msg[273] = 0x74;
  //msg[274] = 0x2d;
  //msg[275] = 0x54;
  //msg[276] = 0x79;
  //msg[277] = 0x70;
  //msg[278] = 0x65;
  //msg[279] = 0x3a;
  //msg[280] = 0x20;
  //msg[281] = 0x6d;
  //msg[282] = 0x75;
  //msg[283] = 0x6c;
  //msg[284] = 0x74;
  //msg[285] = 0x69;
  //msg[286] = 0x70;
  //msg[287] = 0x61;
  //msg[288] = 0x72;
  //msg[289] = 0x74;
  //msg[290] = 0x2f;
  //msg[291] = 0x6d;
  //msg[292] = 0x69;
  //msg[293] = 0x78;
  //msg[294] = 0x65;
  //msg[295] = 0x64;
  //msg[296] = 0x3b;
  //msg[297] = 0x20;
  //msg[298] = 0x62;
  //msg[299] = 0x6f;
  //msg[300] = 0x75;
  //msg[301] = 0x6e;
  //msg[302] = 0x64;
  //msg[303] = 0x61;
  //msg[304] = 0x72;
  //msg[305] = 0x79;
  //msg[306] = 0x3d;
  //msg[307] = 0x75;
  //msg[308] = 0x6e;
  //msg[309] = 0x69;
  //msg[310] = 0x71;
  //msg[311] = 0x75;
  //msg[312] = 0x65;
  //msg[313] = 0x2d;
  //msg[314] = 0x62;
  //msg[315] = 0x6f;
  //msg[316] = 0x75;
  //msg[317] = 0x6e;
  //msg[318] = 0x64;
  //msg[319] = 0x61;
  //msg[320] = 0x72;
  //msg[321] = 0x79;
  //msg[322] = 0x2d;
  //msg[323] = 0x31;
  //msg[324] = 0xd;
  //msg[325] = 0xa;
  //msg[326] = 0x4d;
  //msg[327] = 0x49;
  //msg[328] = 0x4d;
  //msg[329] = 0x45;
  //msg[330] = 0x2d;
  //msg[331] = 0x56;
  //msg[332] = 0x65;
  //msg[333] = 0x72;
  //msg[334] = 0x73;
  //msg[335] = 0x69;
  //msg[336] = 0x6f;
  //msg[337] = 0x6e;
  //msg[338] = 0x3a;
  //msg[339] = 0x20;
  //msg[340] = 0x31;
  //msg[341] = 0x2e;
  //msg[342] = 0x30;
  //msg[343] = 0xd;
  //msg[344] = 0xa;
  //msg[345] = 0xd;
  //msg[346] = 0xa;
  //msg[347] = 0x2d;
  //msg[348] = 0x2d;
  //msg[349] = 0x75;
  //msg[350] = 0x6e;
  //msg[351] = 0x69;
  //msg[352] = 0x71;
  //msg[353] = 0x75;
  //msg[354] = 0x65;
  //msg[355] = 0x2d;
  //msg[356] = 0x62;
  //msg[357] = 0x6f;
  //msg[358] = 0x75;
  //msg[359] = 0x6e;
  //msg[360] = 0x64;
  //msg[361] = 0x61;
  //msg[362] = 0x72;
  //msg[363] = 0x79;
  //msg[364] = 0x2d;
  //msg[365] = 0x31;
  //msg[366] = 0xd;
  //msg[367] = 0xa;
  //msg[368] = 0x43;
  //msg[369] = 0x6f;
  //msg[370] = 0x6e;
  //msg[371] = 0x74;
  //msg[372] = 0x65;
  //msg[373] = 0x6e;
  //msg[374] = 0x74;
  //msg[375] = 0x2d;
  //msg[376] = 0x54;
  //msg[377] = 0x79;
  //msg[378] = 0x70;
  //msg[379] = 0x65;
  //msg[380] = 0x3a;
  //msg[381] = 0x20;
  //msg[382] = 0x61;
  //msg[383] = 0x70;
  //msg[384] = 0x61;
  //msg[385] = 0x74;
  //msg[386] = 0x69;
  //msg[387] = 0x6f;
  //msg[388] = 0x6e;
  //msg[389] = 0x2f;
  //msg[390] = 0x53;
  //msg[391] = 0x44;
  //msg[392] = 0x50;
  //msg[393] = 0x3b;
  //msg[394] = 0x20;
  //msg[395] = 0x63;
  //msg[396] = 0x68;
  //msg[397] = 0x61;
  //msg[398] = 0x72;
  //msg[399] = 0x73;
  //msg[400] = 0x65;
  //msg[401] = 0x74;
  //msg[402] = 0x3d;
  //msg[403] = 0x49;
  //msg[404] = 0x53;
  //msg[405] = 0x4f;
  //msg[406] = 0x2d;
  //msg[407] = 0x31;
  //msg[408] = 0x30;
  //msg[409] = 0x36;
  //msg[410] = 0x34;
  //msg[411] = 0x36;
  //msg[412] = 0x38;
  //msg[413] = 0x38;
  //msg[414] = 0x35;
  //msg[415] = 0xd;
  //msg[416] = 0xa;
  //msg[417] = 0xd;
  //msg[418] = 0xa;
  //msg[419] = 0xa;
  //msg[420] = 0x6f;
  //msg[421] = 0x3d;
  //msg[422] = 0x6a;
  //msg[423] = 0x34;
  //msg[424] = 0x36;
  //msg[425] = 0x39;
  //msg[426] = 0x36;
  //msg[427] = 0xd;
  //msg[428] = 0xa;
  //msg[429] = 0x6d;
  //msg[430] = 0x3d;
  //msg[431] = 0x61;
  //msg[432] = 0x75;
  //msg[433] = 0x64;
  //msg[434] = 0x69;
  //msg[435] = 0x6f;
  //msg[436] = 0x20;
  //msg[437] = 0x39;
  //msg[438] = 0x30;
  //msg[439] = 0x39;
  //msg[440] = 0x32;
  //msg[441] = 0x20;
  //msg[442] = 0x52;
  //msg[443] = 0x54;
  //msg[444] = 0x50;
  //msg[445] = 0x2f;
  //msg[446] = 0x41;
  //msg[447] = 0x56;
  //msg[448] = 0x50;
  //msg[449] = 0x20;
  //msg[450] = 0x30;
  //msg[451] = 0x20;
  //msg[452] = 0x33;
  //msg[453] = 0x20;
  //msg[454] = 0x34;
  //msg[455] = 0xd;
  //msg[456] = 0xa;
  //msg[457] = 0x2d;
  //msg[458] = 0x2d;
  //msg[459] = 0x75;
  //msg[460] = 0x6e;
  //msg[461] = 0x69;
  //msg[462] = 0x71;
  //msg[463] = 0x75;
  //msg[464] = 0x65;
  //msg[465] = 0x2d;
  //msg[466] = 0x62;
  //msg[467] = 0x6f;
  //msg[468] = 0x75;
  //msg[469] = 0x6e;
  //msg[470] = 0x64;
  //msg[471] = 0x61;
  //msg[472] = 0x72;
  //msg[473] = 0x79;
  //msg[474] = 0x2d;
  //msg[475] = 0x31;
  //msg[476] = 0xd;
  //msg[477] = 0xa;
  //msg[478] = 0x43;
  //msg[479] = 0x6f;
  //msg[480] = 0x6e;
  //msg[481] = 0x74;
  //msg[482] = 0x65;
  //msg[483] = 0x6e;
  //msg[484] = 0x74;
  //msg[485] = 0x2d;
  //msg[486] = 0x54;
  //msg[487] = 0x79;
  //msg[488] = 0x70;
  //msg[489] = 0x65;
  //msg[490] = 0x3a;
  //msg[491] = 0x20;
  //msg[492] = 0x61;
  //msg[493] = 0x70;
  //msg[494] = 0x70;
  //msg[495] = 0x2f;
  //msg[496] = 0x49;
  //msg[497] = 0x53;
  //msg[498] = 0xe8;
  //msg[499] = 0x3;
  //msg[500] = 0x43;
  //msg[501] = 0x6f;
  //msg[502] = 0x6e;
  //msg[503] = 0x74;
  //msg[504] = 0x42;
  //msg[505] = 0x6e;
  //msg[506] = 0x74;
  //msg[507] = 0x2d;
  //msg[508] = 0x44;
  //msg[509] = 0x69;
  //msg[510] = 0x73;
  //msg[511] = 0x70;
  //msg[512] = 0x6f;
  //msg[513] = 0x73;
  //msg[514] = 0x69;
  //msg[515] = 0x74;
  //msg[516] = 0x69;
  //msg[517] = 0x6f;
  //msg[518] = 0x6e;
  //msg[519] = 0x3a;
  //msg[520] = 0x20;
  //msg[521] = 0x73;
  //msg[522] = 0x69;
  //msg[523] = 0x67;
  //msg[524] = 0x6e;
  //msg[525] = 0x61;
  //msg[526] = 0x6c;
  //msg[527] = 0x3b;
  //msg[528] = 0x20;
  //msg[529] = 0x68;
  //msg[530] = 0x61;
  //msg[531] = 0x6e;
  //msg[532] = 0x64;
  //msg[533] = 0x6c;
  //msg[534] = 0x69;
  //msg[535] = 0x6e;
  //msg[536] = 0x67;
  //msg[537] = 0x3d;
  //msg[538] = 0x6f;
  //msg[539] = 0x70;
  //msg[540] = 0x74;
  //msg[541] = 0x69;
  //msg[542] = 0x6f;
  //msg[543] = 0x6e;
  //msg[544] = 0x61;
  //msg[545] = 0x6c;
  //msg[546] = 0xd;
  //msg[547] = 0xa;
  //msg[548] = 0xd;
  //msg[549] = 0xa;
  //msg[550] = 0x30;
  //msg[551] = 0x30;
  //msg[552] = 0x30;
  //msg[553] = 0x20;
  //msg[554] = 0x30;
  //msg[555] = 0x30;
  //msg[556] = 0x20;
  //msg[557] = 0x38;
  //msg[558] = 0x39;
  //msg[559] = 0x20;
  //msg[560] = 0x38;
  //msg[561] = 0x62;
  //msg[562] = 0xa;
  //msg[563] = 0x30;
  //msg[564] = 0x65;
  //msg[565] = 0x20;
  //msg[566] = 0x39;
  //msg[567] = 0x35;
  //msg[568] = 0x20;
  //msg[569] = 0x31;
  //msg[570] = 0x65;
  //msg[571] = 0x20;
  //msg[572] = 0x31;
  //msg[573] = 0x65;
  //msg[574] = 0x20;
  //msg[575] = 0x31;
  //msg[576] = 0x65;
  //msg[577] = 0x20;
  //msg[578] = 0x30;
  //msg[579] = 0x36;
  //msg[580] = 0x20;
  //msg[581] = 0x32;
  //msg[582] = 0x36;
  //msg[583] = 0x20;
  //msg[584] = 0x30;
  //msg[585] = 0x35;
  //msg[586] = 0x20;
  //msg[587] = 0x30;
  //msg[588] = 0x64;
  //msg[589] = 0x20;
  //msg[590] = 0x66;
  //msg[591] = 0x35;
  //msg[592] = 0x20;
  //msg[593] = 0x30;
  //msg[594] = 0x31;
  //msg[595] = 0x20;
  //msg[596] = 0x30;
  //msg[597] = 0x36;
  //msg[598] = 0x20;
  //msg[599] = 0x31;
  //msg[600] = 0x30;
  //msg[601] = 0x20;
  //msg[602] = 0x30;
  //msg[603] = 0x34;
  //msg[604] = 0x20;
  //msg[605] = 0x30;
  //msg[606] = 0x30;
  //msg[607] = 0xa;
  //msg[608] = 0x2d;
  //msg[609] = 0x2d;
  //msg[610] = 0x75;
  //msg[611] = 0x6e;
  //msg[612] = 0x69;
  //msg[613] = 0x71;
  //msg[614] = 0x75;
  //msg[615] = 0x65;
  //msg[616] = 0x2d;
  //msg[617] = 0x62;
  //msg[618] = 0x6f;
  //msg[619] = 0x75;
  //msg[620] = 0x6e;
  //msg[621] = 0x64;
  //msg[622] = 0x61;
  //msg[623] = 0x72;
  //msg[624] = 0x79;
  //msg[625] = 0x2d;
  //msg[626] = 0x31;
  //msg[627] = 0x2d;
  //msg[628] = 0x2d;
  //msg[629] = 0xd;
  //msg[630] = 0xa;
  //msg[631] =    0;
  
  fprintf (stdout, ">> LIBOSIP BREAKPOINT[1]\n");  
  
  len = strlen(msg);
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
    while (j != 0) {
      j--;
      osip_message_init (&sip);
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
