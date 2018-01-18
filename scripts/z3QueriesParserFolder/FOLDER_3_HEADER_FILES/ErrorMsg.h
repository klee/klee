#ifndef __ERRORMSG_H__
#define __ERRORMSG_H__

/*************************/
/* FILE NAME: ErrorMsg.h */
/*************************/

/*************************/
/* PROJECT INCLUDE FILES */
/*************************/
#include "util.h"

/**********************/
/* EXTERNAL VARIABLES */
/**********************/
extern int ErrorMsg_tokPos;

/***********/
/* NEWLINE */
/***********/
void ErrorMsg_Newline(void);

/************************/
/* LOG FUNCTION :: Open */
/************************/
void ErrorMsg_OpenLog();

/*************************/
/* LOG FUNCTION :: close */
/*************************/
void ErrorMsg_CloseLog();

/****************/
/* LOG FILENAME */
/****************/
void ErrorMsg_Set_Log_Filename(string log_Filename);

/*********/
/* RESET */
/*********/
void ErrorMsg_Reset(string filename);

#endif
