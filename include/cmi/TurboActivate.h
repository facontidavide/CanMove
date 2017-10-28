/* TurboActivate.h */
#pragma once

/* Define "uint32_t" for old versions of MS C++ (VS 2008 and below) */
#if !defined(_WIN32) || _MSC_VER >= 1600
    #include <stdint.h>
#else
    typedef unsigned int uint32_t;
#endif

#ifdef _WIN32
    /*
    These defines assume you're using the MSVC or Intel compilers on Windows.
    We do *not* recommend using the GNU / Mingw compilers on Windows.
    These compilers are buggy (among other problems). However, if you choose
    to use the GNU / Mingw on Windows then you'll have to modify these defines
    to properly support these buggy compilers.

    Note: using GNU compilers on Linux / Mac / Solaris / etc. is fine.
    These recommendations are for Windows only.
    */
    #include <windows.h>

    #ifdef TURBOACTIVATE_EXPORTS
        #ifdef TURBOACTIVATE_STATIC
            #define TURBOACTIVATE_API extern "C"
        #else
            #define TURBOACTIVATE_API extern "C" __declspec(dllexport)
        #endif
    #else
        #ifdef __cplusplus
            #ifdef TURBOACTIVATE_STATIC
                #define TURBOACTIVATE_API extern "C"
            #else
                #define TURBOACTIVATE_API extern "C" __declspec(dllimport)
            #endif
        #else
            #ifdef TURBOACTIVATE_STATIC
                #define TURBOACTIVATE_API
            #else
                #define TURBOACTIVATE_API __declspec(dllimport)
            #endif
        #endif
    #endif

    #if defined(USE_STDCALL_TA_DLL) && !defined(TURBOACTIVATE_STATIC)
        #define TA_CC __stdcall
    #else
        #define TA_CC __cdecl
    #endif
    #define TA_DEPRECATED(func) __declspec(deprecated) func

    typedef LPWSTR STRTYPE;
    typedef LPCWSTR STRCTYPE;
#else
    #if __GNUC__ >= 4
        #ifdef __cplusplus
            #define TURBOACTIVATE_API extern "C" __attribute__((visibility("default")))
        #else
            #define TURBOACTIVATE_API __attribute__((visibility("default")))
        #endif
    #else
        #ifdef __cplusplus
            #define TURBOACTIVATE_API extern "C"
        #else
            #define TURBOACTIVATE_API
        #endif
    #endif

    #define TA_CC
    #ifdef __GNUC__
        #define TA_DEPRECATED(func) func __attribute__ ((deprecated))
    #else
        #pragma message("WARNING: You need to implement DEPRECATED for this compiler")
        #define TA_DEPRECATED(func) func
    #endif

    typedef char* STRTYPE;
    typedef const char* STRCTYPE;
    typedef int32_t HRESULT;

    #ifdef __ANDROID__
        #include <jni.h>
    #endif
#endif

typedef struct _ACTIVATE_OPTIONS {
    /* The size, in bytes, of this structure. Set this value to
    the size of the ACTIVATE_OPTIONS structure. */
    uint32_t nLength;

    /* Extra data to pass to the LimeLM servers that will be visible for
    you to see and use. Maximum size is 255 UTF-8 characters. */
    STRTYPE sExtraData;
} ACTIVATE_OPTIONS, *PACTIVATE_OPTIONS;


/* Flags for the IsGeninueEx() function. */

/* If the user activated using offline activation
   (ActivateRequestToFile(), ActivateFromFile() ), then with this
   flag IsGenuineEx() will still try to validate with the LimeLM
   servers, however instead of returning TA_E_INET (when within the
   grace period) or TA_FAIL (when past the grace period) it will
   instead only return TA_OK (if IsActivated()).

   If you still want to get the TA_E_INET error code, without
   deactivating after the grace period has expired, then use
   this flag in tandem with TA_OFFLINE_SHOW_INET_ERR.

   If the user activated using online activation then this flag
   is ignored.
*/
#define TA_SKIP_OFFLINE ((uint32_t)1)

