/*
 * rivetCrypt.c - Commands to do encryption and decryption.
 */

#include <tcl.h>
#include "rivet.h"
#include <crypt.h>
#include <unistd.h>

#define MODE_DECRYPT 0
#define MODE_ENCRYPT 1

/* encrypt/decrypt string in place using key,
 * mode = 1 to encrypt and 0 to decrypt
 */
static void
Rivet_Crypt(char *string, const char *key, long offset, int mode)
{
    const char *kp = key;

    offset = offset % strlen(key);
    while (offset--) kp++;

    /* printf("encrypt '%s' with key '%s', mode %d\n",string,key,mode); */

    while (*string != '\0')
    {
	if (*string >= 32 && *string <= 126)
	{
	    if (mode)
		*string = (((*string - 32) + (*kp - 32)) % 94) + 32;
	    else
		*string = (((*string - 32) - (*kp - 32) + 94) % 94) + 32;
	}

	string++;
	kp++;
	if (*kp == '\0') {
	    kp = key;
	}
    }
}

TCL_CMD_HEADER( Rivet_EncryptCmd )
{
    char *data, *key;
    char *resultBuffer;
    int dataLen;
    int keyIndex;

    if( objc < 3 ) {
	Tcl_WrongNumArgs( interp, 1, objv, "data key" );
        return TCL_ERROR;
    }

    data = Tcl_GetStringFromObj( objv[1], &dataLen );

    resultBuffer = (char *)Tcl_Alloc( (unsigned)dataLen + 1 );
    strcpy ( resultBuffer, data );

    for( keyIndex = 2; keyIndex < objc; keyIndex++ )
    {
	key = Tcl_GetStringFromObj( objv[keyIndex], NULL );
        Rivet_Crypt( resultBuffer, key, 0L, MODE_ENCRYPT );
    }

    Tcl_SetObjResult( interp, Tcl_NewStringObj( resultBuffer, -1 ) );
    return TCL_OK;
}

TCL_CMD_HEADER( Rivet_DecryptCmd )
{
    char *data, *key;
    char *resultBuffer;
    int dataLen;
    int keyIndex;

    if( objc < 3 ) {
	Tcl_WrongNumArgs( interp, 1, objv, "data key" );
        return TCL_ERROR;
    }

    data = Tcl_GetStringFromObj( objv[1], &dataLen );

    resultBuffer = (char *)Tcl_Alloc( (unsigned)dataLen + 1 );
    strcpy ( resultBuffer, data );

    for( keyIndex = 2; keyIndex < objc; keyIndex++ )
    {
	key = Tcl_GetStringFromObj( objv[keyIndex], NULL );
        Rivet_Crypt( resultBuffer, key, 0L, MODE_DECRYPT );
    }

    Tcl_SetObjResult( interp, Tcl_NewStringObj( resultBuffer, -1 ) );
    return TCL_OK;
}

TCL_CMD_HEADER( Rivet_CryptCmd )
{
    char *key, *salt;
    char *resultBuffer;

    if( objc != 3 ) {
	Tcl_WrongNumArgs( interp, 1, objv, "key salt" );
        return TCL_ERROR;
    }

    key = Tcl_GetStringFromObj( objv[1], NULL );
    salt = Tcl_GetStringFromObj( objv[2], NULL );

    resultBuffer = crypt((const char *)key, (const char *)salt);

    if( resultBuffer == NULL ) {
	Tcl_AppendResult (interp,
		"crypt function failed: ",
		Tcl_GetStringFromObj(objv[1], NULL),
		(char *)NULL );
	return TCL_ERROR;
    }
    Tcl_SetObjResult( interp, Tcl_NewStringObj( resultBuffer, -1 ) );
    return TCL_OK;
}

int
Rivet_InitCrypt( Tcl_Interp *interp )
{
    TCL_OBJ_CMD( "encrypt", Rivet_EncryptCmd );
    TCL_OBJ_CMD( "decrypt", Rivet_DecryptCmd );
    TCL_OBJ_CMD( "crypt", Rivet_CryptCmd );

    return TCL_OK;
}
