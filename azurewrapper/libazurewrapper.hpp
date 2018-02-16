#ifndef LIB_AZURE_HPP_WRAPPER
#define LIB_AZURE_HPP_WRAPPER
/**
 * @file
 * @version 1.0
 *
 * @section LICENSE
 *
 * This software is open source.
 *
 * Renaissance Computing Institute,
 * (A Joint Institute between the University of North Carolina at Chapel Hill,
 * North Carolina State University, and Duke University)
 * http://www.renci.org
 *
 * For questions, comments please contact software@renci.org
 *
 * @section DESCRIPTION
 *
 * This file contains the definitions for the c code to exercise the Microsoft Azure Interface
 */

// azure includes
#include <was/storage_account.h>
#include <was/blob.h>
#include <cpprest/filestream.h>
#include <cpprest/containerstream.h>
#include <string.h>
#define AZURE_ACCOUNT "AZURE_ACCOUNT"
#define AZURE_ACCOUNT_KEY "AZURE_ACCOUNT_KEY"
#define AZURE_HTTPS_STRING "DefaultEndpointsProtocol=https"
#define AZURE_ACCOUNT_FILE "AZURE_ACCOUNT_FILE"
#define MAX_ACCOUNT_SIZE  256
#define MAX_ACCOUNT_KEY_SIZE  256
#define MAX_ERROR_LENGTH  512

extern "C" {
bool putNewFile(const char* _container,
                const char*  complete_file_path,
                const char* _file_name,
                const char* _connection,
                long long*   code,
                char*        message);

bool putTheFile( const char*  connection,
                 const char*  complete_file_path,
                 const char*  file_name,
                 const char*  prev_physical_path,
                 const char*  container,
                 long long*   code,
                 char*        message);

bool getTheFile(
    const char*      container,
    const char*      file_name,
    const char*      connection,
    const char*      destination,
    int              mode);

bool
getTheFileStatus (
    const char* container, 
    const char* file,  
    const char* connection);

bool
getTheFileLength (const char*       _container,
                  const char*       _file,
                  const char*       _connection,
                  long long*         length); 

bool deleteTheFile (
    const char* container, 
    const char* file,  
    const char* connection);
}; // extern "C" 
#endif