/* If the user activated using offline activation, and you're
   using this flag in tandem with TA_SKIP_OFFLINE, then IsGenuineEx()
   will return TA_E_INET on internet failure instead of TA_OK.

   If the user activated using online activation then this flag
   is ignored.
*/
#define TA_OFFLINE_SHOW_INET_ERR ((uint32_t)2)

typedef struct _GENUINE_OPTIONS {
    /* The size, in bytes, of this structure. Set this value to
    the size of the GENUINE_OPTIONS structure. */
    uint32_t nLength;

    /* Flags to pass to IsGenuineEx() to control its behavior. */
    uint32_t flags;

    /* How often to contact the LimeLM servers for validation. 90 days recommended. */
    uint32_t nDaysBetweenChecks;

    /* If the call fails because of an internet error,
    how long, in days, should the grace period last (before
    returning deactivating and returning TA_FAIL).

    14 days is recommended. */
    uint32_t nGraceDaysOnInetErr;
} GENUINE_OPTIONS, *PGENUINE_OPTIONS;


/* Flags for the UseTrial() and CheckAndSavePKey() functions. */
#define TA_SYSTEM ((uint32_t)1)
#define TA_USER ((uint32_t)2)

// Use the TA_DISALLOW_VM in UseTrial() to disallow trials in virtual machines.
// If you use this flag in UseTrial() and the customer's machine is a Virtual
// Machine, then UseTrial() will return TA_E_IN_VM.
#define TA_DISALLOW_VM ((uint32_t)4)

/*
TurboActivate 2.x Compatability Notes
======================================

With TurboActivate 3+ we've broken backwards compatability
with TurboActivate 2.x by adding a "flags" argument to a
number of functions. To be compatible with TurboActivate 2.x versions of the
calls, then simply use the "TA_USER" flag.

For instance, "CheckAndSavePKey(productKey, TA_USER);" is equivalent to
the "CheckAndSavePKey(productKey);" in 2.x. "UseTrial(TA_USER);"
is equivalent to the "UseTrial(YOUR_VERSION_GUID);" in 2.x.



When to use TA_USER or TA_SYSTEM
=======================================

If your app is running as a regular user then we recommend always using
TA_USER for all the functions. However, if you're running a system service
(i.e. a Windows Service, etc.) then you should use TA_SYSTEM.

*/



/*
   Activates the product on this computer. You must call CheckAndSavePKey()
   with a valid product key or have used the TurboActivate wizard sometime
   before calling this function.


   Returns: TA_OK on success. Handle all other return codes as failures.

   Possible return codes: TA_OK, TA_FAIL, TA_E_PKEY, TA_E_INET, TA_E_INUSE,
                          TA_E_REVOKED, TA_E_PDETS, TA_E_COM, TA_E_EXPIRED,
                          TA_E_IN_VM, TA_E_KEY_FOR_TURBOFLOAT, TA_E_ANDROID_NOT_INIT
*/
TURBOACTIVATE_API HRESULT TA_CC Activate();


/*
   Activates the product on this computer. You must call CheckAndSavePKey()
   with a valid product key or have used the TurboActivate wizard sometime
   before calling this function.


   Returns: TA_OK on success. Handle all other return codes as failures.

   Possible return codes: TA_OK, TA_FAIL, TA_E_PKEY, TA_E_INET, TA_E_INUSE,
                          TA_E_REVOKED, TA_E_PDETS, TA_E_COM, TA_E_EXPIRED,
                          TA_E_EDATA_LONG, TA_E_INVALID_ARGS, TA_E_IN_VM,
                          TA_E_KEY_FOR_TURBOFLOAT, TA_E_ANDROID_NOT_INIT
*/
TURBOACTIVATE_API HRESULT TA_CC ActivateEx(PACTIVATE_OPTIONS options);


