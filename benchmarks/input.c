/*********************/
/* FILENAME: input.c */
/*********************/

/*************************/
/* GENERAL INCLUDE FILES */
/*************************/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <time.h>

/**********************/
/* KLEE INCLUDE FILES */
/**********************/
#include "klee/klee.h"

void MyMalloc(char *p, int size);
void MyConstStringAssign(char *p,const char *q);
void MyStringAssignWithOffset(char *p, char *q,int offset1);
void MyWriteCharToStringAtOffset(char *p,int offset2,char c);
void MyWriteCharToStringAtOffset(char *p,int offset2,char c);
int  MyReadCharAtConstOffset_Is_EQ_ToConstChar(char *p,int offset,char c);
int  MyReadCharAtConstOffset_Is_NEQ_ToConstChar(char *p,int offset,char c);

void MyMyPrintOutput(char *s)
{
	fprintf(stdout,"%s\n",s);
}

/************/
/* main ... */
/************/
int main(int argc, char **argv)
{
	char *p1;

	p1 = malloc(30);

	if (p1[0]  == 'A') {
	if (p1[1]  == 'B') {
	if (p1[2]  == 'C') {
	if (p1[3]  == 'D') {
	if (p1[4]  == 'E') {
	if (p1[5]  == 'F') {
	if (p1[6]  == 'G') {
	if (p1[7]  == 'H') {
	if (p1[8]  == 'I') {
	if (p1[9]  == 'J') {
	if (p1[10] == 'K') {
	if (p1[11] == 'L') {
	if (p1[12] == 'M') {
	if (p1[13] == 'N') {
	if (p1[14] == 'O') {
	if (p1[15] == 'P') {
	if (p1[16] == 'Q') {
	if (p1[17] == '#') {
	if (p1[18] == '#') {
	if (p1[19] == '#') {
	if (p1[20] == 'R') {
	if (p1[21] == 'S') {
	if (p1[22] == 'T') {
	if (p1[23] == 'U') {
	if (p1[24] == 'V') {
	if (p1[25] == '@') {
	if (p1[26] == '@') {
	if (p1[27] == '@') {
	if (p1[28] == '@') {
	if (p1[29] == '$') {
		MyPrintOutput(">> GOT HERE !!!\n");
	}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}
}
