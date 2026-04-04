/******************************************************************************
 *                *          ***                    ***
 *              ***          ***                    ***
 * ***  ****  **********     ***        *****       ***  ****          *****
 * *********  **********     ***      *********     ************     *********
 * ****         ***          ***              ***   ***       ****   ***
 * ***          ***  ******  ***      ***********   ***        ****   *****
 * ***          ***  ******  ***    *************   ***        ****      *****
 * ***          ****         ****   ***       ***   ***       ****          ***
 * ***           *******      ***** **************  *************    *********
 * ***             *****        ***   *******   **  **  ******         *****
 *                           t h e  r e a l t i m e  t a r g e t  e x p e r t s
 *
 * http://www.rt-labs.com
 * Copyright (C) 2009. rt-labs AB, Sweden. All rights reserved.
 *------------------------------------------------------------------------------
 * $Id: osal_defs.h 472 2013-04-08 11:39:51Z rtlaka $
 *------------------------------------------------------------------------------
 */

#ifndef _osal_defs_
#define _osal_defs_
	
//#define EC_DEBUG

#include "fsl_debug_console.h"

#ifdef EC_DEBUG
#define EC_PRINT printf
#else
#define EC_PRINT(...)
#endif

#ifndef PACKED
#define PACKED_BEGIN
#define PACKED  __attribute__((__packed__))
#define PACKED_END
#endif

#define OSAL_THREAD_HANDLE void *
#define OSAL_THREAD_FUNC void
#define OSAL_THREAD_FUNC_RT void

#endif