/*
   Get the "activation request" file for offline activation. You must call CheckAndSavePKey()
   with a valid product key or have used the TurboActivate wizard sometime
   before calling this function.


   Returns: TA_OK on success. Handle all other return codes as failures.

   Possible return codes: TA_OK, TA_FAIL, TA_E_PKEY, TA_E_PDETS, TA_E_COM,
                          TA_E_ANDROID_NOT_INIT
*/
TURBOACTIVATE_API HRESULT TA_CC ActivationRequestToFile(STRCTYPE filename);


/*
   Get the "activation request" file for offline activation. You must call CheckAndSavePKey()
   with a valid product key or have used the TurboActivate wizard sometime
   before calling this function.


   Returns: TA_OK on success. Handle all other return codes as failures.

   Possible return codes: TA_OK, TA_FAIL, TA_E_PKEY, TA_E_PDETS, TA_E_COM,
                          TA_E_EDATA_LONG, TA_E_INVALID_ARGS, TA_E_ANDROID_NOT_INIT
*/
TURBOACTIVATE_API HRESULT TA_CC ActivationRequestToFileEx(STRCTYPE filename, PACTIVATE_OPTIONS options);


/*
   Activate from the "activation response" file for offline activation.


   Returns: TA_OK on success. Handle all other return codes as failures.

   Possible return codes: TA_OK, TA_FAIL, TA_E_PKEY, TA_E_PDETS, TA_E_COM,
                          TA_E_EXPIRED, TA_E_IN_VM, TA_E_ANDROID_NOT_INIT
*/
TURBOACTIVATE_API HRESULT TA_CC ActivateFromFile(STRCTYPE filename);


/*
   Blacklists keys so they are no longer valid. Use "BlackListKeys" only if
   you're using the "Serial-only plan" in LimeLM. Otherwise revoke keys.


   Returns: TA_OK on success. Handle all other return codes as failures.

   Possible return codes: TA_OK, TA_FAIL
*/
TURBOACTIVATE_API HRESULT TA_CC BlackListKeys(STRCTYPE * keys, uint32_t numKeys);


/*
   Checks and saves the product key.


   Note: If you pass in the TA_SYSTEM flag and you don't have "admin" or "elevated"
   permission then the call will fail.

   If you call this function once in the past with the flag TA_SYSTEM and the calling
   process was an admin process then subsequent calls with the TA_SYSTEM flag will
   succeed even if the calling process is *not* admin/elevated.

   If you want to take advantage of this behavior from an admin process
   (e.g. an installer) but the user hasn't entered a product key then you can
   call this function with a null string:

            CheckAndSavePKey(0, TA_SYSTEM);

   This will set everything up so that subsequent calls with the TA_SYSTEM flag will
   succeed even if from non-admin processes.


   Returns: TA_OK on success. Handle all other return codes as failures.

   Possible return codes: TA_OK, TA_FAIL, TA_E_PDETS, TA_E_PERMISSION,
                          TA_E_INVALID_FLAGS
*/
TURBOACTIVATE_API HRESULT TA_CC CheckAndSavePKey(STRCTYPE productKey, uint32_t flags);


/*
   Deactivates the product on this computer. Set erasePkey to 1 to erase the stored
   product key, 0 to keep the product key around. If you're using deactivate to let
   a user move between computers it's almost always best to *not* erase the product
   key. This way you can just use Activate() when the user wants to reactivate
   instead of forcing the user to re-enter their product key over-and-over again.


   Returns: TA_OK on success. Handle all other return codes as failures.

   Possible return codes: TA_OK, TA_FAIL, TA_E_PKEY, TA_E_INET, TA_E_PDETS,
                          TA_E_COM, TA_E_ANDROID_NOT_INIT
*/
TURBOACTIVATE_API HRESULT TA_CC Deactivate(char erasePkey);


/*
   Get the "deactivation request" file for offline deactivation.


   Returns: TA_OK on success. Handle all other return codes as failures.

   Possible return codes: TA_OK, TA_FAIL, TA_E_PKEY, TA_E_PDETS, TA_E_COM,
                          TA_E_ANDROID_NOT_INIT
*/
TURBOACTIVATE_API HRESULT TA_CC DeactivationRequestToFile(STRCTYPE filename, char erasePkey);


