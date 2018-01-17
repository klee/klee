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
  
  msg[  0] = 0x49;
  msg[  1] = 0x4E;
  msg[  2] = 0x56;
  msg[  3] = 0x49;
  msg[  4] = 0x54;
  msg[  5] = 0x45;
  msg[  6] = 0x20;
  msg[  7] = 0x73;
  msg[  8] = 0x69;
  msg[  9] = 0x70;
  msg[ 10] = 0x3A;
  msg[ 11] = 0x75;
  msg[ 12] = 0x73;
  msg[ 13] = 0x65;
  msg[ 14] = 0x72;
  msg[ 15] = 0x40;
  msg[ 16] = 0x63;
  msg[ 17] = 0x7F;
  msg[ 18] = 0x6D;
  msg[ 19] = 0x61;
  msg[ 20] = 0x70;
  msg[ 21] = 0x6E;
  msg[ 22] = 0x79;
  msg[ 23] = 0x2E;
  msg[ 24] = 0x63;
  msg[ 25] = 0x6F;
  msg[ 26] = 0x6D;
  msg[ 27] = 0x20;
  msg[ 28] = 0x53;
  msg[ 29] = 0x49;
  msg[ 30] = 0x50;
  msg[ 31] = 0x2F;
  msg[ 32] = 0x32;
  msg[ 33] = 0x2E;
  msg[ 34] = 0x30;
  msg[ 35] = 0x0D;
  msg[ 36] = 0x0A;
  msg[ 37] = 0x54;
  msg[ 38] = 0x6F;
  msg[ 39] = 0x3A;
  msg[ 40] = 0x20;
  msg[ 41] = 0x73;
  msg[ 42] = 0x69;
  msg[ 43] = 0x70;
  msg[ 44] = 0x3A;
  msg[ 45] = 0x6A;
  msg[ 46] = 0x2E;
  msg[ 47] = 0x75;
  msg[ 48] = 0x73;
  msg[ 49] = 0x65;
  msg[ 50] = 0x72;
  msg[ 51] = 0x40;
  msg[ 52] = 0x63;
  msg[ 53] = 0x6F;
  msg[ 54] = 0x6D;
  msg[ 55] = 0x70;
  msg[ 56] = 0x61;
  msg[ 57] = 0x6E;
  msg[ 58] = 0x79;
  msg[ 59] = 0x2E;
  msg[ 60] = 0x63;
  msg[ 61] = 0x6F;
  msg[ 62] = 0x6D;
  msg[ 63] = 0x0D;
  msg[ 64] = 0x0A;
  msg[ 65] = 0x46;
  msg[ 66] = 0x72;
  msg[ 67] = 0x6F;
  msg[ 68] = 0x6D;
  msg[ 69] = 0x3A;
  msg[ 70] = 0x20;
  msg[ 71] = 0x73;
  msg[ 72] = 0x69;
  msg[ 73] = 0x70;
  msg[ 74] = 0x3A;
  msg[ 75] = 0x63;
  msg[ 76] = 0x72;
  msg[ 77] = 0x40;
  msg[ 78] = 0x75;
  msg[ 79] = 0x6E;
  msg[ 80] = 0x69;
  msg[ 81] = 0x76;
  msg[ 82] = 0x65;
  msg[ 83] = 0x72;
  msg[ 84] = 0x73;
  msg[ 85] = 0x69;
  msg[ 86] = 0x74;
  msg[ 87] = 0x79;
  msg[ 88] = 0x2E;
  msg[ 89] = 0x65;
  msg[ 90] = 0x64;
  msg[ 91] = 0x75;
  msg[ 92] = 0x0D;
  msg[ 93] = 0x0A;
  msg[ 94] = 0x43;
  msg[ 95] = 0x61;
  msg[ 96] = 0x6C;
  msg[ 97] = 0x6C;
  msg[ 98] = 0x2D;
  msg[ 99] = 0x49;
  msg[100] = 0x44;
  msg[101] = 0x3A;
  msg[102] = 0x20;
  msg[103] = 0x39;
  msg[104] = 0x20;
  msg[105] = 0x30;
  msg[106] = 0x30;
  msg[107] = 0x20;
  msg[108] = 0x34;
  msg[109] = 0x20;
  msg[110] = 0x31;
  msg[111] = 0x30;
  msg[112] = 0x20;
  msg[113] = 0x30;
  msg[114] = 0x30;
  msg[115] = 0x20;
  msg[116] = 0x33;
  msg[117] = 0x33;
  msg[118] = 0x20;
  msg[119] = 0x49;
  msg[120] = 0x71;
  msg[121] = 0x56;
  msg[122] = 0x49;
  msg[123] = 0x54;
  msg[124] = 0x45;
  msg[125] = 0x0D;
  msg[126] = 0x0A;
  msg[127] = 0x56;
  msg[128] = 0x69;
  msg[129] = 0x61;
  msg[130] = 0x3A;
  msg[131] = 0x20;
  msg[132] = 0x53;
  msg[133] = 0x49;
  msg[134] = 0x50;
  msg[135] = 0x2F;
  msg[136] = 0x32;
  msg[137] = 0x2E;
  msg[138] = 0x30;
  msg[139] = 0x2F;
  msg[140] = 0x55;
  msg[141] = 0x44;
  msg[142] = 0x50;
  msg[143] = 0x20;
  msg[144] = 0x31;
  msg[145] = 0x33;
  msg[146] = 0x35;
  msg[147] = 0x2E;
  msg[148] = 0x31;
  msg[149] = 0x38;
  msg[150] = 0x30;
  msg[151] = 0x2E;
  msg[152] = 0x31;
  msg[153] = 0x33;
  msg[154] = 0x30;
  msg[155] = 0x2E;
  msg[156] = 0x31;
  msg[157] = 0x33;
  msg[158] = 0x33;
  msg[159] = 0x0D;
  msg[160] = 0x0A;
  msg[161] = 0x31;
  msg[162] = 0x32;
  msg[163] = 0x74;
  msg[164] = 0x3A;
  msg[165] = 0x20;
  msg[166] = 0x3C;
  msg[167] = 0x73;
  msg[168] = 0x69;
  msg[169] = 0x70;
  msg[170] = 0x3A;
  msg[171] = 0x6A;
  msg[172] = 0x70;
  msg[173] = 0x65;
  msg[174] = 0x74;
  msg[175] = 0x65;
  msg[176] = 0x72;
  msg[177] = 0x73;
  msg[178] = 0x6F;
  msg[179] = 0x6E;
  msg[180] = 0x40;
  msg[181] = 0x6C;
  msg[182] = 0x65;
  msg[183] = 0x76;
  msg[184] = 0x65;
  msg[185] = 0x6C;
  msg[186] = 0x33;
  msg[187] = 0x2E;
  msg[188] = 0x63;
  msg[189] = 0x6F;
  msg[190] = 0x43;
  msg[191] = 0x53;
  msg[192] = 0x65;
  msg[193] = 0x71;
  msg[194] = 0x3A;
  msg[195] = 0x20;
  msg[196] = 0x38;
  msg[197] = 0x33;
  msg[198] = 0x34;
  msg[199] = 0x38;
  msg[200] = 0x20;
  msg[201] = 0x49;
  msg[202] = 0x4E;
  msg[203] = 0x56;
  msg[204] = 0x49;
  msg[205] = 0x54;
  msg[206] = 0x45;
  msg[207] = 0x0D;
  msg[208] = 0x0A;
  msg[209] = 0x43;
  msg[210] = 0x6F;
  msg[211] = 0x6E;
  msg[212] = 0x74;
  msg[213] = 0x61;
  msg[214] = 0x63;
  msg[215] = 0x74;
  msg[216] = 0x3A;
  msg[217] = 0x20;
  msg[218] = 0x3C;
  msg[219] = 0x73;
  msg[220] = 0x69;
  msg[221] = 0x70;
  msg[222] = 0x3A;
  msg[223] = 0x6A;
  msg[224] = 0x70;
  msg[225] = 0x65;
  msg[226] = 0x74;
  msg[227] = 0x65;
  msg[228] = 0x72;
  msg[229] = 0x73;
  msg[230] = 0x6F;
  msg[231] = 0x6E;
  msg[232] = 0x40;
  msg[233] = 0x6C;
  msg[234] = 0x65;
  msg[235] = 0x76;
  msg[236] = 0x65;
  msg[237] = 0x6C;
  msg[238] = 0x33;
  msg[239] = 0x2E;
  msg[240] = 0x63;
  msg[241] = 0x6F;
  msg[242] = 0x6D;
  msg[243] = 0x3E;
  msg[244] = 0x0D;
  msg[245] = 0x0A;
  msg[246] = 0x43;
  msg[247] = 0x6F;
  msg[248] = 0x6E;
  msg[249] = 0x74;
  msg[250] = 0x65;
  msg[251] = 0x6E;
  msg[252] = 0x74;
  msg[253] = 0x2D;
  msg[254] = 0x4C;
  msg[255] = 0x65;
  msg[256] = 0x6E;
  msg[257] = 0x67;
  msg[258] = 0x74;
  msg[259] = 0x68;
  msg[260] = 0x3A;
  msg[261] = 0x20;
  msg[262] = 0x35;
  msg[263] = 0x36;
  msg[264] = 0x32;
  msg[265] = 0x0D;
  msg[266] = 0x0A;
  msg[267] = 0x43;
  msg[268] = 0x6F;
  msg[269] = 0x6E;
  msg[270] = 0x74;
  msg[271] = 0x65;
  msg[272] = 0x6E;
  msg[273] = 0x74;
  msg[274] = 0x2D;
  msg[275] = 0x54;
  msg[276] = 0x79;
  msg[277] = 0x70;
  msg[278] = 0x65;
  msg[279] = 0x3A;
  msg[280] = 0x20;
  msg[281] = 0x6D;
  msg[282] = 0x75;
  msg[283] = 0x6C;
  msg[284] = 0x74;
  msg[285] = 0x69;
  msg[286] = 0x70;
  msg[287] = 0x61;
  msg[288] = 0x72;
  msg[289] = 0x74;
  msg[290] = 0x2F;
  msg[291] = 0x6D;
  msg[292] = 0x69;
  msg[293] = 0x78;
  msg[294] = 0x65;
  msg[295] = 0x64;
  msg[296] = 0x3B;
  msg[297] = 0x20;
  msg[298] = 0x62;
  msg[299] = 0x6F;
  msg[300] = 0x75;
  msg[301] = 0x6E;
  msg[302] = 0x64;
  msg[303] = 0x61;
  msg[304] = 0x72;
  msg[305] = 0x79;
  msg[306] = 0x3D;
  msg[307] = 0x75;
  msg[308] = 0x6E;
  msg[309] = 0x69;
  msg[310] = 0x71;
  msg[311] = 0x75;
  msg[312] = 0x65;
  msg[313] = 0x2D;
  msg[314] = 0x62;
  msg[315] = 0x6F;
  msg[316] = 0x75;
  msg[317] = 0x6E;
  msg[318] = 0x64;
  msg[319] = 0x61;
  msg[320] = 0x72;
  msg[321] = 0x79;
  msg[322] = 0x2D;
  msg[323] = 0x31;
  msg[324] = 0x0D;
  msg[325] = 0x0A;
  msg[326] = 0x4D;
  msg[327] = 0x49;
  msg[328] = 0x4D;
  msg[329] = 0x45;
  msg[330] = 0x2D;
  msg[331] = 0x56;
  msg[332] = 0x65;
  msg[333] = 0x72;
  msg[334] = 0x73;
  msg[335] = 0x69;
  msg[336] = 0x6F;
  msg[337] = 0x6E;
  msg[338] = 0x3A;
  msg[339] = 0x20;
  msg[340] = 0x31;
  msg[341] = 0x2E;
  msg[342] = 0x30;
  msg[343] = 0x0D;
  msg[344] = 0x0A;
  msg[345] = 0x0D;
  msg[346] = 0x0A;
  msg[347] = 0x2D;
  msg[348] = 0x2D;
  msg[349] = 0x75;
  msg[350] = 0x6E;
  msg[351] = 0x69;
  msg[352] = 0x71;
  msg[353] = 0x75;
  msg[354] = 0x65;
  msg[355] = 0x2D;
  msg[356] = 0x62;
  msg[357] = 0x6F;
  msg[358] = 0x75;
  msg[359] = 0x6E;
  msg[360] = 0x64;
  msg[361] = 0x61;
  msg[362] = 0x72;
  msg[363] = 0x79;
  msg[364] = 0x2D;
  msg[365] = 0x31;
  msg[366] = 0x0D;
  msg[367] = 0x0A;
  msg[368] = 0x43;
  msg[369] = 0x6F;
  msg[370] = 0x6E;
  msg[371] = 0x74;
  msg[372] = 0x65;
  msg[373] = 0x6E;
  msg[374] = 0x74;
  msg[375] = 0x2D;
  msg[376] = 0x54;
  msg[377] = 0x79;
  msg[378] = 0x70;
  msg[379] = 0x65;
  msg[380] = 0x3A;
  msg[381] = 0x20;
  msg[382] = 0x61;
  msg[383] = 0x70;
  msg[384] = 0x61;
  msg[385] = 0x74;
  msg[386] = 0x69;
  msg[387] = 0x6F;
  msg[388] = 0x6E;
  msg[389] = 0x2F;
  msg[390] = 0x53;
  msg[391] = 0x44;
  msg[392] = 0x50;
  msg[393] = 0x3B;
  msg[394] = 0x20;
  msg[395] = 0x63;
  msg[396] = 0x68;
  msg[397] = 0x61;
  msg[398] = 0x72;
  msg[399] = 0x73;
  msg[400] = 0x65;
  msg[401] = 0x74;
  msg[402] = 0x3D;
  msg[403] = 0x49;
  msg[404] = 0x53;
  msg[405] = 0x4F;
  msg[406] = 0x2D;
  msg[407] = 0x31;
  msg[408] = 0x30;
  msg[409] = 0x36;
  msg[410] = 0x34;
  msg[411] = 0x36;
  msg[412] = 0x38;
  msg[413] = 0x38;
  msg[414] = 0x35;
  msg[415] = 0x0D;
  msg[416] = 0x0A;
  msg[417] = 0x0D;
  msg[418] = 0x0A;
  msg[419] = 0x0A;
  msg[420] = 0x6F;
  msg[421] = 0x3D;
  msg[422] = 0x6A;
  msg[423] = 0x34;
  msg[424] = 0x36;
  msg[425] = 0x39;
  msg[426] = 0x36;
  msg[427] = 0x0D;
  msg[428] = 0x0A;
  msg[429] = 0x6D;
  msg[430] = 0x3D;
  msg[431] = 0x61;
  msg[432] = 0x75;
  msg[433] = 0x64;
  msg[434] = 0x69;
  msg[435] = 0x6F;
  msg[436] = 0x20;
  msg[437] = 0x39;
  msg[438] = 0x30;
  msg[439] = 0x39;
  msg[440] = 0x32;
  msg[441] = 0x20;
  msg[442] = 0x52;
  msg[443] = 0x54;
  msg[444] = 0x50;
  msg[445] = 0x2F;
  msg[446] = 0x41;
  msg[447] = 0x56;
  msg[448] = 0x50;
  msg[449] = 0x20;
  msg[450] = 0x30;
  msg[451] = 0x20;
  msg[452] = 0x33;
  msg[453] = 0x20;
  msg[454] = 0x34;
  msg[455] = 0x0D;
  msg[456] = 0x0A;
  msg[457] = 0x2D;
  msg[458] = 0x2D;
  msg[459] = 0x75;
  msg[460] = 0x6E;
  msg[461] = 0x69;
  msg[462] = 0x71;
  msg[463] = 0x75;
  msg[464] = 0x65;
  msg[465] = 0x2D;
  msg[466] = 0x62;
  msg[467] = 0x6F;
  msg[468] = 0x75;
  msg[469] = 0x6E;
  msg[470] = 0x64;
  msg[471] = 0x61;
  msg[472] = 0x72;
  msg[473] = 0x79;
  msg[474] = 0x2D;
  msg[475] = 0x31;
  msg[476] = 0x0D;
  msg[477] = 0x0A;
  msg[478] = 0x43;
  msg[479] = 0x6F;
  msg[480] = 0x6E;
  msg[481] = 0x74;
  msg[482] = 0x65;
  msg[483] = 0x6E;
  msg[484] = 0x74;
  msg[485] = 0x2D;
  msg[486] = 0x54;
  msg[487] = 0x79;
  msg[488] = 0x70;
  msg[489] = 0x65;
  msg[490] = 0x3A;
  msg[491] = 0x20;
  msg[492] = 0x61;
  msg[493] = 0x70;
  msg[494] = 0x70;
  msg[495] = 0x2F;
  msg[496] = 0x49;
  msg[497] = 0x53;
  msg[498] = 0xE8;
  msg[499] = 0x03;
  msg[500] = 0x00;
  msg[501] = 0x00;
  msg[502] = 0x76;
  msg[503] = 0x65;
  msg[504] = 0x72;
  msg[505] = 0x73;
  msg[506] = 0x69;
  msg[507] = 0x6F;
  msg[508] = 0x6E;
  msg[509] = 0x3D;
  msg[510] = 0x51;
  msg[511] = 0x78;
  msg[512] = 0x76;
  msg[513] = 0x33;
  msg[514] = 0xA3;
  msg[515] = 0xA3;
  msg[516] = 0xA3;
  msg[517] = 0xA3;
  msg[518] = 0xA3;
  msg[519] = 0xA3;
  msg[520] = 0xA3;
  msg[521] = 0xA3;
  msg[522] = 0xA3;
  msg[523] = 0xA3;
  msg[524] = 0x3B;
  msg[525] = 0x20;
  msg[526] = 0x62;
  msg[527] = 0x61;
  msg[528] = 0x73;
  msg[529] = 0x65;
  msg[530] = 0x3D;
  msg[531] = 0x65;
  msg[532] = 0x74;
  msg[533] = 0x73;
  msg[534] = 0x69;
  msg[535] = 0x31;
  msg[536] = 0x32;
  msg[537] = 0x31;
  msg[538] = 0x0D;
  msg[539] = 0x0A;
  msg[540] = 0x43;
  msg[541] = 0x6F;
  msg[542] = 0x6E;
  msg[543] = 0x74;
  msg[544] = 0x42;
  msg[545] = 0x6E;
  msg[546] = 0x74;
  msg[547] = 0x2D;
  msg[548] = 0x44;
  msg[549] = 0x69;
  msg[550] = 0x73;
  msg[551] = 0x70;
  msg[552] = 0x6F;
  msg[553] = 0x73;
  msg[554] = 0x69;
  msg[555] = 0x74;
  msg[556] = 0x69;
  msg[557] = 0x6F;
  msg[558] = 0x6E;
  msg[559] = 0x3A;
  msg[560] = 0x20;
  msg[561] = 0x73;
  msg[562] = 0x69;
  msg[563] = 0x67;
  msg[564] = 0x6E;
  msg[565] = 0x61;
  msg[566] = 0x6C;
  msg[567] = 0x3B;
  msg[568] = 0x20;
  msg[569] = 0x68;
  msg[570] = 0x61;
  msg[571] = 0x6E;
  msg[572] = 0x64;
  msg[573] = 0x6C;
  msg[574] = 0x69;
  msg[575] = 0x6E;
  msg[576] = 0x67;
  msg[577] = 0x3D;
  msg[578] = 0x6F;
  msg[579] = 0x70;
  msg[580] = 0x74;
  msg[581] = 0x69;
  msg[582] = 0x6F;
  msg[583] = 0x6E;
  msg[584] = 0x61;
  msg[585] = 0x6C;
  msg[586] = 0x0D;
  msg[587] = 0x0A;
  msg[588] = 0x0D;
  msg[589] = 0x0A;
  msg[590] = 0x30;
  msg[591] = 0x30;
  msg[592] = 0x30;
  msg[593] = 0x20;
  msg[594] = 0x30;
  msg[595] = 0x30;
  msg[596] = 0x20;
  msg[597] = 0x38;
  msg[598] = 0x39;
  msg[599] = 0x20;
  msg[600] = 0x38;
  msg[601] = 0x62;
  msg[602] = 0x0A;
  msg[603] = 0x30;
  msg[604] = 0x65;
  msg[605] = 0x20;
  msg[606] = 0x39;
  msg[607] = 0x35;
  msg[608] = 0x20;
  msg[609] = 0x31;
  msg[610] = 0x65;
  msg[611] = 0x20;
  msg[612] = 0x31;
  msg[613] = 0x65;
  msg[614] = 0x20;
  msg[615] = 0x31;
  msg[616] = 0x65;
  msg[617] = 0x20;
  msg[618] = 0x30;
  msg[619] = 0x36;
  msg[620] = 0x20;
  msg[621] = 0x32;
  msg[622] = 0x36;
  msg[623] = 0x20;
  msg[624] = 0x30;
  msg[625] = 0x35;
  msg[626] = 0x20;
  msg[627] = 0x30;
  msg[628] = 0x64;
  msg[629] = 0x20;
  msg[630] = 0x66;
  msg[631] = 0x35;
  msg[632] = 0x20;
  msg[633] = 0x30;
  msg[634] = 0x31;
  msg[635] = 0x20;
  msg[636] = 0x30;
  msg[637] = 0x36;
  msg[638] = 0x20;
  msg[639] = 0x31;
  msg[640] = 0x30;
  msg[641] = 0x20;
  msg[642] = 0x30;
  msg[643] = 0x34;
  msg[644] = 0x20;
  msg[645] = 0x30;
  msg[646] = 0x30;
  msg[647] = 0x0A;
  msg[648] = 0x2D;
  msg[649] = 0x2D;
  msg[650] = 0x75;
  msg[651] = 0x6E;
  msg[652] = 0x69;
  msg[653] = 0x71;
  msg[654] = 0x75;
  msg[655] = 0x65;
  msg[656] = 0x2D;
  msg[657] = 0x62;
  msg[658] = 0x6F;
  msg[659] = 0x75;
  msg[660] = 0x6E;
  msg[661] = 0x64;
  msg[662] = 0x61;
  msg[663] = 0x72;
  msg[664] = 0x79;
  msg[665] = 0x2D;
  msg[666] = 0x31;
  msg[667] = 0x2D;
  msg[668] = 0x2D;
  msg[669] = 0x0D;
  msg[670] = 0x0A;
  msg[671] =    0;
    
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

      if (sip == NULL) { fprintf (stdout, "LIBOSIP ERROR: sip is somehow NULL ...\n"); }
      if (msg == NULL) { fprintf (stdout, "LIBOSIP ERROR: msg is somehow NULL ...\n"); }

      if (osip_message_parse (sip, msg, len) != 0) {
        fprintf (stdout, "LIBOSIP ERROR: failed while parsing 1\n");
        osip_message_free (sip);
        return -1;
      }
      osip_message_free (sip);
    }

    osip_message_init (&sip);

    if (sip == NULL) { fprintf (stdout, "LIBOSIP ERROR: sip is somehow NULL ...\n"); }
    if (msg == NULL) { fprintf (stdout, "LIBOSIP ERROR: msg is somehow NULL ...\n"); }

    if (osip_message_parse (sip, msg, len) != 0) {
      fprintf (stdout, "LIBOSIP ERROR: failed while parsing 2\n");
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
