


// =-=-=-=-=-=-=-
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/stat.h>

// =-=-=-=-=-=-=-
// irods includes
#include <irods/rodsErrorTable.h>


// =-=-=-=-=-=-=-
// our own include file
#include "libazurewrapper.hpp"

// =-=-=-=-=-=-=-
// azure includes
#include <was/storage_account.h>
#include <was/blob.h>
#include <cpprest/filestream.h>  
#include <cpprest/containerstream.h> 


/*
 * This function is called to put a file into Azure storage
 *
 */

bool putNewFile(const char* _container, 
                const char*  complete_file_path, 
                const char* _file_name, 
                const char* _connection,
                long long*   code,
                char*        message) {

   std::string container = _container;
   std::string file_name = _file_name;
   utility::string_t connection = _connection;
   
   try { 
      // Retrieve storage account from connection string.
     //rodsLog(LOG_DEBUG6, "putNewFile: container %s complete_file_path %s, file_name %s", 
      //       container.c_str(), complete_file_path, file_name.c_str());
      azure::storage::cloud_storage_account account = 
         azure::storage::cloud_storage_account::parse(connection);

      // Create the blob client.
      azure::storage::cloud_blob_client client = account.create_cloud_blob_client();

      // Get the container reference
      azure::storage::cloud_blob_container azure_container = 
         client.get_container_reference(U(container));

      // create the container if it's not there already
      azure_container.create_if_not_exists();

      // Retrieve reference to a blob 
      azure::storage::cloud_block_blob blob = azure_container.get_block_blob_reference(U(file_name));

      // Create or overwrite the blob with contents from the local file
      concurrency::streams::istream input_stream = 
         concurrency::streams::file_stream<uint8_t>::open_istream(U(complete_file_path)).get();
      blob.upload_from_stream(input_stream);
      input_stream.close().wait();

   } catch (const std::exception& e) {
      // Note that in theory, it might be a good idea to try to delete the  
      // file from Azure at this point, in case of a partial write.  We aren't
      // going to do this because A) Hopefully the Azure call will roll back any partial
      // write on failure B) From the point of view of iRODS the file isn't there and C)
      // the default semantic of Azure is to overwrite existing files the next time a file
      // of the same name is written.
      //rodsLog (LOG_ERROR, "Could not put file in AZURE - [%s]", e.what());
      *code = UNIV_MSS_SYNCTOARCH_ERR;
      strcpy(message, e.what());
      return false;
   }

   return true;
}


/** 
 * @brief This function is the high level function that adds a data file
 *        to the AZURE storage.  It uses the microsoft libazurestorage to do this.
 *
 *  This function checks to see if the file we are putting is already on azure. If it
 *  is, then we delete the existing file and add the new one.
 *
 * @param complete_file_path The complete file path for the file
 * @param file_name The file name component
 * @param prev_physical_path The previos physical path
 * @param container The Azure container
 * @return res.  The wrapper_error results
 */
bool putTheFile(
    const char*      connection,
    const char*      complete_file_path, 
    const char*      file_name,
    const char*      prev_physical_path,
    const char*      container,
    long long *      code,
    char*            message) {

    // NOTE: At some point we will need to need to look at the "force flag" to do some error checking.
    // For now, we are just going to see if the file is already there. If it is, we will delete it
    // and re-add the new version.

    // =-=-=-=-=-=-=-
    // stat the file, if it exists on the system we need to check 
    // the size.  If it is non-zero, register first to get OID and then overwrite.
    // If it is zero then just put the file. 
    bool status = true;
    bool result = true;
    std::string prev_physical_str( prev_physical_path );
    //rodsLog(LOG_DEBUG6, "Putting the file: complete_file_path %s, file_name %s", 
    //        complete_file_path, file_name.c_str());

    // only query if the file path does not start with a "/".  Files already on azure will not.
    if (0 != prev_physical_str.find( "/")) {
        status = getTheFileStatus( 
                     container, 
                     file_name,
                     connection);
    }

    // non-zero status means file was there.  We technically don't need this since azure seems
    // to overwrite by default.  Left here because we may want (and be able to) change that behavior....
    if( status ) {
       // lets delete the file
       //rodsLog(LOG_DEBUG6, 
       //        "Calling deleteTheFile in putTheFile: container %s file_name %s" , 
       //         container.c_str(), file_name.c_str());
       status = deleteTheFile(container, file_name, connection); 
       //rodsLog(LOG_DEBUG6, "Result of deleteTheFile is %d", status);
       if (status == false) {
          //rodsLog(LOG_DEBUG6, "Erroring out of putTheFile after the delete");
          *code = UNIV_MSS_SYNCTOARCH_ERR;
          strcpy(message, "azureSyncToArchPlugin - error in deleteTheFile");
          return false;
       }
    }

    //rodsLog(LOG_DEBUG6, 
    //       "Calling putNewFile in putTheFile: container %s file_name %s complete_file_path %s" , 
    //        container.c_str(), file_name.c_str(), complete_file_path);
    result = putNewFile( container, complete_file_path,  file_name, connection, code, message);

    //rodsLog(LOG_DEBUG, "finished writing file - %s", complete_file_path);
    return result;
}