/*
   Gets the extra data you passed in using ActivateEx().


   lpValueStr
   [out] Pointer to a buffer that receives the value of the string.

   cchValue
   [in] Size (in wide characters on Windows or characters in Unix) of the buffer
        pointed to by the lpValueStr parameter.

   If 'cchValue' is zero, the function returns the required buffer size (in wide characters
   on Windows, characters on Unix) and makes no use of the lpValueStr buffer.


   Returns: TA_OK on success. Handle all other return codes as failures.

   Possible return codes: TA_OK, TA_FAIL, TA_E_PDETS
*/
TURBOACTIVATE_API HRESULT TA_CC GetExtraData(STRTYPE lpValueStr, int cchValue);


/*
   Gets the value of a feature.


   lpValueStr
   [out] Pointer to a buffer that receives the value of the string.

   cchValue
   [in] Size (in wide characters on Windows or characters in Unix) of the buffer
        pointed to by the lpValueStr parameter.

   If 'cchValue' is zero, the function returns the required buffer size (in wide characters
   on Windows, characters on Unix) and makes no use of the lpValueStr buffer.


   Returns: TA_OK on success. Handle all other return codes as failures.

   Possible return codes: TA_OK, TA_FAIL, TA_E_PDETS, TA_E_INSUFFICIENT_BUFFER
*/
TURBOACTIVATE_API HRESULT TA_CC GetFeatureValue(STRCTYPE featureName, STRTYPE lpValueStr, int cchValue);


/*
   Gets the stored product key. NOTE: if you want to check if a product key is valid
   simply call IsProductKeyValid().


   lpValueStr
   [out] Pointer to a buffer that receives the value of the string.

   cchValue
   [in] Size (in wide characters on Windows or characters in Unix) of the buffer
        pointed to by the lpValueStr parameter.

   If 'cchValue' is zero, the function returns the required buffer size (in wide characters
   on Windows, characters on Unix) and makes no use of the lpValueStr buffer.


   Returns: TA_OK on success. Handle all other return codes as failures.

   Possible return codes: TA_OK, TA_FAIL, TA_E_PKEY, TA_E_PDETS
*/
TURBOACTIVATE_API HRESULT TA_CC GetPKey(STRTYPE lpValueStr, int cchValue);


/*
   Checks whether the computer has been activated.


   Returns: TA_OK on success. Handle all other return codes as failures.

   Possible return codes: TA_OK, TA_FAIL, TA_E_GUID, TA_E_PDETS, TA_E_COM,
                          TA_E_IN_VM, TA_E_ANDROID_NOT_INIT
*/
TURBOACTIVATE_API HRESULT TA_CC IsActivated(STRCTYPE versionGUID);


/*
   Checks whether the computer is genuinely activated by verifying with the LimeLM servers.
   If reactivation is needed then it will do this as well.


   Returns: TA_OK or TA_E_FEATURES_CHANGED on success. Handle all other return codes as failures.

   Possible return codes: TA_OK, TA_FAIL, TA_E_ACTIVATE, TA_E_INET, TA_E_GUID
                          TA_E_PDETS, TA_E_COM, TA_E_EXPIRED
                          TA_E_REVOKED, TA_E_IN_VM, TA_E_FEATURES_CHANGED,
                          TA_E_ANDROID_NOT_INIT
*/
TURBOACTIVATE_API HRESULT TA_CC IsGenuine(STRCTYPE versionGUID);


