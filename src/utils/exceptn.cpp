/*
* Exceptions
* (C) 1999-2007 Jack Lloyd
*
* Distributed under the terms of the Botan license
*/

#include <botan/exceptn.h>
#include <botan/parsing.h>

namespace Botan {

/*
* Constructor for Invalid_Key_Length
*/
Invalid_Key_Length::Invalid_Key_Length(const std::string& name, u32bit length)
   {
   set_msg(name + " cannot accept a key of length " + std::to_string(length));
   }

/*
* Constructor for Invalid_Block_Size
*/
Invalid_Block_Size::Invalid_Block_Size(const std::string& mode,
                                       const std::string& pad)
   {
   set_msg("Padding method " + pad + " cannot be used with " + mode);
   }

/*
* Constructor for Invalid_IV_Length
*/
Invalid_IV_Length::Invalid_IV_Length(const std::string& mode, u32bit bad_len)
   {
   set_msg("IV length " + std::to_string(bad_len) + " is invalid for " + mode);
   }

/*
* Constructor for Algorithm_Not_Found
*/
Algorithm_Not_Found::Algorithm_Not_Found(const std::string& name)
   {
   set_msg("Could not find any algorithm named \"" + name + "\"");
   }

/*
* Constructor for Invalid_Algorithm_Name
*/
Invalid_Algorithm_Name::Invalid_Algorithm_Name(const std::string& name)
   {
   set_msg("Invalid algorithm name: " + name);
   }

/*
* Constructor for Config_Error
*/
Config_Error::Config_Error(const std::string& err, u32bit line)
   {
   set_msg("Config error at line " + std::to_string(line) + ": " + err);
   }

}