/** 
 * @brief This function is the high level function that retrieves a data file
 *        from the DDN storage using the Azure interface.
 *
 *  This function uses the Azure API to GET the specified file
 *  from Azure and put it in the cache.
 *
 * @param container The Azure container
 * @param file_name The file name component
 * @param resource A character pointer to the resource for this request.
 * @param connection The Azure connection string
 * @param destination A character pointer to the destination for this request.
 *        This is a file.
 * @param mode The file mode to be used when creating the file.  This comes
 *        from the users original file and is stored in the icat.
 * @return res.  The return code from curl_easy_perform.
 */

bool getTheFile(
    const char*      _container,
    const char*      _file_name,
    const char*      _connection,
    const char*       destination, 
    int               mode) {

    const std::string container = _container;
    const std::string file_name = _file_name;
    const utility::string_t connection = _connection;
    const utility::string_t destination_string = destination;

    // library
    const int destFd = open(destination, O_WRONLY | O_CREAT | O_TRUNC, mode);
    if (destFd < 0) {
        return(UNIX_FILE_OPEN_ERR - errno);
    } else {
       close(destFd);
       try {
          // Retrieve storage account from connection string.
          azure::storage::cloud_storage_account account = 
             azure::storage::cloud_storage_account::parse(connection);

          // Create the blob client.
          azure::storage::cloud_blob_client client = account.create_cloud_blob_client();

          // Retrieve a reference to a previously created container.
          azure::storage::cloud_blob_container azure_container = 
             client.get_container_reference(U(container));

          // Retrieve reference to a blob named "my-blob-2".
          azure::storage::cloud_block_blob text_blob = 
             azure_container.get_block_blob_reference(U(file_name));

          // Download the contents of a blog to the file
          // Not sure if this will preserve the mode we set above.
          text_blob.download_to_file(destination_string);
       } catch  (const std::exception& e) {
          //rodsLog (LOG_ERROR, "Could not check file existence in AZURE - [%s]", e.what());
          unlink(destination);
          return (false);
       }
    } 

    return true;

}

/** 
 * @brief This function is the low level function that retrieves the 
 *        status of a data file from the azure storage using the libazurestorage library.
 *
 *  This function checks to see if the specified file exists in azure. If so, it returns true, 
 *  otherwise false.  It may be possible to to get more info (like the size of the file) but
 *  that will be for later.
 *
 * @param container The container for the file (normally the checksum of a directory path)
 * @param file The file in which we are interested.
 * @param connection The azure connection string.
 * @return res.  Whether or not the file exists.
 */

bool 
getTheFileStatus (const char* _container, 
                  const char* _file,  
                  const char* _connection) {

  const std::string container = _container;
  const std::string file = _file;
  const utility::string_t connection = _connection;

   try {
      // Retrieve storage account from connection string.
      azure::storage::cloud_storage_account storage_account = 
         azure::storage::cloud_storage_account::parse(connection);

      // Create the blob client.
      azure::storage::cloud_blob_client blob_client = 
         storage_account.create_cloud_blob_client();

      // Get the container
      azure::storage::cloud_blob_container azure_container = blob_client.get_container_reference(U(container));

      // Now get the blob
      azure::storage::cloud_block_blob the_blob = azure_container.get_block_blob_reference(U(file));
      
      // Return whether or not this blob exists.
      return (the_blob.exists());
   }
   catch (const std::exception& e) {
      //rodsLog (LOG_ERROR, "Could not check file existence in AZURE - [%s]", e.what());
      return (false);
   }

}