/*
   Checks whether the computer is genuinely activated by verifying with the LimeLM servers
   after a certain number of days you specify.

   This is meant as a replacement of both IsActivated() and IsGenuine(). Call this at the
   top of your program and let IsGenuineEx() handle all the details.

   This differs with IsGenuine() in 3 major ways:

        1. You can specify how often to verify with the LimeLM servers and it handles
           all the date tracking behind the scenes.


        2. IsGenuineEx() prevents your app from hammering the end-user's network after
           and TA_E_INET error return code by not checking with the LimeLM servers until
           at least 5 hours has passed. If you call IsGenuineEx() after a TA_E_INET return
           and before 5 hours has elapsed then this function will return TA_E_INET_DELAYED.

           (If you give the user the option to recheck with LimeLM, e.g. via a button
           like "Retry now" then call IsGenuine() to immediately retry without waiting 5 hours).


        3. If a TA_E_INET error is being returned, and the grace period has expired,
           then IsGenuineEx() will return TA_FAIL. IsGenuineEx() will continue to try
           contacting the LimeLM servers on subsequent calls (5 hours apart), but you
           should treat the TA_FAIL as a hard failure.


   Returns: TA_OK or TA_E_FEATURES_CHANGED on success. Handle TA_E_INET and TA_E_INET_DELAYED as warnings that
            you should let the end user know about.

            Handle all other return codes as failures.

   Possible return codes: TA_OK, TA_FAIL, TA_E_ACTIVATE, TA_E_INET, TA_E_GUID
                          TA_E_PDETS, TA_E_COM, TA_E_EXPIRED, TA_E_REVOKED,
                          TA_E_INVALID_ARGS, TA_E_INVALID_FLAGS, TA_E_IN_VM,
                          TA_E_INET_DELAYED, TA_E_FEATURES_CHANGED,
                          TA_E_ANDROID_NOT_INIT
*/
TURBOACTIVATE_API HRESULT TA_CC IsGenuineEx(STRCTYPE versionGUID, PGENUINE_OPTIONS options);


/*
   Checks if the product key installed for this product is valid. This does NOT check if
   the product key is activated or genuine. Use IsActivated() and IsGenuine() instead.


   Returns: TA_OK on success. Handle all other return codes as failures.

   Possible return codes: TA_OK, TA_FAIL, TA_E_GUID, TA_E_PDETS
*/
TURBOACTIVATE_API HRESULT TA_CC IsProductKeyValid(STRCTYPE versionGUID);


/*
   Sets the custom proxy to be used by functions that connect to the internet.


   Proxy in the form: http://username:password@host:port/

   Example 1 (just a host): http://127.0.0.1/
   Example 2 (host and port): http://127.0.0.1:8080/
   Example 3 (all 3): http://user:pass@127.0.0.1:8080/

   If the port is not specified, TurboActivate will default to using port 1080 for proxies.


   Returns: TA_OK on success. Handle all other return codes as failures.

   Possible return codes: TA_OK, TA_FAIL
*/
TURBOACTIVATE_API HRESULT TA_CC SetCustomProxy(STRCTYPE proxy);


/*
   Get the number of trial days remaining.
   0 days if the trial has expired or has been tampered with
   (1 day means *at most* 1 day, that is it could be 30 seconds)

   You must call UseTrial() at least once in the past before calling this function.


   Returns: TA_OK on success. Handle all other return codes as failures.

   Possible return codes: TA_OK, TA_FAIL, TA_E_GUID, TA_E_PDETS
*/
TURBOACTIVATE_API HRESULT TA_CC TrialDaysRemaining(STRCTYPE versionGUID, uint32_t * DaysRemaining);


/*
   Begins the trial the first time it's called. Calling it again will validate the trial
   data hasn't been tampered with.


   Note: If you pass in the TA_SYSTEM flag and you don't have "admin" or "elevated"
   permission then the call will fail.

   If you call this function once in the past with the flag TA_SYSTEM and the calling
   process was an admin process then subsequent calls with the TA_SYSTEM flag will
   succeed even if the calling process is *not* admin/elevated.


   Returns: TA_OK on success. Handle all other return codes as failures.

   Possible return codes: TA_OK, TA_FAIL, TA_E_PDETS, TA_E_PERMISSION, TA_E_COM
                          TA_E_INVALID_FLAGS, TA_E_IN_VM, TA_E_ANDROID_NOT_INIT
*/
TURBOACTIVATE_API HRESULT TA_CC UseTrial(uint32_t flags);


