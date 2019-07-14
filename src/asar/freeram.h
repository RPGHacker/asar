// From https://github.com/randomdude999/smw-freeram-manager
#ifdef __cplusplus
extern "C" {
#endif

typedef void* freeram_handle;

// (un)load the library. returns 1 on success, 0 on failure
// it's usually not necessary to manually unload the library before application exit
int freeram_loadlib();
int freeram_unloadlib();
// Open the ram file for the specified ROM. Pass in the name of the ROM, ending with .smc/.sfc.
// returns NULL on failure. If it fails, it sets *error to a string describing what went wrong.
// You need to free that string yourself.
freeram_handle freeram_open(const char* romname, char** error);
// Close the specified handle, writing out the file again. returns 1 on success, 0 on failure. on failure, check errno.
// You need to call this after you're done with the ROM.
int freeram_close(freeram_handle handle);
// Get and claim a freeram address. returns a negative number on failure, or snes address on success
// -1 - no matching freeram found
// -2 - library not loaded
// -3 - invalid identifier
// -4 - invalid flags
// -5 - claim with the same identifier but different size/flags exists
int freeram_get_ram(freeram_handle handle, int size, const char* identifier, const char* flags);
// Unclaim the freeram with the specified identifier. Returns 0 on success.
// returns -1 if that address wasn't claimed
//         -2 if the library isn't loaded
int freeram_unclaim_ram(freeram_handle handle, const char* identifier);

#ifdef __cplusplus
}
#endif