/** 
 * @brief This function is the low level function that retrieves the 
 *        length of a data file from the azure storage using the libazurestorage library.
 *
 *  This function checks to see if the specified file exists in azure. If so, it continues 
 *  on to return the length of the file, otherwise it returns -1 (0 length may be possible)
 *
 * @param container The container for the file (normally the checksum of a directory path)
 * @param file The file in which we are interested.
 * @param connection The azure connection string.
 * param  length
 * @return true on success, false on failure
 */

bool 
getTheFileLength (const char*       _container, 
                  const char*       _file,  
                  const char*       _connection, 
                  long long*         length) {

   const std::string container = _container;
   const std::string file = _file;
   const utility::string_t connection = _connection;

   bool result = true;
   utility::size64_t azure_length;

   try {
      //rodsLog(LOG_DEBUG6, "Getting the length of %s", file.c_str());
      // Retrieve storage account from connection string.
      azure::storage::cloud_storage_account storage_account = 
         azure::storage::cloud_storage_account::parse(connection);

      // Create the blob client.
      azure::storage::cloud_blob_client blob_client = 
         storage_account.create_cloud_blob_client();

      // Get the container
      azure::storage::cloud_blob_container azure_container = blob_client.get_container_reference(U(container));

      // Now get the blob
      azure::storage::cloud_block_blob the_blob = azure_container.get_block_blob_reference(U(file));
      
      // if the file existe. we get it's length
      if (the_blob.exists()) {
         azure::storage::cloud_blob_properties &props = the_blob.properties();
         azure_length = props.size();
         *length = static_cast<long long>(azure_length);
          //rodsLog(LOG_DEBUG6, "Setting the length of %s to %ld", file.c_str(), *length);
      } else {
        // The file doesn't exist. Signal this to the caller.
        *length = -1;
     }
   } catch (const std::exception& e) {
      //rodsLog (LOG_ERROR, "Could not check file existence in AZURE - [%s]", e.what());
      return (false);
   }
   
   return result;
}

/** 
 * @brief This function is the high level function that deletes a data file
 *        from azure storage using the libazure interface. 
 *
 * @param container The container for the file (normally the checksum of a directory path)
 * @param file The file in which we are interested.
 * @param connection The azure connection string.
 * @return res.  Whether or not the file was successfully deleted. Note that if the file was not
 *               there to be deleted, the function returns true.  This preserves the semantic
 *               that after this code is executed, the file is not on the system.
 */
bool deleteTheFile (const char* _container, 
                           const char* _file,  
                           const char* _connection) {


   const std::string container = _container;
   const std::string file = _file;
   const utility::string_t connection = _connection;
   bool result = true;

   const utility::string_t utilConnection = connection;
   try {
      // Retrieve storage account from connection string.
      //rodsLog(LOG_DEBUG6, "In deleteTheFile: Retrieving the storage account from %s", utilConnection.c_str());
      azure::storage::cloud_storage_account storage_account = 
         azure::storage::cloud_storage_account::parse(utilConnection);

      //rodsLog(LOG_DEBUG6, "Trying to create the client");
      // Create the blob client.
      azure::storage::cloud_blob_client blob_client = 
         storage_account.create_cloud_blob_client();
      //rodsLog(LOG_DEBUG6, "Created the client");

      // Get the container
      //rodsLog(LOG_DEBUG6, "Trying to get the container for %s", container.c_str());
      azure::storage::cloud_blob_container azure_container = blob_client.get_container_reference(U(container));
      //rodsLog(LOG_DEBUG6, "Got the container");

      // Now get the blob
      azure::storage::cloud_block_blob the_blob = azure_container.get_block_blob_reference(U(file));
      
      if (the_blob.exists()) {
         // Try to delete the blob.
         the_blob.delete_blob();
         return (result);
      } else {
         // you tried to delete it, and it's not there. No reason to call that a failure.
         return (result);
      }
   }
   catch (const std::exception& e) {
      //rodsLog (LOG_ERROR, "Could not delete blob in AZURE - [%s]", e.what());
      return (false);
   }
}