/*
   Extends the trial using a trial extension created in LimeLM.


   Returns: TA_OK on success. Handle all other return codes as failures.

   Possible return codes: TA_OK, TA_FAIL, TA_E_INET, TA_E_PDETS, TA_E_TRIAL_EUSED
                          TA_E_TRIAL_EEXP
*/
TURBOACTIVATE_API HRESULT TA_CC ExtendTrial(STRCTYPE trialExtension);


/*
   Loads the "TurboActivate.dat" file from a path rather than loading it
   from the same dir as TurboActivate.dll on Windows or the app that
   uses libTurboActivate.dylib / libTurboActivate.so on Mac / Linux.

   You can load multiple *.dat files for licensing multiple products within
   one process. You can switch between the products by using the
   SetCurrentProduct() with the VersionGUID of the product you want to use
   licensing for.


   Returns: TA_OK on success. Handle all other return codes as failures.

   Possible return codes: TA_OK, TA_FAIL, TA_E_PDETS


   Note: Once the product details file has loaded, all subsequent calls to "PDetsFromPath"
   will return TA_FAIL
*/
TURBOACTIVATE_API HRESULT TA_CC PDetsFromPath(STRCTYPE filename);


/*
   This functions allows you to use licensing for multiple products within
   the same running process. First load all the TurboActivate.dat files
   for all your products using the PDetsFromPath() function. Then, to use
   any of the licensing functions for a product you need to for any
   particular product, you must first call SetCurrentProduct() to "switch"
   to the product.


   Returns: TA_OK on success. Handle all other return codes as failures.

   Possible return codes: TA_OK, TA_FAIL
*/
TURBOACTIVATE_API HRESULT TA_CC SetCurrentProduct(STRCTYPE versionGUID);


/*
   Gets the "current product" previously set by SetCurrentProduct().


   lpValueStr
   [out] Pointer to a buffer that receives the value of the string.

   cchValue
   [in] Size (in wide characters on Windows or characters in Unix) of the buffer
        pointed to by the lpValueStr parameter.

   If 'cchValue' is zero, the function returns the required buffer size (in wide characters
   on Windows, characters on Unix) and makes no use of the lpValueStr buffer.


   Returns: TA_OK on success. Handle all other return codes as failures.

   Possible return codes: TA_OK, TA_FAIL, TA_E_INSUFFICIENT_BUFFER
*/
TURBOACTIVATE_API HRESULT TA_CC GetCurrentProduct(STRTYPE lpValueStr, int cchValue);


/*
   This function allows you to set a custom folder to store the activation
   data files. For normal use we do not recommend you use this function.

   Only use this function if you absolutely must store data into a separate
   folder. For example if your application runs on a USB drive and can't write
   any files to the main disk, then you can use this function to save the activation
   data files to a directory on the USB disk.

   If you are using this function (which we only recommend for very special use-cases)
   then you must call this function on every start of your program at the very top of
   your app before any other functions are called.

   The directory you pass in must already exist. And the process using TurboActivate
   must have permission to create, write, and delete files in that directory.


   Returns: TA_OK on success. Handle all other return codes as failures.

   Possible return codes: TA_OK, TA_FAIL, TA_E_PDETS
*/
TURBOACTIVATE_API HRESULT TA_CC SetCustomActDataPath(STRCTYPE directory);



/* Flags for the IsDateValid() function. */

/* With this flag, IsDateValid() will return TA_OK if the date has not
   expired and the system dates have not been tampered with. Otherwise, 
   TA_FAIL will be returned.
*/ 
#define TA_HAS_NOT_EXPIRED ((uint32_t)1)

/*
   Checks if the string in the form "YYYY-MM-DD HH:mm:ss" is a valid
   date/time. The date must be in UTC time and "24-hour" format. If your
   date is in some other time format first convert it to UTC time before
   passing it into this function.

   Returns: TA_OK on success. Handle all other return codes as failures.

   Possible return codes: TA_OK, TA_FAIL, TA_E_PDETS, TA_E_INVALID_FLAGS
*/
TURBOACTIVATE_API HRESULT TA_CC IsDateValid(STRCTYPE date_time, uint32_t flags);


#ifdef __ANDROID__
/*
   If you're using TurboActivate from Android, and you're not using the JNI interface,
   then you must first initialize TurboActivate to use JNIEnv from your app. For purely
   native apps you can get this using the "android_main" function.

   Returns: TA_OK on success. Handle all other return codes as failures.

   Possible return codes: TA_OK, TA_FAIL
*/
TURBOACTIVATE_API HRESULT TA_CC TA_InitAndroid(JNIEnv* env);
#endif

/*
   Get the number of days left in the activation grace period.
   Returns: TA_OK on success. Handle all other return codes as failures.
   Possible return codes: TA_OK, TA_FAIL, TA_E_GUID, TA_E_PDETS

   This function is obsolete and will be removed in TurboActivate 4.0; use the UseTrial(),
   TrialDaysRemaining(), and ExtendTrial() functions instead.
*/
//TA_DEPRECATED(TURBOACTIVATE_API HRESULT TA_CC GracePeriodDaysRemaining(STRCTYPE versionGUID, uint32_t * DaysRemaining));



/*
 General Success and Failure return codes.
*/
#define TA_OK               ((HRESULT)0L)
#define TA_FAIL             ((HRESULT)1L)


/*
 MessageId: TA_E_PKEY

 MessageText:

 Invalid product key
*/
#define TA_E_PKEY                 ((HRESULT)0x00000002L)


/*
 MessageId: TA_E_ACTIVATE

 MessageText:

 The product needs to be activated.
*/
#define TA_E_ACTIVATE             ((HRESULT)0x00000003L)


/*
 MessageId: TA_E_INET

 MessageText:

 Connection to the server failed.
*/
#define TA_E_INET                 ((HRESULT)0x00000004L)


/*
 MessageId: TA_E_INUSE

 MessageText:

 The product key has already been activated with the maximum number of computers.
*/
#define TA_E_INUSE                ((HRESULT)0x00000005L)


/*
 MessageId: TA_E_REVOKED

 MessageText:

 The product key has been revoked.
*/
#define TA_E_REVOKED              ((HRESULT)0x00000006L)


/*
 MessageId: TA_E_GUID

 MessageText:

 The version GUID doesn't match that of the product details file.
*/
#define TA_E_GUID                 ((HRESULT)0x00000007L)


/*
 MessageId: TA_E_PDETS

 MessageText:

 The product details file "TurboActivate.dat" failed to load.
*/
#define TA_E_PDETS                ((HRESULT)0x00000008L)


/*
 MessageId: TA_E_TRIAL

 MessageText:

 The trial data has been corrupted, using the oldest date possible.
*/
#define TA_E_TRIAL                ((HRESULT)0x00000009L)


/*
 MessageId: TA_E_TRIAL_EUSED

 MessageText:

 The trial extension has already been used.
*/
#define TA_E_TRIAL_EUSED          ((HRESULT)0x0000000CL)


/*
 MessageId: TA_E_TRIAL_EEXP

 MessageText:

 The trial extension has expired.
*/
#define TA_E_TRIAL_EEXP           ((HRESULT)0x0000000DL)


/*
 MessageId: TA_E_EXPIRED

 MessageText:

 The activation has expired or the system time has been tampered
 with. Ensure your time, timezone, and date settings are correct.
*/
#define TA_E_EXPIRED              ((HRESULT)0x0000000DL)


/*
 MessageId: TA_E_REACTIVATE

 MessageText:

 The product key needs to be reactivated because your hardware
 has changed or the features data has changed. To reactivate simply
 call the "Activate()" function.

 Note: As of TurboActivate 3.3 this is no longer used.
*/
#define TA_E_REACTIVATE           ((HRESULT)0x0000000AL)


/*
 MessageId: TA_E_COM

 MessageText:

 The hardware id couldn't be generated due to an error in the COM setup.
 Re-enable Windows Management Instrumentation (WMI) in your group policy
 editor or reset the local group policy to the default values. Contact
 your system admin for more information.

 This error is Windows only.

 This error can also be caused by the user (or another program) disabling
 the "Windows Management Instrumentation" service. Make sure the "Startup type"
 is set to Automatic and then start the service.


 To further debug WMI problems open the "Computer Management" (compmgmt.msc),
 expand the "Services and Applications", right click "WMI Control" click
 "Properties" and view the status of the WMI.
*/
#define TA_E_COM                  ((HRESULT)0x0000000BL)


/*
 MessageId: TA_E_INSUFFICIENT_BUFFER

 MessageText:

 The the buffer size was too small. Create a larger buffer and try again.
*/
#define TA_E_INSUFFICIENT_BUFFER  ((HRESULT)0x0000000EL)


/*
 MessageId: TA_E_PERMISSION

 MessageText:

 Insufficient system permission. Either start your process as an
 admin / elevated user or call the function again with the
 TA_USER flag instead of the TA_SYSTEM flag.
*/
#define TA_E_PERMISSION           ((HRESULT)0x0000000FL)


/*
 MessageId: TA_E_INVALID_FLAGS

 MessageText:

 The flags you passed to CheckAndSavePKey(...) or UseTrial(...)
 were invalid (or missing). Flags like "TA_SYSTEM" and "TA_USER"
 are mutually exclusive -- you can only use one or the other.
*/
#define TA_E_INVALID_FLAGS        ((HRESULT)0x00000010L)


/*
 MessageId: TA_E_IN_VM

 MessageText:

 The function failed because this instance of your program
 if running inside a virtual machine / hypervisor and you've
 prevented the function from running inside a VM.
*/
#define TA_E_IN_VM                ((HRESULT)0x00000011L)


/*
 MessageId: TA_E_EDATA_LONG

 MessageText:

 The "extra data" was too long. You're limited to 255 UTF-8 characters.
 Or, on Windows, a Unicode string that will convert into 255 UTF-8
 characters or less.
*/
#define TA_E_EDATA_LONG           ((HRESULT)0x00000012L)


/*
 MessageId: TA_E_INVALID_ARGS

 MessageText:

 The arguments passed to the function are invalid. Double check your logic.
*/
#define TA_E_INVALID_ARGS         ((HRESULT)0x00000013L)


/*
 MessageId: TA_E_KEY_FOR_TURBOFLOAT

 MessageText:

 The product key used is for TurboFloat, not TurboActivate.
*/
#define TA_E_KEY_FOR_TURBOFLOAT   ((HRESULT)0x00000014L)


/*
 MessageId: TA_E_INET_DELAYED

 MessageText:

 IsGenuineEx() previously had a TA_E_INET error, and instead
 of hammering the end-user's network, IsGenuineEx() is waiting
 5 hours before rechecking on the network.
*/
#define TA_E_INET_DELAYED         ((HRESULT)0x00000015L)


/*
 MessageId: TA_E_FEATURES_CHANGED

 MessageText:

 If IsGenuine() or IsGenuineEx() reactivated and the features
 have changed, then this will be the return code. Treat this
 as a success.
*/
#define TA_E_FEATURES_CHANGED     ((HRESULT)0x00000016L)


/*
 MessageId: TA_E_ANDROID_NOT_INIT

 MessageText:

 You didn't call the TA_InitAndroid() function before using
 the TurboActivate functions. This only applies to using TurboActivate
 on Android -- specifically using TA from native code. You don't
 need to call this from Java.
*/
#define TA_E_ANDROID_NOT_INIT     ((HRESULT)0x00000017L)
